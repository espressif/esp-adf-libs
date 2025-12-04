/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "audio_codec_test.h"
#include "esp_audio_codec_version.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "unity_test_utils_memory.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_es_parse_types.h"
#include "test_common.h"
#include "esp_board_manager.h"

extern const char test_mp3_start[] asm("_binary_test_mp3_start");
extern const char test_mp3_end[] asm("_binary_test_mp3_end");
extern const char test_flac_start[] asm("_binary_test_flac_start");
extern const char test_flac_end[] asm("_binary_test_flac_end");

#define TAG            "CODEC_TEST"
#define MAX_GEN_SIZE   (80 * 1024)
#define WAVE_AMPLITUDE (16000.0)
#define WAVE_FREQUECY  (1000)
#define RING_FIFO_SIZE (32 * 1024)
#define LEAKS          (400)

typedef enum {
    DECODE_STATE_NONE,
    DECODE_STATE_RUNNING,
    DECODE_STATE_ERR,
} decode_state_t;

typedef struct {
    esp_audio_type_t     type;
    audio_info_t         info;
    uint8_t             *wav_pcm;
    int                  wav_size;
    RingbufHandle_t      fifo;
    int                  read_pcm_size;
    int                  write_pcm_size;
    decode_state_t       dec_state;
    float                signal_power;
    float                noise_power;
    int                  sample_shift;
    int                  wav_peak;
    int                  pcm_peak;
    bool                 use_test_data;
    esp_es_parse_type_t  parse_type;
    int                  test_send_size;
    int                  test_file_size;
    uint8_t             *test_frames;
    FILE                *enc_file;
    FILE                *dec_file;
    bool                 need_reset;
} chain_info_t;

static chain_info_t           chain;
static bool                   chain_testing = false;
static audio_codec_test_cfg_t enc_test_cfg;
static audio_codec_test_cfg_t dec_test_cfg;

extern esp_es_parse_func_t esp_es_parse_get_default(esp_es_parse_type_t parse_type);

static int read_pcm(uint8_t *data, int size)
{
    if (chain.read_pcm_size + size <= chain.wav_size) {
        memcpy(data, chain.wav_pcm + chain.read_pcm_size, size);
        chain.read_pcm_size += size;
        return size;
    }
    // Quit encoding
    return 0;
}

static int read_raw(uint8_t *data, int size)
{
    size_t item_size = 0;
    uint8_t *item = (uint8_t *)xRingbufferReceive(chain.fifo, &item_size, portMAX_DELAY);
    if (item) {
        if (size >= item_size) {
            memcpy(data, item, item_size);
        } else {
            ESP_LOGE(TAG, "Size not enough need %d", (int)item_size);
        }
        vRingbufferReturnItem(chain.fifo, item);
    }
    return size >= item_size ? item_size : 0;
}

static bool has_phase_shift(esp_audio_type_t type)
{
    switch (type) {
        case ESP_AUDIO_TYPE_AAC:
        case ESP_AUDIO_TYPE_OPUS:
        case ESP_AUDIO_TYPE_AMRNB:
        case ESP_AUDIO_TYPE_AMRWB:
        case ESP_AUDIO_TYPE_SBC:
        case ESP_AUDIO_TYPE_LC3:
        case ESP_AUDIO_TYPE_ALAC:
            return true;
        default:
            break;
    }
    return false;
}

static int write_pcm(uint8_t *data, int size)
{
    if (chain.use_test_data) {
        return size;
    }
    if (chain.write_pcm_size + size > chain.read_pcm_size) {
        ESP_LOGE(TAG, "Decode too many samples");
    } else if (chain.info.bits_per_sample == 16) {
        int samples = size / (chain.info.bits_per_sample >> 3);
        int16_t *v = (int16_t *)data;
        int16_t *wav = (int16_t *)(chain.wav_pcm + chain.write_pcm_size);
        wav -= chain.sample_shift;
        float signal = 0;
        float noise = 0;
        int start_sample = 0;
        if (chain.sample_shift == 0 && has_phase_shift(chain.type)) {
            start_sample = chain.write_pcm_size / (chain.info.bits_per_sample >> 3);
            if (chain.wav_peak == 0) {
                for (int i = chain.info.channel; i < samples - chain.info.channel; i += chain.info.channel) {
                    if (wav[i] > wav[i - chain.info.channel] && wav[i] > wav[i + chain.info.channel]) {
                        chain.wav_peak = start_sample + i;
                        ESP_LOGD(TAG, "Wav peak at %d value %d\n", chain.wav_peak, (int)wav[i]);
                        break;
                    }
                }
            }
            if (chain.pcm_peak == 0) {
                for (int i = chain.info.channel; i < samples - chain.info.channel; i += chain.info.channel) {
                    if (v[i] > WAVE_AMPLITUDE / 2 && v[i] > v[i - chain.info.channel] && v[i] > v[i + chain.info.channel]) {
                        chain.pcm_peak = start_sample + i;
                        ESP_LOGD(TAG, "PCM peak at %d value %d\n", chain.pcm_peak, (int)v[i]);
                        break;
                    }
                }
            }
            start_sample = samples;
            if (chain.wav_peak && chain.pcm_peak) {
                chain.sample_shift = chain.pcm_peak - chain.wav_peak;
                if (samples > chain.sample_shift) {
                    start_sample = samples - chain.sample_shift;
                    v += start_sample;
                }
            }
        }
        for (int i = start_sample; i < samples; i++) {
            signal += (*v) * (*v);
            int16_t d = *wav - *v;
            noise += d * d;
            v++;
            wav++;
        }
        chain.signal_power += signal / samples;
        chain.noise_power += noise / samples;
    }
    chain.write_pcm_size += size;
    return size;
}

static int write_pcm_to_file(uint8_t *data, int size)
{
    fwrite(data, 1, size, chain.dec_file);
    chain.write_pcm_size += size;
    return size;
}

static int write_pcm_to_compare(uint8_t *data, int size)
{
    uint8_t *cmp_buf = calloc(1, size);
    chain.write_pcm_size += size;
    fread(cmp_buf, 1, size, chain.dec_file);
    if (memcmp(cmp_buf, data, size) != 0) {
        ESP_LOGE(TAG, "Decoded data not match");
        free(cmp_buf);
        return -1;
    }
    free(cmp_buf);
    return size;
}

static void dec_thread(void *arg)
{
    if (audio_decoder_test(chain.type, &dec_test_cfg, &chain.info, chain.need_reset) != 0) {
        chain.dec_state = DECODE_STATE_ERR;
    } else {
        chain.dec_state = DECODE_STATE_NONE;
    }
    ESP_LOGI(TAG, "Decode Thread finished");
    vTaskDelete(NULL);
}

static int write_raw(uint8_t *data, int size)
{
    if (chain.dec_state == DECODE_STATE_NONE) {
        chain.dec_state = DECODE_STATE_RUNNING;
        xTaskCreatePinnedToCore(dec_thread, "Decode", 20 * 1024, NULL, 10, NULL, 0);
    }
    xRingbufferSend(chain.fifo, (void *)data, size, portMAX_DELAY);
    return 0;
}

static int write_raw_to_file(uint8_t *data, int size)
{
    if (chain.dec_state == DECODE_STATE_NONE) {
        chain.dec_state = DECODE_STATE_RUNNING;
        xTaskCreatePinnedToCore(dec_thread, "Decode", 20 * 1024, NULL, 10, NULL, 0);
    }
    xRingbufferSend(chain.fifo, (void *)data, size, portMAX_DELAY);
    fwrite(data, 1, size, chain.enc_file);
    return 0;
}

static int write_raw_to_compare(uint8_t *data, int size)
{
    if (chain.dec_state == DECODE_STATE_NONE) {
        chain.dec_state = DECODE_STATE_RUNNING;
        xTaskCreatePinnedToCore(dec_thread, "Decode", 20 * 1024, NULL, 10, NULL, 0);
    }
    uint8_t *cmp_buf = calloc(1, size);
    xRingbufferSend(chain.fifo, (void *)data, size, portMAX_DELAY);
    fread(cmp_buf, 1, size, chain.enc_file);
    if (memcmp(cmp_buf, data, size) != 0) {
        ESP_LOGE(TAG, "Encoded data not match");
        free(cmp_buf);
        return -1;
    }
    free(cmp_buf);
    return 0;
}

static int get_test_raw_frame(uint8_t **data, int *size, bool is_bos)
{
    esp_es_parse_func_t parser = esp_es_parse_get_default(chain.parse_type);
    if (parser == NULL) {
        *size = 0;
        return -1;
    }
REPARSE:
    if (chain.test_send_size >= chain.test_file_size) {
        *size = 0;
        return -1;
    }
    esp_es_parse_raw_t raw = {
        .buffer = chain.test_frames + chain.test_send_size,
        .len = chain.test_file_size - chain.test_send_size,
        .bos = is_bos,
    };
    esp_es_parse_frame_info_t frame_info = {0};
    esp_es_parse_err_t ret = parser(&raw, &frame_info);
    if (ret == ESP_ES_PARSE_ERR_OK || ret == ESP_ES_PARSE_ERR_SKIP_ONLY) {
        if (frame_info.skipped_size > 0) {
            is_bos = false;
            chain.test_send_size += frame_info.skipped_size;
            goto REPARSE;
        }
        *size = frame_info.frame_size;
        *data = raw.buffer;
        chain.test_send_size += frame_info.frame_size;
    } else if (ret == ESP_ES_PARSE_ERR_WRONG_HEADER) {
        chain.test_send_size += 1;
        goto REPARSE;
    } else {
        *size = 0;
        return -1;
    }
    return 0;
}

static bool encoder_use_test_data(esp_audio_type_t type)
{
    chain.test_send_size = 0;
    chain.test_file_size = 0;
    switch (type) {
        case ESP_AUDIO_TYPE_MP3:
            chain.parse_type = ESP_ES_PARSE_TYPE_MP3;
            chain.test_file_size = (int)(test_mp3_end - test_mp3_start);
            chain.test_frames = (uint8_t *)test_mp3_start;
            return true;
        case ESP_AUDIO_TYPE_FLAC:
            chain.parse_type = ESP_ES_PARSE_TYPE_FLAC;
            chain.test_file_size = (int)(test_flac_end - test_flac_start);
            chain.test_frames = (uint8_t *)test_flac_start;
            return true;
        default:
            return false;
    }
}

static int encoder_to_decoder(esp_audio_type_t type, int sample_rate, uint8_t channel,
                              bool need_reset, FILE *enc_file, FILE *dec_file)
{
    memset(&chain, 0, sizeof(chain_info_t));
    chain.type = type;
    chain.info.sample_rate = sample_rate;
    chain.info.bits_per_sample = 16;
    chain.info.channel = channel;
    chain.info.no_file_header = true;
    chain.need_reset = need_reset;
    chain.enc_file = enc_file;
    chain.dec_file = dec_file;
    int ret = 0;
    chain.use_test_data = encoder_use_test_data(type);
    bool is_bos = true;
    do {
        chain.fifo = xRingbufferCreate(RING_FIFO_SIZE, RINGBUF_TYPE_NOSPLIT);
        if (chain.fifo == NULL) {
            break;
        }
        if (chain.use_test_data == false) {
            // Generate PCM data and do encode
            chain.wav_pcm = malloc(MAX_GEN_SIZE);
            if (chain.wav_pcm == NULL) {
                break;
            }
            chain.wav_size = audio_codec_gen_pcm(&chain.info, chain.wav_pcm, MAX_GEN_SIZE);
            audio_encoder_test(type, &enc_test_cfg, &chain.info, chain.need_reset);
        } else {
            // Directly read frame data use parser
            while (1) {
                uint8_t *frame_data = NULL;
                int frame_size = 0;
                get_test_raw_frame(&frame_data, &frame_size, is_bos);
                if (frame_size == 0) {
                    break;
                }
                is_bos = false;
                write_raw(frame_data, frame_size);
            }
        }
        // Let read thread quit
        xRingbufferSend(chain.fifo, (void *)"quit", 4, portMAX_DELAY);
    } while (0);
    while (chain.dec_state == DECODE_STATE_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (chain.use_test_data == false) {
        if (chain.read_pcm_size != chain.write_pcm_size) {
            ESP_LOGE(TAG, "Encoded data size %d not equal to decoded size %d", chain.read_pcm_size, chain.write_pcm_size);
        }
        if (chain.signal_power > 0 && chain.noise_power > 0) {
            float snr = 10 * log10(chain.signal_power / chain.noise_power);
            if (snr < 20) {
                ESP_LOGE(TAG, "Have issue with decoder or encoder for %s, SNR: %f", esp_audio_codec_get_name(type), snr);
            } else {
                ESP_LOGI(TAG, "SNR %f", snr);
            }
        } else {
            ESP_LOGI(TAG, "Lossless audio check OK");
        }
    }
    if (chain.wav_pcm) {
        free(chain.wav_pcm);
        chain.wav_pcm = NULL;
    }
    if (chain.fifo) {
        vRingbufferDelete(chain.fifo);
        chain.fifo = NULL;
    }
    ESP_LOGI(TAG, "Chain test done\n");
    return ret;
}

static void chain_test_thread(void *arg)
{
    esp_audio_type_t types[] = {
        ESP_AUDIO_TYPE_AAC,
        ESP_AUDIO_TYPE_G711A,
        ESP_AUDIO_TYPE_G711U,
        ESP_AUDIO_TYPE_AMRNB,
        ESP_AUDIO_TYPE_AMRWB,
        ESP_AUDIO_TYPE_ADPCM,
        ESP_AUDIO_TYPE_OPUS,
        ESP_AUDIO_TYPE_ALAC,
        ESP_AUDIO_TYPE_MP3,
        ESP_AUDIO_TYPE_FLAC,
        ESP_AUDIO_TYPE_SBC,
        ESP_AUDIO_TYPE_LC3,
    };
    int sample_rate = 48000;
    uint8_t channel = 2;
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        int new_sample_rate = sample_rate;
        int new_channel = channel;
        if (types[i] == ESP_AUDIO_TYPE_G711A || types[i] == ESP_AUDIO_TYPE_G711U || types[i] == ESP_AUDIO_TYPE_AMRNB) {
            new_sample_rate = 8000;
            new_channel = 1;
        } else if (types[i] == ESP_AUDIO_TYPE_AMRWB) {
            new_sample_rate = 16000;
            new_channel = 1;
        } else if (types[i] == ESP_AUDIO_TYPE_MP3 || types[i] == ESP_AUDIO_TYPE_FLAC) {
            new_sample_rate = 44100;
            new_channel = 2;
        } else if (types[i] == ESP_AUDIO_TYPE_SBC || types[i] == ESP_AUDIO_TYPE_LC3) {
            new_sample_rate = 48000;
            new_channel = 2;
        }
        ESP_LOGI(TAG, "Start to do chain test for %s sample_rate %d channel %d", esp_audio_codec_get_name(types[i]),
                 new_sample_rate, new_channel);
        enc_test_cfg.read = read_pcm;
        enc_test_cfg.write = write_raw;
        dec_test_cfg.read = read_raw;
        dec_test_cfg.write = write_pcm;
        encoder_to_decoder(types[i], new_sample_rate, new_channel, false, NULL, NULL);
    }
    chain_testing = false;
    vTaskDelete(NULL);
}

static void chain_reset_test_thread(void *arg)
{
    esp_audio_type_t types[] = {
        ESP_AUDIO_TYPE_AAC,
        ESP_AUDIO_TYPE_G711A,
        ESP_AUDIO_TYPE_G711U,
        ESP_AUDIO_TYPE_AMRNB,
        ESP_AUDIO_TYPE_AMRWB,
        ESP_AUDIO_TYPE_ADPCM,
        ESP_AUDIO_TYPE_OPUS,
        ESP_AUDIO_TYPE_ALAC,
        ESP_AUDIO_TYPE_MP3,
        ESP_AUDIO_TYPE_FLAC,
        ESP_AUDIO_TYPE_SBC,
        ESP_AUDIO_TYPE_LC3,
    };
    int sample_rate = 48000;
    uint8_t channel = 2;
    char enc_file_name[100];
    char dec_file_name[100];
    FILE *enc_file = NULL;
    FILE *dec_file = NULL;
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        int new_sample_rate = sample_rate;
        int new_channel = channel;
        if (types[i] == ESP_AUDIO_TYPE_G711A || types[i] == ESP_AUDIO_TYPE_G711U || types[i] == ESP_AUDIO_TYPE_AMRNB) {
            new_sample_rate = 8000;
            new_channel = 1;
        } else if (types[i] == ESP_AUDIO_TYPE_AMRWB) {
            new_sample_rate = 16000;
            new_channel = 1;
        } else if (types[i] == ESP_AUDIO_TYPE_MP3 || types[i] == ESP_AUDIO_TYPE_FLAC) {
            new_sample_rate = 44100;
            new_channel = 2;
        } else if (types[i] == ESP_AUDIO_TYPE_SBC || types[i] == ESP_AUDIO_TYPE_LC3) {
            new_sample_rate = 48000;
            new_channel = 2;
        }
        ESP_LOGI(TAG, "Start to do chain test for %s sample_rate %d channel %d", (char *)esp_audio_codec_get_name(types[i]),
                 new_sample_rate, new_channel);
        if (types[i] != ESP_AUDIO_TYPE_MP3 && types[i] != ESP_AUDIO_TYPE_FLAC) {
            snprintf(enc_file_name, sizeof(enc_file_name), "/sdcard/enc_%s.%s",
                     (char *)esp_audio_codec_get_name(types[i]), (char *)esp_audio_codec_get_name(types[i]));
            enc_file = fopen(enc_file_name, "wb");
            if (enc_file == NULL) {
                ESP_LOGE(TAG, "Fail to open enc file %s", enc_file_name);
                continue;
            }
            snprintf(dec_file_name, sizeof(dec_file_name), "/sdcard/dec_%s.pcm", (char *)esp_audio_codec_get_name(types[i]));
            dec_file = fopen(dec_file_name, "wb");
            if (dec_file == NULL) {
                ESP_LOGE(TAG, "Fail to open dec file %s", dec_file_name);
                continue;
            }
            enc_test_cfg.read = read_pcm;
            enc_test_cfg.write = write_raw_to_file;
            dec_test_cfg.read = read_raw;
            dec_test_cfg.write = write_pcm_to_file;
        } else {
            snprintf(dec_file_name, sizeof(dec_file_name), "/sdcard/dec_%s.pcm", (char *)esp_audio_codec_get_name(types[i]));
            dec_file = fopen(dec_file_name, "wb");
            if (dec_file == NULL) {
                ESP_LOGE(TAG, "Fail to open dec file %s", dec_file_name);
                continue;
            }
            dec_test_cfg.read = read_raw;
            dec_test_cfg.write = write_pcm_to_file;
        }
        encoder_to_decoder(types[i], new_sample_rate, new_channel, true, enc_file, dec_file);
        if (chain.enc_file) {
            fclose(chain.enc_file);
            chain.enc_file = NULL;
            enc_file = NULL;
        }
        if (chain.dec_file) {
            fclose(chain.dec_file);
            chain.dec_file = NULL;
            dec_file = NULL;
        }
        if (types[i] != ESP_AUDIO_TYPE_MP3 && types[i] != ESP_AUDIO_TYPE_FLAC) {
            snprintf(enc_file_name, sizeof(enc_file_name), "/sdcard/enc_%s.%s",
                     (char *)esp_audio_codec_get_name(types[i]), (char *)esp_audio_codec_get_name(types[i]));
            enc_file = fopen(enc_file_name, "rb");
            if (enc_file == NULL) {
                ESP_LOGE(TAG, "Fail to open enc file %s", enc_file_name);
                continue;
            }
            snprintf(dec_file_name, sizeof(dec_file_name), "/sdcard/dec_%s.pcm", (char *)esp_audio_codec_get_name(types[i]));
            dec_file = fopen(dec_file_name, "rb");
            if (dec_file == NULL) {
                ESP_LOGE(TAG, "Fail to open dec file %s", dec_file_name);
                continue;
            }
            enc_test_cfg.read = read_pcm;
            enc_test_cfg.write = write_raw_to_compare;
            dec_test_cfg.read = read_raw;
            dec_test_cfg.write = write_pcm_to_compare;
        } else {
            snprintf(dec_file_name, sizeof(dec_file_name), "/sdcard/dec_%s.pcm", (char *)esp_audio_codec_get_name(types[i]));
            dec_file = fopen(dec_file_name, "rb");
            if (dec_file == NULL) {
                ESP_LOGE(TAG, "Fail to open dec file %s", dec_file_name);
                continue;
            }
            dec_test_cfg.read = read_raw;
            dec_test_cfg.write = write_pcm_to_compare;
        }
        encoder_to_decoder(types[i], new_sample_rate, new_channel, false, enc_file, dec_file);
        if (chain.enc_file) {
            fclose(chain.enc_file);
            chain.enc_file = NULL;
            enc_file = NULL;
        }
        if (chain.dec_file) {
            fclose(chain.dec_file);
            chain.dec_file = NULL;
            dec_file = NULL;
        }
    }
    chain_testing = false;
    vTaskDelete(NULL);
}

int audio_codec_gen_pcm(audio_info_t *info, uint8_t *data, int size)
{
    int sample_size = info->bits_per_sample * info->channel >> 3;
    // align to samples
    int sample_count = size / sample_size;
    int pcm_size = sample_count * sample_size;
    // only support 30k
    if (info->bits_per_sample == 16) {
        int i = 0;
        int16_t *v = (int16_t *)data;
        int fs = info->sample_rate >= 32000 ? WAVE_FREQUECY : info->sample_rate / 32;
        // Gen 1KHZ tone
        float coef = 6.2831853 * fs / info->sample_rate;
        while (i < sample_count) {
            int16_t t = (int16_t)(WAVE_AMPLITUDE * sin(coef * i));
            *(v++) = t;
            if (info->channel == 2) {
                *(v++) = t;
            }
            i++;
        }
    }
    return pcm_size;
}

TEST_CASE("Encode to Decode chain test", CODEC_TEST_MODULE_NAME)
{
    chain_testing = true;
    xTaskCreatePinnedToCore(chain_test_thread, "Chain_Test", 40 * 1024, NULL, 20, NULL, 1);
    while (chain_testing) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

TEST_CASE("Encode to Decode chain reset test", CODEC_TEST_MODULE_NAME)
{
    esp_board_manager_init();
    chain_testing = true;
    xTaskCreatePinnedToCore(chain_reset_test_thread, "Chain_Reset_Test", 40 * 1024, NULL, 20, NULL, 1);
    while (chain_testing) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_board_manager_deinit();
}

void setUp(void)
{
    unity_utils_record_free_mem();
}

void tearDown(void)
{
    unity_utils_evaluate_leaks_direct(LEAKS);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Start test for esp_audio_codec version %s", esp_audio_codec_get_version());
    float v = 1.0;
    printf("This line is specially used for pre-allocate float print memory %.2f\n", v);
    unity_run_menu();
}
