/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "audio_codec_test.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_simple_dec.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG "SIMP_DEC_TEST"

typedef union {
    esp_m4a_dec_cfg_t m4a_cfg;
    esp_ts_dec_cfg_t  ts_cfg;
    esp_aac_dec_cfg_t aac_cfg;
} simp_dec_all_t;

typedef struct {
    uint8_t *data;
    int      read_size;
    int      size;
} read_ctx_t;

typedef struct {
    uint8_t *data;
    int      write_size;
    int      read_size;
    int      size;
    int      decode_size;
} write_ctx_t;

static write_ctx_t write_ctx;
static read_ctx_t  read_ctx;
static FILE       *simp_dec_fp;

static esp_audio_simple_dec_type_t get_simple_decoder_type(char *file)
{
    char *ext = strrchr(file, '.');
    if (ext == NULL) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_NONE;
    }
    ext++;
    if (strcasecmp(ext, "aac") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AAC;
    }
    if (strcasecmp(ext, "mp3") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_MP3;
    }
    if (strcasecmp(ext, "amrnb") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB;
    }
    if (strcasecmp(ext, "amrwb") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB;
    }
    if (strcasecmp(ext, "flac") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC;
    }
    if (strcasecmp(ext, "wav") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_WAV;
    }
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "m4a") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_M4A;
    }
    if (strcasecmp(ext, "ts") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_TS;
    }
    return ESP_AUDIO_SIMPLE_DEC_TYPE_NONE;
}

static void get_simple_decoder_config(esp_audio_simple_dec_cfg_t *cfg)
{
    simp_dec_all_t *all_cfg = (simp_dec_all_t *)cfg->dec_cfg;
    switch (cfg->dec_type) {
        case ESP_AUDIO_SIMPLE_DEC_TYPE_AAC: {
            esp_aac_dec_cfg_t *aac_cfg = &all_cfg->aac_cfg;
            aac_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_aac_dec_cfg_t);
            break;
        }
        case ESP_AUDIO_SIMPLE_DEC_TYPE_M4A: {
            esp_m4a_dec_cfg_t *m4a_cfg = &all_cfg->m4a_cfg;
            m4a_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_m4a_dec_cfg_t);
            break;
        }
        case ESP_AUDIO_SIMPLE_DEC_TYPE_TS: {
            esp_ts_dec_cfg_t *ts_cfg = &all_cfg->ts_cfg;
            ts_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_ts_dec_cfg_t);
            break;
        }
        default:
            break;
    }
}

static int read_raw_from_file(uint8_t *data, int size)
{
    return fread(data, 1, size, simp_dec_fp);
}

static int encoder_read_pcm(uint8_t *data, int size)
{
    if (read_ctx.read_size + size <= read_ctx.size) {
        memcpy(data, read_ctx.data + read_ctx.read_size, size);
        read_ctx.read_size += size;
        return size;
    }
    return 0;
}

static int encoder_write_frame(uint8_t *data, int size)
{
    if (write_ctx.write_size + size < write_ctx.size) {
        memcpy(write_ctx.data + write_ctx.write_size, data, size);
        write_ctx.write_size += size;
        return size;
    }
    return 0;
}

static int get_encode_data(audio_info_t *aud_info)
{
    read_ctx.size = audio_codec_gen_pcm(aud_info, read_ctx.data, read_ctx.size);
    read_ctx.read_size = 0;

    // Encode AAC, encoded data will be sent out through callback `decode_one_frame`
    audio_codec_test_cfg_t enc_cfg = {
        .read = encoder_read_pcm,
        .write = encoder_write_frame,
    };
    audio_encoder_test(ESP_AUDIO_TYPE_AAC, &enc_cfg, aud_info);
    return 0;
}

static int simple_decoder_read_data(uint8_t *data, int size)
{
    int left = write_ctx.write_size - write_ctx.read_size;
    if (left > size) {
        left = size;
    }
    if (left) {
        memcpy(data, write_ctx.data + write_ctx.read_size, left);
        write_ctx.read_size += left;
    }
    return left;
}

static int simple_decoder_write_pcm(uint8_t *data, int size)
{
    write_ctx.decode_size += size;
    return size;
}

int audio_simple_decoder_test(esp_audio_simple_dec_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info)
{
    esp_audio_dec_register_default();
    int ret = esp_audio_simple_dec_register_default();
    int max_out_size = 4096;
    int read_size = 512;
    uint8_t *in_buf = malloc(read_size);
    uint8_t *out_buf = malloc(max_out_size);
    esp_audio_simple_dec_handle_t decoder = NULL;
    do {
        if (in_buf == NULL || out_buf == NULL) {
            ESP_LOGI(TAG, "No memory for decoder");
            ret = ESP_AUDIO_ERR_MEM_LACK;
            break;
        }
        simp_dec_all_t all_cfg = {};
        esp_audio_simple_dec_cfg_t dec_cfg = {
            .dec_type = type,
            .dec_cfg = &all_cfg,
        };
        get_simple_decoder_config(&dec_cfg);
        ret = esp_audio_simple_dec_open(&dec_cfg, &decoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGI(TAG, "Fail to open simple decoder ret %d", ret);
            break;
        }
        int total_decoded = 0;
        uint64_t decode_time = 0;
        esp_audio_simple_dec_raw_t raw = {
            .buffer = in_buf,
        };
        uint64_t read_start = esp_timer_get_time();
        while (ret == ESP_AUDIO_ERR_OK) {
            ret = cfg->read(in_buf, read_size);
            if (ret < 0) {
                break;
            }
            raw.buffer = in_buf;
            raw.len = ret;
            raw.eos = (ret < read_size);
            esp_audio_simple_dec_out_t out_frame = {
                .buffer = out_buf,
                .len = max_out_size,
            };
            // ATTENTION: when input raw data unconsumed (`raw.len > 0`) do not overwrite its content
            // Or-else unexpected error may happen for data corrupt.
            while (raw.len) {
                uint64_t start = esp_timer_get_time();
                if (start > read_start + 30000000) {
                    raw.eos = true;
                    break;
                }
                ret = esp_audio_simple_dec_process(decoder, &raw, &out_frame);
                decode_time += esp_timer_get_time() - start;
                if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                    // Handle output buffer not enough case
                    uint8_t *new_buf = realloc(out_buf, out_frame.needed_size);
                    if (new_buf == NULL) {
                        break;
                    }
                    out_buf = new_buf;
                    out_frame.buffer = new_buf;
                    max_out_size = out_frame.needed_size;
                    out_frame.len = max_out_size;
                    continue;
                }
                if (ret != ESP_AUDIO_ERR_OK) {
                    ESP_LOGE(TAG, "Fail to decode data ret %d", ret);
                    break;
                }
                if (out_frame.decoded_size) {
                    if (total_decoded == 0) {
                        // Update audio information
                        esp_audio_simple_dec_info_t dec_info = {};
                        esp_audio_simple_dec_get_info(decoder, &dec_info);
                        info->sample_rate = dec_info.sample_rate;
                        info->bits_per_sample = dec_info.bits_per_sample;
                        info->channel = dec_info.channel;
                    }
                    total_decoded += out_frame.decoded_size;
                    if (cfg->write) {
                        cfg->write(out_frame.buffer, out_frame.decoded_size);
                    }
                }
                // In case that input data contain multiple frames
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
            if (raw.eos) {
                break;
            }
        }
        if (total_decoded) {
            int sample_size = info->channel * info->bits_per_sample >> 3;
            float cpu_usage = (float)decode_time * sample_size * info->sample_rate / total_decoded / 10000;
            ESP_LOGI(TAG, "Decode for %d cpu: %.2f%%", type, cpu_usage);
        }
    } while (0);
    esp_audio_simple_dec_close(decoder);
    esp_audio_simple_dec_unregister_default();
    esp_audio_dec_unregister_default();
    if (in_buf) {
        free(in_buf);
    }
    if (out_buf) {
        free(out_buf);
    }
    return ret;
}

int audio_simple_decoder_test_file(char *file, codec_write_cb writer, audio_info_t *info)
{
    esp_audio_simple_dec_type_t type = get_simple_decoder_type(file);
    if (type == ESP_AUDIO_SIMPLE_DEC_TYPE_NONE) {
        ESP_LOGE(TAG, "Not supported file format %s", file);
        return -1;
    }
    simp_dec_fp = fopen(file, "rb");
    if (simp_dec_fp == NULL) {
        ESP_LOGE(TAG, "File %s not found", file);
        return -1;
    }
    audio_codec_test_cfg_t dec_cfg = {
        .read = read_raw_from_file,
        .write = writer,
    };
    int ret = audio_simple_decoder_test(type, &dec_cfg, info);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to do simple decoder process");
    }
    fclose(simp_dec_fp);
    return ret;
}

TEST_CASE("AAC simple decoder test", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    // Prepare buffer to hold pcm data and encoded data
    memset(&read_ctx, 0, sizeof(read_ctx));

    int size = 40 * 1024;
    read_ctx.data = malloc(size);
    TEST_ASSERT_NOT_NULL(read_ctx.data);
    read_ctx.size = size;

    memset(&write_ctx, 0, sizeof(write_ctx));
    size = 20 * 1024;
    write_ctx.data = malloc(size);
    TEST_ASSERT_NOT_NULL(write_ctx.data);
    write_ctx.size = size;
    audio_info_t aud_info = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 2,
    };
    // Get encoded data and save into `write_ctx.data`
    TEST_ESP_OK(get_encode_data(&aud_info));

    // Do simple decoder test to read encoded data, gather the decoded pcm data size to `write_ctx.decode_size`
    audio_info_t dec_info = { 0 };
    audio_codec_test_cfg_t dec_cfg = {
        .read = simple_decoder_read_data,
        .write = simple_decoder_write_pcm,
    };
    TEST_ESP_OK(audio_simple_decoder_test(ESP_AUDIO_SIMPLE_DEC_TYPE_AAC, &dec_cfg, &dec_info));

    // Verify the decoder results
    TEST_ASSERT_EQUAL_INT(write_ctx.decode_size, read_ctx.read_size);
    bool info_same = (memcmp(&dec_info, &aud_info, sizeof(audio_info_t)) == 0);
    TEST_ASSERT_TRUE(info_same);
    // Clear up resources
    free(write_ctx.data);
    free(read_ctx.data);
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}
