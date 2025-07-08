/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
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
    esp_sbc_dec_cfg_t   sbc_cfg;
    esp_lc3_dec_cfg_t   lc3_cfg;
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
        case ESP_AUDIO_TYPE_SBC: {
            esp_sbc_dec_cfg_t *sbc_cfg = &cfg->sbc_cfg;
            sbc_cfg->sbc_mode = ESP_SBC_MODE_STD;
            sbc_cfg->ch_num = 2;
            sbc_cfg->enable_plc = false;
            dec_cfg->cfg = sbc_cfg;
            dec_cfg->cfg_sz = sizeof(esp_sbc_dec_cfg_t);
        } break;
        case ESP_AUDIO_TYPE_LC3: {
            esp_lc3_dec_cfg_t *lc3_cfg = &cfg->lc3_cfg;
            lc3_cfg->sample_rate = 48000;
            lc3_cfg->channel = 2;
            lc3_cfg->bits_per_sample = 16;
            lc3_cfg->frame_dms = 100;
            lc3_cfg->nbyte = 120;
            lc3_cfg->is_cbr = true;
            lc3_cfg->len_prefixed = false;
            lc3_cfg->enable_plc = false;
            dec_cfg->cfg = lc3_cfg;
            dec_cfg->cfg_sz = sizeof(esp_lc3_dec_cfg_t);
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
    int max_raw_size = 10 * 1024;
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
    esp_aac_dec_cfg_t aac_cfg = {0};
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
    esp_aac_dec_cfg_t aac_cfg = {0};
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

TEST_CASE("Decoder opus", CODEC_TEST_MODULE_NAME)
{
    // ==================== Test Data Definition ====================
    
    // Single frame Opus data (self_delimited = false)
    uint8_t opus_1frame_s0[] = {
        0x58,0x62,0xf9,0x1a,0x1a,0x15,0xd0,0xdd,0x80,0x04,0xa2,0xf6,0x17,0x19,0xc9,0x31,0xab,0x37,0xb9,
        0x32,0x60,0xf1,0xea,0x77,0x9d,0xf3,0xb8,0xf4,0x30,0xe0,0xd3,0x02,0x6c,0x32,0xf6,0x12,0xb5,0x5c,
        0xed,0x11,0x47,0xca,0xc4,0x82,0xfb,0x2c,0xed,0x1a,0x1d,0x45,0x2a,0x4a,0xb1,0xc9,0x53,0xa7,0x8b,
        0x96,0x5e,0x42,0xc6,0x8b,0x4d,0xd7,0xaa,0xdc,0x99,0xc7,0x54,0x11,0xe4,0x2f,0xe8,
    };
    size_t opus_1frame_s0_len = sizeof(opus_1frame_s0);
    
    // Double frame Opus data (self_delimited = false)
    uint8_t opus_2frame_s0[] = {
        0x58,0x62,0xf9,0x1a,0x1a,0x15,0xd0,0xdd,0x80,0x04,0xa2,0xf6,0x17,0x19,0xc9,0x31,0xab,0x37,0xb9,
        0x32,0x60,0xf1,0xea,0x77,0x9d,0xf3,0xb8,0xf4,0x30,0xe0,0xd3,0x02,0x6c,0x32,0xf6,0x12,0xb5,0x5c,
        0xed,0x11,0x47,0xca,0xc4,0x82,0xfb,0x2c,0xed,0x1a,0x1d,0x45,0x2a,0x4a,0xb1,0xc9,0x53,0xa7,0x8b,
        0x96,0x5e,0x42,0xc6,0x8b,0x4d,0xd7,0xaa,0xdc,0x99,0xc7,0x54,0x11,0xe4,0x2f,0xe8,
        0x58,0xe6,0xb1,0x9e,0x7a,0x1c,0xbf,0xde,0xd6,0x7a,0xc7,0x4a,0x4d,0xa9,0xae,0xec,0x91,0x9b,0x3e,
        0xc5,0x95,0x5e,0x8f,0x0b,0x35,0x57,0x5c,0x23,0x30,0x39,0x3c,0x99,0x56,0x7a,0xfb,0xb1,0xea,0xf6,
        0x0f,0xda,0x1a,0xf4,0xdd,0x74,0x40,0x77,0x4c,0x3f,0xed,0x70,0xd3,0xb9,0x27,0x81,0x7d,0xbe,0x6d,
        0xae,0x61,0xc7,0xb0,0x53,0x84,0x04,0xd4,0x03,0x9a,0xe7,0x86,0x65,0xc9,0x60,0x7e,0x77,0xde,0xc9,
        0xf7,0x29,0x89,0xe4,0x42,0xab,0x32,0xa5,0xa1,0xa6,0xa1,0x9f,0x41,0x5b,0x1a,0xa2,0x60,0x1e,0xf4,
        0x20,0xb3,0x37,0x60,0xd4,0x0b,0x01,0x5f,0x6d,0x66,0x1f,0xd3,0xab,0x38,0xb2,0xba,0xa0,0x8e,0xd4,
        0x68,0x93,0x56,0xa3,0x3f,0x30,0x5a,0xea,0x71,0xab,0x18,
    };
    size_t opus_2frame_s0_len = sizeof(opus_2frame_s0);
   
    // Single frame Opus data (self_delimited = true)
    uint8_t opus_1frame_s1[] = {
        0x58,0x48,0x62,0xf9,0x1a,0x1a,0x15,0xd0,0xdd,0x80,0x04,0xa2,0xf6,
        0x17,0x19,0xc9,0x31,0xab,0x37,0xb9,0x32,0x60,0xf1,0xea,0x77,0x9d,0xf3,0xb8,0xf4,0x30,0xe0,0xd3,
        0x02,0x6c,0x32,0xf6,0x12,0xb5,0x5c,0xed,0x11,0x47,0xca,0xc4,0x82,0xfb,0x2c,0xed,0x1a,0x1d,0x45,
        0x2a,0x4a,0xb1,0xc9,0x53,0xa7,0x8b,0x96,0x5e,0x42,0xc6,0x8b,0x4d,0xd7,0xaa,0xdc,0x99,0xc7,0x54,
        0x11,0xe4,0x2f,0xe8,
    };
    size_t opus_1frame_s1_len = sizeof(opus_1frame_s1);
    
    // Double frame Opus data (self_delimited = true)
    uint8_t opus_2frame_s1[] = {
        0x58,0x48,0x62,0xf9,0x1a,0x1a,0x15,0xd0,0xdd,0x80,0x04,0xa2,0xf6,0x17,0x19,0xc9,0x31,0xab,0x37,
        0xb9,0x32,0x60,0xf1,0xea,0x77,0x9d,0xf3,0xb8,0xf4,0x30,0xe0,0xd3,0x02,0x6c,0x32,0xf6,0x12,0xb5,
        0x5c,0xed,0x11,0x47,0xca,0xc4,0x82,0xfb,0x2c,0xed,0x1a,0x1d,0x45,0x2a,0x4a,0xb1,0xc9,0x53,0xa7,
        0x8b,0x96,0x5e,0x42,0xc6,0x8b,0x4d,0xd7,0xaa,0xdc,0x99,0xc7,0x54,0x11,0xe4,0x2f,0xe8,
        0x58,0x7c,0xe6,0xb1,0x9e,0x7a,0x1c,0xbf,0xde,0xd6,0x7a,0xc7,0x4a,0x4d,0xa9,0xae,0xec,0x91,0x9b,
        0x3e,0xc5,0x95,0x5e,0x8f,0x0b,0x35,0x57,0x5c,0x23,0x30,0x39,0x3c,0x99,0x56,0x7a,0xfb,0xb1,0xea,
        0xf6,0x0f,0xda,0x1a,0xf4,0xdd,0x74,0x40,0x77,0x4c,0x3f,0xed,0x70,0xd3,0xb9,0x27,0x81,0x7d,0xbe,
        0x6d,0xae,0x61,0xc7,0xb0,0x53,0x84,0x04,0xd4,0x03,0x9a,0xe7,0x86,0x65,0xc9,0x60,0x7e,0x77,0xde,
        0xc9,0xf7,0x29,0x89,0xe4,0x42,0xab,0x32,0xa5,0xa1,0xa6,0xa1,0x9f,0x41,0x5b,0x1a,0xa2,0x60,0x1e,
        0xf4,0x20,0xb3,0x37,0x60,0xd4,0x0b,0x01,0x5f,0x6d,0x66,0x1f,0xd3,0xab,0x38,0xb2,0xba,0xa0,0x8e,
        0xd4,0x68,0x93,0x56,0xa3,0x3f,0x30,0x5a,0xea,0x71,0xab,0x18,
    };
    size_t opus_2frame_s1_len = sizeof(opus_2frame_s1);

    // ==================== Decoder Configuration ====================
    esp_opus_dec_cfg_t opus_cfg = ESP_OPUS_DEC_CONFIG_DEFAULT();
    opus_cfg.frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS;  // 60ms frame duration
    opus_cfg.sample_rate = ESP_AUDIO_SAMPLE_RATE_16K;             // 16kHz sample rate
    opus_cfg.channel = ESP_AUDIO_MONO;                            // Mono channel
    
    // ==================== Decoder Handle and Buffers ====================
    esp_audio_dec_handle_t dec_handle = NULL;
    esp_audio_dec_info_t info = {0};
    
    // Input buffer
    esp_audio_dec_in_raw_t raw = {
        .buffer = NULL,
        .len = 0,
        .consumed = 0,
    };
    
    // Output buffer (allocate enough space for double packet)
    esp_audio_dec_out_frame_t out_frame = {
        .buffer = malloc(opus_cfg.sample_rate * opus_cfg.channel * 60 * 2 * 2 / 1000), // Double packet size
        .len = opus_cfg.sample_rate * opus_cfg.channel * 60 * 2 / 1000,                // Single packet size
        .needed_size = 0,
    };
    TEST_ASSERT_NOT_NULL(out_frame.buffer);

    // ==================== Test Scenario 1: self_delimited = false ====================
    printf("\n=== Test Scenario 1: self_delimited = false ===\n");
    
    opus_cfg.self_delimited = false;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_open((void *)&opus_cfg, sizeof(opus_cfg), &dec_handle));
    TEST_ASSERT_NOT_NULL(dec_handle);

    // Test single frame decoding
    printf("Test single frame decoding (self_delimited = false)\n");
    raw.buffer = opus_1frame_s0;
    raw.len = opus_1frame_s0_len;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_1frame_s0_len);

    // Test double frame decoding (two separate calls)
    printf("Test double frame decoding (self_delimited = false) - two separate calls\n");
    raw.buffer = opus_2frame_s0;
    raw.len = opus_1frame_s0_len;  // Decode first frame only
    raw.consumed = 0;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_1frame_s0_len);
    
    raw.buffer = opus_2frame_s0 + opus_1frame_s0_len;  // Point to second frame
    raw.len = opus_2frame_s0_len - opus_1frame_s0_len; // Remaining length
    raw.consumed = 0;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_2frame_s0_len - opus_1frame_s0_len);

    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_close(dec_handle));

    // ==================== Test Scenario 2: self_delimited = true ====================
    printf("\n=== Test Scenario 2: self_delimited = true ===\n");
    
    opus_cfg.self_delimited = true;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_open((void *)&opus_cfg, sizeof(opus_cfg), &dec_handle));
    TEST_ASSERT_NOT_NULL(dec_handle);

    // Test single frame decoding
    printf("Test single frame decoding (self_delimited = true)\n");
    raw.buffer = opus_1frame_s1;
    raw.len = opus_1frame_s1_len;
    raw.consumed = 0;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_1frame_s1_len);

    // Test double frame decoding (one-time decoding)
    printf("Test double frame decoding (self_delimited = true) - one-time decoding\n");
    raw.buffer = opus_2frame_s1;
    raw.len = opus_2frame_s1_len;
    raw.consumed = 0;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_1frame_s1_len);  // Decode first frame
    
    raw.buffer = opus_2frame_s1 + raw.consumed;  // Point to second frame
    raw.len = opus_2frame_s1_len - opus_1frame_s1_len;  // Remaining length
    raw.consumed = 0;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    TEST_ASSERT_EQUAL(raw.consumed, opus_2frame_s1_len - opus_1frame_s1_len);

    // ==================== Test Scenario 3: Buffer Insufficient Test ====================
    printf("\n=== Test Scenario 3: Buffer Insufficient Test ===\n");
    
    // Test 1: Configure 60ms frame duration, but output buffer is only 20ms size
    printf("Test buffer insufficient: Configure 60ms frame duration, but output buffer is only 20ms size\n");
    out_frame.len = opus_cfg.sample_rate * opus_cfg.channel * 20 * 2 / 1000;  // 20ms buffer
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_BUFF_NOT_ENOUGH, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));

    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_close(dec_handle));

    // ==================== Test Scenario 4: Frame Duration Mismatch Test ====================
    printf("\n=== Test Scenario 4: Frame Duration Mismatch Test ===\n");
    
    // Configure 20ms frame duration, but data is 60ms
    opus_cfg.frame_duration = ESP_OPUS_DEC_FRAME_DURATION_20_MS;
    opus_cfg.self_delimited = false;
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_open((void *)&opus_cfg, sizeof(opus_cfg), &dec_handle));
    TEST_ASSERT_NOT_NULL(dec_handle);

    printf("Test frame duration mismatch: Configure 20ms frame duration, but encoded data is 60ms\n");
    raw.buffer = opus_1frame_s0;
    raw.len = opus_1frame_s0_len;
    out_frame.len = opus_cfg.sample_rate * opus_cfg.channel * 20 * 2 / 1000;  // 20ms buffer
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_BUFF_NOT_ENOUGH, esp_opus_dec_decode(dec_handle, &raw, &out_frame, &info));
    
    TEST_ASSERT_NOT_NULL(dec_handle);
    TEST_ASSERT_EQUAL(ESP_AUDIO_ERR_OK, esp_opus_dec_close(dec_handle));
    
    // ==================== Cleanup Resources ====================
    free(out_frame.buffer);
}
