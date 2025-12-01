/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "audio_codec_test.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_reg.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_simple_dec_default.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_log.h"

#define TAG "SIMP_DEC_CUST"

#define READ_BLOCK_SIZE  (512)

typedef struct {
    int dec_frame_num;
    int err_frame_num;
} fake_mp3_dec_t;

static int mp3_parse_count = 0;
static fake_mp3_dec_t fake_mp3_dec;

extern const char test_mp3_start[] asm("_binary_test_mp3_start");
extern const char test_mp3_end[] asm("_binary_test_mp3_end");

static bool get_mp3_version(uint8_t version_idx, uint8_t *version)
{
    if (version_idx == 0b11) {
        *version = 0;
    } else if (version_idx == 0b10) {
        *version = 1;
    } else if (version_idx == 0b00) {
        *version = 2;
    } else {
        return false;
    }
    return true;
}

static esp_es_parse_err_t simple_mp3_parse(esp_es_parse_raw_t        *in,
                                           esp_es_parse_frame_info_t *frame_info)
{
    // Not consider ID3 header currently
    if (in->len < 4) {
        // Input length is not enough
        return ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH;
    }
    uint8_t *header = (uint8_t *)in->buffer;
    // Check for MP3 frame sync (11 bits)
    if ((header[0] != 0xFF) || ((header[1] & 0xE0) != 0xE0)) {
        // Wrong header need skip and reparse
        return ESP_ES_PARSE_ERR_WRONG_HEADER;
    }
    uint8_t version_bits = (header[1] >> 3) & 0x03;
    uint8_t layer_bits = (header[1] >> 1) & 0x03;
    uint8_t bitrate_index = (header[2] >> 4) & 0x0F;
    uint8_t sample_rate_index = (header[2] >> 2) & 0x03;
    uint8_t padding_bit = (header[2] >> 1) & 0x01;
    uint8_t version_idx;
    if (get_mp3_version(version_bits, &version_idx) == false) {
        return ESP_ES_PARSE_ERR_WRONG_HEADER;
    }
    // Check layer bits: 0b11=Layer I, 0b10=Layer II, 0b01=Layer III, 0b00=reserved
    if (layer_bits == 0b00) {
        return ESP_ES_PARSE_ERR_WRONG_HEADER;
    }
    // Convert layer_bits to layer index: Layer I(3)->0, Layer II(2)->1, Layer III(1)->2
    uint8_t layer_idx = 3 - layer_bits;
    const uint16_t bitrate_table[9][16] = {
        // MPEG-1 (version_idx = 0)
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},  // Layer I
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0}, // Layer II
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},  // Layer III
        // MPEG-2 (version_idx = 1)
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer I
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer II
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer III
        // MPEG-2.5 (version_idx = 2)
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer I
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},      // Layer II
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}       // Layer III
    };
    const uint32_t sample_rate_table[][4] = {
        // MPEG-1 (version_idx = 0)
        {44100, 48000, 32000, 0},  // Layer I, II, III
        // MPEG-2 (version_idx = 1)
        {22050, 24000, 16000, 0},  // Layer I, II, III
        // MPEG-2.5 (version_idx = 2)
        {11025, 12000, 8000, 0}    // Layer I, II, III
    };
    int sample_rate = sample_rate_table[version_idx][sample_rate_index];
    if (sample_rate == 0) {
        return ESP_ES_PARSE_ERR_WRONG_HEADER;
    }
    // Calculate table row index: version_idx * 3 + layer_idx
    int table_row = version_idx * 3 + layer_idx;
    int bitrate = bitrate_table[table_row][bitrate_index];
    if (bitrate == 0) {
        return ESP_ES_PARSE_ERR_WRONG_HEADER;
    }
    frame_info->aud_info.sample_rate = sample_rate;
    int samples_per_frame;
    // Set parsed frame size so that parser can load the whole frame
    if (layer_bits == 0b11) {  // Layer I
        samples_per_frame = 384;
        frame_info->frame_size = ((12000 * bitrate) / sample_rate + padding_bit) * 4;
    } else if (layer_bits == 0b10) {  // Layer II
        samples_per_frame = 1152;
        frame_info->frame_size = (samples_per_frame / 8 * bitrate * 1000) / sample_rate + padding_bit;
    } else {  // Layer III (0b01)
        samples_per_frame = (version_idx == 0) ? 1152 : 576;  // MPEG-1: 1152, MPEG-2/2.5: 576
        frame_info->frame_size = (samples_per_frame / 8 * bitrate * 1000) / sample_rate + padding_bit;
    }
    frame_info->aud_info.sample_num = samples_per_frame;
    frame_info->aud_info.bits_per_sample = 16;
    frame_info->aud_info.channel = 2;
    mp3_parse_count++;
    return ESP_ES_PARSE_ERR_OK;
}

static esp_audio_err_t fake_mp3_dec_open(void *cfg, uint32_t cfg_sz, void **decoder)
{
    memset(&fake_mp3_dec, 0, sizeof(fake_mp3_dec));
    *decoder = &fake_mp3_dec;
    return ESP_AUDIO_ERR_OK;
}

static esp_audio_err_t fake_mp3_dec_decode(void *decoder, esp_audio_dec_in_raw_t *raw,
                                           esp_audio_dec_out_frame_t *frame,
                                           esp_audio_dec_info_t *info)
{
    esp_es_parse_frame_info_t frame_info = {};
    fake_mp3_dec_t *mp3_dec = (fake_mp3_dec_t *)decoder;
    esp_es_parse_raw_t parse_raw = {
        .buffer = raw->buffer,
        .len = raw->len,
        .bos = false,
        .eos = false,
    };
    if (simple_mp3_parse(&parse_raw, &frame_info) != ESP_ES_PARSE_ERR_OK) {
        mp3_dec->err_frame_num++;
        return ESP_AUDIO_ERR_FAIL;
    }
    // For double parse in decoder decrease parse count here
    mp3_parse_count--;

    // Fake decode not actually do decode
    int out_frame_size = frame_info.aud_info.sample_num *
                        frame_info.aud_info.bits_per_sample * frame_info.aud_info.channel / 8;
    if (frame->len < out_frame_size) {
        frame->needed_size = out_frame_size;
        return ESP_AUDIO_ERR_BUFF_NOT_ENOUGH;
    }
    frame->decoded_size = out_frame_size;
    if (raw->len != frame_info.frame_size) {
        ESP_LOGE(TAG, "Input frame size not equal to parser");
        return ESP_AUDIO_ERR_FAIL;
    }
    raw->consumed = frame_info.frame_size;
    info->sample_rate = frame_info.aud_info.sample_rate;
    info->channel = frame_info.aud_info.channel;
    info->bits_per_sample = frame_info.aud_info.bits_per_sample;
    mp3_dec->dec_frame_num++;
    return ESP_AUDIO_ERR_OK;
}

static esp_audio_err_t fake_mp3_dec_close(void *decoder)
{
    return ESP_AUDIO_ERR_OK;
}

static int decode_mp3(void)
{
    int max_out_size = 1024;
    uint8_t *out_buf = malloc(max_out_size);
    uint8_t *in_buf = (uint8_t *)test_mp3_start;
    int in_size = (int)(test_mp3_end - test_mp3_start);
    int in_pos = 0;
    esp_audio_simple_dec_handle_t decoder = NULL;
    int ret = ESP_AUDIO_ERR_OK;
    do {
        if (out_buf == NULL) {
            ESP_LOGI(TAG, "No memory for decoder");
            ret = ESP_AUDIO_ERR_MEM_LACK;
            break;
        }
        // No special decoder configuration for MP3
        esp_audio_simple_dec_cfg_t dec_cfg = {
            .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_MP3,
        };
        ret = esp_audio_simple_dec_open(&dec_cfg, &decoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGI(TAG, "Fail to open simple decoder ret %d", ret);
            break;
        }
        esp_audio_simple_dec_raw_t raw = {};
        while (in_pos < in_size && (ret == ESP_AUDIO_ERR_OK)) {
            int block_size = in_size - in_pos;
            if (block_size > READ_BLOCK_SIZE) {
                block_size = READ_BLOCK_SIZE;
            }
            raw.buffer = in_buf + in_pos;
            raw.len = block_size;
            in_pos += block_size;
            // Not handle EOS currently

            esp_audio_simple_dec_out_t out_frame = {
                .buffer = out_buf,
                .len = max_out_size,
            };
            while (raw.len) {
                ret = esp_audio_simple_dec_process(decoder, &raw, &out_frame);
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
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
        }
    } while (0);
    esp_audio_simple_dec_close(decoder);
    if (out_buf) {
        free(out_buf);
    }
    return ret;
}

TEST_CASE("Simple Decoder with Overwrite MP3 Parser", CODEC_TEST_MODULE_NAME)
{
    esp_audio_dec_register_default();
    esp_audio_simple_dec_reg_info_t reg_info = {
        .decoder_ops = ESP_MP3_DEC_DEFAULT_OPS(),
        .parser = simple_mp3_parse,
        .free = NULL,
    };
    esp_audio_simple_dec_register(ESP_AUDIO_SIMPLE_DEC_TYPE_MP3, &reg_info);
    // Reset parse and decoder information
    mp3_parse_count = 0;
    memset(&fake_mp3_dec, 0, sizeof(fake_mp3_dec));

    TEST_ESP_OK(decode_mp3());
    // Verify actual parse and decode result
    TEST_ASSERT_TRUE(mp3_parse_count > 0);
    TEST_ASSERT_EQUAL(0, fake_mp3_dec.dec_frame_num);
    ESP_LOGI(TAG, "Parse count:%d decode count:%d", mp3_parse_count, fake_mp3_dec.dec_frame_num);
    esp_audio_simple_dec_unregister_default();
    esp_audio_dec_unregister_default();
    esp_audio_simple_dec_unregister(ESP_AUDIO_SIMPLE_DEC_TYPE_MP3);
}

TEST_CASE("Simple Decoder with Overwrite MP3 Decoder", CODEC_TEST_MODULE_NAME)
{
    esp_audio_dec_register_default();
    esp_audio_dec_ops_t dec_ops = {
        .open = fake_mp3_dec_open,
        .decode = fake_mp3_dec_decode,
        .close = fake_mp3_dec_close,
    };
    esp_audio_dec_register(ESP_AUDIO_TYPE_MP3, &dec_ops);
    // Reset parse and decoder information
    mp3_parse_count = 0;
    memset(&fake_mp3_dec, 0, sizeof(fake_mp3_dec));

    TEST_ESP_OK(decode_mp3());
    // Verify actual parse and decode result
    TEST_ASSERT_TRUE(mp3_parse_count == 0);
    TEST_ASSERT_TRUE(fake_mp3_dec.dec_frame_num > 0);
    TEST_ASSERT_EQUAL(0, fake_mp3_dec.err_frame_num);
    ESP_LOGI(TAG, "Parse count:%d decode count:%d", mp3_parse_count, fake_mp3_dec.dec_frame_num);
    esp_audio_dec_unregister_default();
}

TEST_CASE("Simple Decoder with Overwrite Both MP3 Parser and Decoder", CODEC_TEST_MODULE_NAME)
{
    esp_audio_dec_register_default();
    esp_audio_simple_dec_reg_info_t reg_info = {
        .decoder_ops =  {
            .open = fake_mp3_dec_open,
            .decode = fake_mp3_dec_decode,
            .close = fake_mp3_dec_close,
        },
        .parser = simple_mp3_parse,
    };
    esp_audio_simple_dec_register(ESP_AUDIO_SIMPLE_DEC_TYPE_MP3, &reg_info);
    // Reset parse and decoder information
    mp3_parse_count = 0;
    memset(&fake_mp3_dec, 0, sizeof(fake_mp3_dec));

    TEST_ESP_OK(decode_mp3());
    // Verify actual parse and decode result
    TEST_ASSERT_TRUE(mp3_parse_count > 0);
    TEST_ASSERT_TRUE(fake_mp3_dec.dec_frame_num > 0);
    TEST_ASSERT_EQUAL(0, fake_mp3_dec.err_frame_num);
    ESP_LOGI(TAG, "Parse count:%d decode count:%d", mp3_parse_count, fake_mp3_dec.dec_frame_num);
    esp_audio_dec_unregister_default();
    esp_audio_simple_dec_unregister(ESP_AUDIO_SIMPLE_DEC_TYPE_MP3);
}
