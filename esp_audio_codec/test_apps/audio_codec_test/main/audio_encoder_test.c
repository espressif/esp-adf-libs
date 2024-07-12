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
#include "esp_log.h"
#include "esp_system.h"
#include "audio_codec_test.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_enc.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG "DEC_TEST"

#define MAX_ENCODED_FRAMES (20)
typedef union {
    esp_aac_enc_config_t   aac_cfg;
    esp_alac_enc_config_t  alac_cfg;
    esp_adpcm_enc_config_t adpcm_cfg;
    esp_amrnb_enc_config_t amrnb_cfg;
    esp_amrwb_enc_config_t amrwb_cfg;
    esp_g711_enc_config_t  g711_cfg;
    esp_opus_enc_config_t  opus_cfg;
} enc_all_cfg_t;

#define ASSIGN_BASIC_CFG(cfg) {                   \
    cfg->sample_rate    = info->sample_rate;      \
    cfg->bits_per_sample = info->bits_per_sample; \
    cfg->channel        = info->channel;          \
}

static int get_encoder_config(esp_audio_type_t type, esp_audio_enc_config_t *enc_cfg, audio_info_t *info)
{
    enc_all_cfg_t *all_cfg = (enc_all_cfg_t *)(enc_cfg->cfg);
    switch (type) {
        case ESP_AUDIO_TYPE_AAC: {
            esp_aac_enc_config_t *cfg = &all_cfg->aac_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_aac_enc_config_t);
            cfg->bitrate = 90000;
            cfg->adts_used = true;
            break;
        }
        case ESP_AUDIO_TYPE_ADPCM: {
            esp_adpcm_enc_config_t *cfg = &all_cfg->adpcm_cfg;
            enc_cfg->cfg_sz = sizeof(esp_adpcm_enc_config_t);
            ASSIGN_BASIC_CFG(cfg);
            break;
        }
        case ESP_AUDIO_TYPE_AMRNB: {
            esp_amrnb_enc_config_t *cfg = &all_cfg->amrnb_cfg;
            enc_cfg->cfg_sz = sizeof(esp_amrnb_enc_config_t);
            ASSIGN_BASIC_CFG(cfg);
            cfg->bitrate_mode = ESP_AMRNB_ENC_BITRATE_MR122;
            cfg->no_file_header = true;
            break;
        }
        case ESP_AUDIO_TYPE_AMRWB: {
            esp_amrwb_enc_config_t *cfg = &all_cfg->amrwb_cfg;
            enc_cfg->cfg_sz = sizeof(esp_amrwb_enc_config_t);
            ASSIGN_BASIC_CFG(cfg);
            cfg->bitrate_mode = ESP_AMRWB_ENC_BITRATE_MD885;
            cfg->no_file_header = true;
        } break;
        case ESP_AUDIO_TYPE_G711A:
        case ESP_AUDIO_TYPE_G711U: {
            esp_g711_enc_config_t *cfg = &all_cfg->g711_cfg;
            enc_cfg->cfg_sz = sizeof(esp_g711_enc_config_t);
            ASSIGN_BASIC_CFG(cfg);
            break;
        }
        case ESP_AUDIO_TYPE_OPUS: {
            esp_opus_enc_config_t *cfg = &all_cfg->opus_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_opus_enc_config_t);
            cfg->bitrate = 90000;
            cfg->frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS;
            cfg->application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO;
            break;
        }
        case ESP_AUDIO_TYPE_ALAC: {
            esp_alac_enc_config_t *cfg = &all_cfg->alac_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_alac_enc_config_t);
            break;
        }
        default:
            ESP_LOGE(TAG, "Not supported encoder type %d", type);
            return -1;
    }
    return 0;
}

int audio_encoder_test(esp_audio_type_t type, audio_codec_test_cfg_t *data_cfg, audio_info_t *info)
{
    // Register all encoder
    esp_audio_enc_register_default();

    enc_all_cfg_t all_cfg = { 0 };
    esp_audio_enc_config_t enc_cfg = {
        .type = type,
        .cfg = &all_cfg,
    };
    esp_audio_enc_handle_t encoder = NULL;
    uint8_t *read_buf = NULL;
    uint8_t *write_buf = NULL;
    int ret = 0;
    int heap_start_size = esp_get_free_heap_size();
    int open_consumed_size = 0;
    int cur_heap_size = 0;
    do {
        // Get encoder configuration
        if (get_encoder_config(type, &enc_cfg, info) != 0) {
            ret = ESP_AUDIO_ERR_NOT_SUPPORT;
            ESP_LOGE(TAG, "Fail to get decoder info");
            break;
        }
        // Open encoder
        ret = esp_audio_enc_open(&enc_cfg, &encoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGE(TAG, "Fail to open encoder ret: %d", ret);
            break;
        }
        cur_heap_size = esp_get_free_heap_size();
        if (heap_start_size > cur_heap_size) {
            open_consumed_size = heap_start_size - cur_heap_size;
        }
        esp_audio_enc_info_t enc_info = { 0 };
        esp_audio_enc_get_info(encoder, &enc_info);
        if (enc_info.spec_info_len) {
            info->spec_info_size = enc_info.spec_info_len;
            info->spec_info = enc_info.codec_spec_info;
        }
        int read_size = 0;
        int out_size = 0;
        int frame_size = (info->bits_per_sample * info->channel) >> 3;
        // Get frame_size
        esp_audio_enc_get_frame_size(encoder, &read_size, &out_size);
        if (frame_size == read_size) {
            read_size *= 256;
            out_size *= 256;
        }
        read_buf = malloc(read_size);
        write_buf = malloc(out_size);
        if (read_buf == NULL || write_buf == NULL) {
            break;
        }
        esp_audio_enc_in_frame_t in_frame = {
            .buffer = read_buf,
            .len = read_size,
        };
        esp_audio_enc_out_frame_t out_frame = {
            .buffer = write_buf,
            .len = out_size,
        };
        uint64_t encode_time = 0;
        uint32_t total_pcm_size = 0;
        uint32_t total_encoded_size = 0;
        int max_process_heap = 0;
        while (1) {
            // Read PCM data
            if (data_cfg->read(read_buf, read_size) == 0) {
                break;
            }
            uint64_t start = esp_timer_get_time();
            heap_start_size = esp_get_free_heap_size();
            ret = esp_audio_enc_process(encoder, &in_frame, &out_frame);
            cur_heap_size = esp_get_free_heap_size();
            if (cur_heap_size < heap_start_size) {
                int process_consumed = heap_start_size - cur_heap_size;
                if (process_consumed > max_process_heap) {
                    max_process_heap = process_consumed;
                }
            }
            encode_time += esp_timer_get_time() - start;
            if (ret != ESP_AUDIO_ERR_OK) {
                ESP_LOGE(TAG, "Fail to encoder data ret %d", ret);
                break;
            }
            // Write encoded data
            data_cfg->write(write_buf, out_frame.encoded_bytes);
            total_pcm_size += read_size;
            total_encoded_size += out_frame.encoded_bytes;
        }
        if (ret == ESP_AUDIO_ERR_OK) {
            // Calculate performance
            float cpu_usage = (float)encode_time / 10 / out_frame.pts;
            ESP_LOGI(TAG, "Encoder for %s cpu: %.2f%%", esp_audio_codec_get_name(type), cpu_usage);
            float compress_ratio = (float)total_encoded_size * 100 / total_pcm_size;
            ESP_LOGI(TAG, "Encoder %s compress ratio: %.2f%% heap usage: %d", esp_audio_codec_get_name(type), compress_ratio,
                     open_consumed_size + max_process_heap);
        }

    } while (0);
    if (encoder) {
        esp_audio_enc_close(encoder);
    }
    if (read_buf) {
        free(read_buf);
    }
    if (write_buf) {
        free(write_buf);
    }
    esp_audio_enc_unregister_default();
    return ret;
}

TEST_CASE("AAC Encoder use Common API", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();
    // Register AAC encoder
    TEST_ESP_OK(esp_aac_enc_register());
    // Configuration for AAC encoder
    esp_aac_enc_config_t aac_cfg = ESP_AAC_ENC_CONFIG_DEFAULT();
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_AAC,
        .cfg = &aac_cfg,
        .cfg_sz = sizeof(aac_cfg)
    };
    // Open encoder
    esp_audio_enc_handle_t encoder = NULL;
    TEST_ESP_OK(esp_audio_enc_open(&enc_cfg, &encoder));

    // Get needed buffer size and prepare memory
    int pcm_size = 0, raw_size = 0;
    esp_audio_enc_get_frame_size(encoder, &pcm_size, &raw_size);
    TEST_ASSERT_GREATER_THAN(0, pcm_size);
    TEST_ASSERT_GREATER_THAN(0, raw_size);
    uint8_t *pcm_data = malloc(MAX_ENCODED_FRAMES * pcm_size);
    uint8_t *raw_data = malloc(raw_size);
    TEST_ASSERT_NOT_NULL(pcm_data);
    TEST_ASSERT_NOT_NULL(raw_data);
    // Generate test pcm data
    audio_info_t aud_info = {
        .sample_rate = aac_cfg.sample_rate,
        .bits_per_sample = aac_cfg.bits_per_sample,
        .channel = aac_cfg.channel,
    };
    audio_codec_gen_pcm(&aud_info, pcm_data, MAX_ENCODED_FRAMES * pcm_size);

    // Do encoding
    for (int i = 0; i < MAX_ENCODED_FRAMES; i++) {
        esp_audio_enc_in_frame_t in_frame = {
            .buffer = pcm_data + pcm_size * i,
            .len = pcm_size,
        };
        esp_audio_enc_out_frame_t out_frame = {
            .buffer = raw_data,
            .len = raw_size,
        };
        TEST_ESP_OK(esp_audio_enc_process(encoder, &in_frame, &out_frame));
        TEST_ASSERT_GREATER_THAN(0, out_frame.encoded_bytes);
    }
    // Clearup resources
    esp_audio_enc_close(encoder);
    free(pcm_data);
    free(raw_data);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_AAC);
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("AAC Encoder use Encoder API directly", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    esp_aac_enc_config_t aac_cfg = ESP_AAC_ENC_CONFIG_DEFAULT();
    esp_audio_enc_handle_t encoder = NULL;
    TEST_ESP_OK(esp_aac_enc_open(&aac_cfg, sizeof(esp_aac_enc_config_t), &encoder));

    // Get needed buffer size and prepare memory
    int pcm_size = 0, raw_size = 0;
    esp_aac_enc_get_frame_size(encoder, &pcm_size, &raw_size);
    TEST_ASSERT_GREATER_THAN(0, pcm_size);
    TEST_ASSERT_GREATER_THAN(0, raw_size);
    uint8_t *pcm_data = malloc(MAX_ENCODED_FRAMES * pcm_size);
    uint8_t *raw_data = malloc(raw_size);
    TEST_ASSERT_NOT_NULL(pcm_data);
    TEST_ASSERT_NOT_NULL(raw_data);

    // Generate test pcm data
    audio_info_t aud_info = {
        .sample_rate = aac_cfg.sample_rate,
        .bits_per_sample = aac_cfg.bits_per_sample,
        .channel = aac_cfg.channel,
    };
    audio_codec_gen_pcm(&aud_info, pcm_data, MAX_ENCODED_FRAMES * pcm_size);

    // Do encoding
    for (int i = 0; i < MAX_ENCODED_FRAMES; i++) {
        esp_audio_enc_in_frame_t in_frame = {
            .buffer = pcm_data + pcm_size * i,
            .len = pcm_size,
        };
        esp_audio_enc_out_frame_t out_frame = {
            .buffer = raw_data,
            .len = raw_size,
        };
        TEST_ESP_OK(esp_aac_enc_process(encoder, &in_frame, &out_frame));
        TEST_ASSERT_GREATER_THAN(0, out_frame.encoded_bytes);
    }

    // Clear up resources
    esp_aac_enc_close(encoder);
    free(pcm_data);
    free(raw_data);
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}
