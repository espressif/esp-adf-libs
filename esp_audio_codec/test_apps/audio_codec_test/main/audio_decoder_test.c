/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_err.h"
#include "esp_system.h"
#include "audio_codec_test.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "DEC_TEST"

typedef union {
    esp_opus_dec_cfg_t  opus_cfg;
    esp_adpcm_dec_cfg_t adpcm_cfg;
    esp_alac_dec_cfg_t  alac_cfg;
    esp_aac_dec_cfg_t   aac_cfg;
    esp_g711_dec_cfg_t  g711_cfg;
} dec_all_cfg_t;

typedef struct {
    uint8_t *data;
    int      read_size;
    int      size;
} read_ctx_t;

typedef struct {
    bool                      use_common_api;
    esp_audio_dec_handle_t    decoder;
    esp_audio_dec_out_frame_t out_frame;
    bool                      decode_err;
} write_ctx_t;

static read_ctx_t  read_ctx;
static write_ctx_t write_ctx;

static int decode_one_frame(uint8_t *data, int size);

static int get_decoder_cfg(esp_audio_dec_cfg_t *dec_cfg, audio_info_t *info)
{
    dec_all_cfg_t *cfg = (dec_all_cfg_t *)dec_cfg->cfg;
    switch (dec_cfg->type) {
        case ESP_AUDIO_TYPE_OPUS: {
            esp_opus_dec_cfg_t *opus_cfg = &cfg->opus_cfg;
            opus_cfg->sample_rate = info->sample_rate;
            opus_cfg->channel = info->channel;
            dec_cfg->cfg = opus_cfg;
            dec_cfg->cfg_sz = sizeof(esp_opus_dec_cfg_t);
        } break;
        case ESP_AUDIO_TYPE_ADPCM: {
            esp_adpcm_dec_cfg_t *adpcm_cfg = &cfg->adpcm_cfg;
            adpcm_cfg->sample_rate = info->sample_rate;
            adpcm_cfg->channel = info->channel;
            adpcm_cfg->bits_per_sample = 4, dec_cfg->cfg = adpcm_cfg;
            dec_cfg->cfg_sz = sizeof(esp_adpcm_dec_cfg_t);
        } break;
        case ESP_AUDIO_TYPE_G711A:
        case ESP_AUDIO_TYPE_G711U: {
            esp_g711_dec_cfg_t *g711_cfg = &cfg->g711_cfg;
            g711_cfg->channel = info->channel;
            dec_cfg->cfg = g711_cfg;
            dec_cfg->cfg_sz = sizeof(esp_g711_dec_cfg_t);
        } break;
        case ESP_AUDIO_TYPE_ALAC: {
            esp_alac_dec_cfg_t *alac_cfg = &cfg->alac_cfg;
            alac_cfg->codec_spec_info = (uint8_t *)info->spec_info;
            alac_cfg->spec_info_len = info->spec_info_size;
            dec_cfg->cfg = alac_cfg;
            dec_cfg->cfg_sz = sizeof(esp_alac_dec_cfg_t);
        } break;
        case ESP_AUDIO_TYPE_AAC: {
            esp_aac_dec_cfg_t *aac_cfg = &cfg->aac_cfg;
            aac_cfg->aac_plus_enable = true;
            dec_cfg->cfg = aac_cfg;
            dec_cfg->cfg_sz = sizeof(esp_aac_dec_cfg_t);
        } break;
        default:
            dec_cfg->cfg = NULL;
            dec_cfg->cfg_sz = 0;
            break;
    }
    return 0;
}

int audio_decoder_test(esp_audio_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info)
{
    dec_all_cfg_t all_cfg = {};
    esp_audio_dec_register_default();
    esp_audio_dec_cfg_t dec_cfg = {
        .type = type,
        .cfg = &all_cfg,
    };
    get_decoder_cfg(&dec_cfg, info);
    esp_audio_dec_handle_t decoder = NULL;
    int max_raw_size = 10*1024;
    int max_out_size = 4096;
    uint8_t *raw_buf = malloc(max_raw_size);
    uint8_t *out_buf = malloc(max_out_size);
    int ret = 0;
    int heap_start_size = esp_get_free_heap_size();
    int open_consumed_size = 0;
    int cur_heap_size = 0;
    do {
        if (raw_buf == NULL || out_buf == NULL) {
            ESP_LOGI(TAG, "No memory for decoder");
            ret = ESP_AUDIO_ERR_MEM_LACK;
            break;
        }
        ret = esp_audio_dec_open(&dec_cfg, &decoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGI(TAG, "Fail to open decoder ret %d", ret);
            break;
        }
        cur_heap_size = esp_get_free_heap_size();
        if (cur_heap_size < heap_start_size) {
            open_consumed_size = heap_start_size - cur_heap_size;
        }
        int total_decoded = 0;
        int process_max_consume = 0;
        uint64_t decode_time = 0;
        while (ret == ESP_AUDIO_ERR_OK) {
            ret = cfg->read(raw_buf, max_raw_size);
            if (ret <= 4) {
                break;
            }
            esp_audio_dec_in_raw_t raw = {
                .buffer = raw_buf,
                .len = ret,
            };
            esp_audio_dec_out_frame_t out_frame = {
                .buffer = out_buf,
                .len = max_out_size,
            };
            while (raw.len) {
                uint64_t start = esp_timer_get_time();
                heap_start_size = esp_get_free_heap_size();
                ret = esp_audio_dec_process(decoder, &raw, &out_frame);
                decode_time += esp_timer_get_time() - start;
                cur_heap_size = esp_get_free_heap_size();
                if (cur_heap_size < heap_start_size) {
                    int process_consumed = heap_start_size - cur_heap_size;
                    if (process_consumed > process_max_consume) {
                        process_max_consume = process_consumed;
                    }
                }
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
                total_decoded += out_frame.decoded_size;
                if (cfg->write) {
                    cfg->write(out_frame.buffer, out_frame.decoded_size);
                }
                // In case that input data contain multiple frames
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
        }
        if (total_decoded) {
            int sample_size = info->channel * info->bits_per_sample >> 3;
            float cpu_usage = (float)decode_time * sample_size * info->sample_rate / total_decoded / 10000;
            ESP_LOGI(TAG, "Decode for %s cpu: %.2f%% heap usage: %d", esp_audio_codec_get_name(type), cpu_usage, 
                     open_consumed_size + process_max_consume);
        }
    } while (0);
    esp_audio_dec_close(decoder);
    esp_audio_dec_unregister_default();
    if (raw_buf) {
        free(raw_buf);
    }
    if (out_buf) {
        free(out_buf);
    }
    return ret;
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

static int get_encode_data(esp_aac_dec_cfg_t *aac_cfg)
{
    memset(&read_ctx, 0, sizeof(read_ctx));
    audio_info_t aud_info = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 2,
    };

    // Generate PCM data to be encoded
    int pcm_size = 50 * 1024;
    read_ctx.data = malloc(pcm_size);
    if (read_ctx.data == NULL) {
        return -1;
    }
    read_ctx.size = audio_codec_gen_pcm(&aud_info, read_ctx.data, pcm_size);
    read_ctx.read_size = 0;

    // Encode AAC, encoded data will be sent out through callback `decode_one_frame`
    audio_codec_test_cfg_t enc_cfg = {
        .read = encoder_read_pcm,
        .write = decode_one_frame,
    };
    audio_encoder_test(ESP_AUDIO_TYPE_AAC, &enc_cfg, &aud_info);
    free(read_ctx.data);
    return 0;
}

static int decode_one_frame(uint8_t *data, int size)
{
    if (write_ctx.decode_err) {
        return 0;
    }
    esp_audio_dec_in_raw_t raw = {
        .buffer = data,
        .len = size,
    };
    esp_audio_dec_out_frame_t *out_frame = &write_ctx.out_frame;
    int ret = 0;
    // Input data may contain multiple frames, each call of process decode only one frame
    while (raw.len) {
        if (write_ctx.use_common_api) {
            ret = esp_audio_dec_process(write_ctx.decoder, &raw, out_frame);
        } else {
            esp_audio_dec_info_t aud_info;
            ret = esp_aac_dec_decode(write_ctx.decoder, &raw, out_frame, &aud_info);
        }
        if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
            // When output buffer for pcm is not enough, need reallocate it according reported `needed_size` and retry
            uint8_t *new_buf = realloc(out_frame->buffer, out_frame->needed_size);
            if (new_buf == NULL) {
                break;
            }
            out_frame->buffer = new_buf;
            out_frame->len = out_frame->needed_size;
            continue;
        }
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGE(TAG, "Fail to decode data ret %d", ret);
            write_ctx.decode_err = true;
            break;
        }
        raw.len -= raw.consumed;
        raw.buffer += raw.consumed;
    }
    return 0;
}

TEST_CASE("AAC decoder use Common API", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    // Register AAC decoder, or you can call `esp_audio_dec_register_default` to register all supported decoder
    TEST_ESP_OK(esp_aac_dec_register());

    // Configuration for AAC decoder
    esp_aac_dec_cfg_t aac_cfg = { 0 };
    memset(&write_ctx, 0, sizeof(write_ctx));
    write_ctx.use_common_api = true;
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_AAC,
        .cfg = &aac_cfg,
        .cfg_sz = sizeof(aac_cfg),
    };

    // Open decoder
    TEST_ESP_OK(esp_audio_dec_open(&dec_cfg, &write_ctx.decoder));

    // Allocate buffer to hold output PCM data
    write_ctx.out_frame.len = 4096;
    write_ctx.out_frame.buffer = malloc(write_ctx.out_frame.len);
    TEST_ASSERT_NOT_NULL(write_ctx.out_frame.buffer);

    // Loop to get encoder data and call `decode_one_frame` to decode one frame of encoded AAC data
    get_encode_data(&aac_cfg);

    // Verify no decode error happen
    TEST_ASSERT_TRUE(write_ctx.decode_err == false);

    // Clear up resources
    esp_audio_dec_close(write_ctx.decoder);
    free(write_ctx.out_frame.buffer);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_AAC);
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("AAC decoder use Decoder API directly", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    // Configuration for AAC decoder
    esp_aac_dec_cfg_t aac_cfg = { 0 };
    memset(&write_ctx, 0, sizeof(write_ctx));

    // Open AAC encoder
    TEST_ESP_OK(esp_aac_dec_open(&aac_cfg, sizeof(aac_cfg), &write_ctx.decoder));

    // Allocate buffer to hold output PCM data
    write_ctx.out_frame.len = 4096;
    write_ctx.out_frame.buffer = malloc(write_ctx.out_frame.len);
    TEST_ASSERT_NOT_NULL(write_ctx.out_frame.buffer);

    // Loop to get encoder data, see `decode_one_frame` for how to decode one frame of encoded data
    get_encode_data(&aac_cfg);

    // Verify no decode error happen
    TEST_ASSERT_TRUE(write_ctx.decode_err == false);

    // Clear up resources
    esp_aac_dec_close(write_ctx.decoder);
    free(write_ctx.out_frame.buffer);

    // Verify no leakage happen
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("Decoder query type", CODEC_TEST_MODULE_NAME)
{
    TEST_ASSERT_EQUAL(esp_audio_dec_check_audio_type(ESP_AUDIO_TYPE_AMRNB), ESP_AUDIO_ERR_NOT_SUPPORT);
    esp_audio_dec_register_default();
    TEST_ASSERT_EQUAL(esp_audio_dec_check_audio_type(ESP_AUDIO_TYPE_AMRNB), ESP_AUDIO_ERR_OK);
    esp_audio_dec_unregister_default();
}
