/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "test_common.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_data_weaver.h"
#include "ae_common.h"

#define TAG "TEST_BIT_CONVERT"

#define BIT_CVT_TEST_SAMPLES 4
#define BIT_CVT_MAX_CHANNELS 4
#define BIT_CVT_NUM_FORMATS  (sizeof(formats) / sizeof(formats[0]))
#define BIT_CVT_DATASETS     (sizeof(conversion_datasets) / sizeof(conversion_datasets[0]))

typedef struct {
    uint8_t      src_bits;
    uint8_t      dst_bits;
    const char  *desc;
    int32_t      expected_results[BIT_CVT_TEST_SAMPLES];
} bit_cvt_dataset_t;

typedef struct {
    uint8_t      bits;
    const char  *name;
} bit_cvt_format_info_t;

static uint8_t  test_u8_data[BIT_CVT_TEST_SAMPLES]     = {0x00, 0x40, 0x80, 0xFF};
static int16_t  test_s16_data[BIT_CVT_TEST_SAMPLES]    = {-32768, -16384, 0, 32767};
static int32_t  test_s32_data[BIT_CVT_TEST_SAMPLES]    = {-2147483648LL, -1073741824, 0, 2147483647};
static uint8_t  test_s24_data[BIT_CVT_TEST_SAMPLES][3] = {{0x00, 0x00, 0x80}, {0x00, 0x00, 0xC0}, {0x00, 0x00, 0x00}, {0xFF, 0xFF, 0x7F}};
static uint32_t sr[]                                   = {44100, 8000};
static uint8_t  ch[]                                   = {1, 2, 4};

static const bit_cvt_dataset_t conversion_datasets[] = {
    {ESP_AE_BIT8, ESP_AE_BIT8, "u8->u8", {0, 64, 128, 255}},
    {ESP_AE_BIT8, ESP_AE_BIT16, "u8->s16", {-32768, -16384, 0, 32512}},
    {ESP_AE_BIT8, ESP_AE_BIT24, "u8->s24", {-8388608, -4194304, 0, 8323072}},
    {ESP_AE_BIT8, ESP_AE_BIT32, "u8->s32", {-2147483648, -1073741824, 0, 2130706432}},
    {ESP_AE_BIT16, ESP_AE_BIT8, "s16->u8", {0, 64, 128, 255}},
    {ESP_AE_BIT16, ESP_AE_BIT16, "s16->s16", {-32768, -16384, 0, 32767}},
    {ESP_AE_BIT16, ESP_AE_BIT24, "s16->s24", {-8388608, -4194304, 0, 8388352}},
    {ESP_AE_BIT16, ESP_AE_BIT32, "s16->s32", {-2147483648, -1073741824, 0, 2147418112}},
    {ESP_AE_BIT24, ESP_AE_BIT8, "s24->u8", {0, 64, 128, 255}},
    {ESP_AE_BIT24, ESP_AE_BIT16, "s24->s16", {-32768, -16384, 0, 32767}},
    {ESP_AE_BIT24, ESP_AE_BIT24, "s24->s24", {-8388608, -4194304, 0, 8388607}},
    {ESP_AE_BIT24, ESP_AE_BIT32, "s24->s32", {-2147483648, -1073741824, 0, 2147483392}},
    {ESP_AE_BIT32, ESP_AE_BIT8, "s32->u8", {0, 64, 128, 255}},
    {ESP_AE_BIT32, ESP_AE_BIT16, "s32->s16", {-32768, -16384, 0, 32767}},
    {ESP_AE_BIT32, ESP_AE_BIT24, "s32->s24", {-8388608, -4194304, 0, 8388607}},
    {ESP_AE_BIT32, ESP_AE_BIT32, "s32->s32", {-2147483648, -1073741824, 0, 2147483647}}};

static const bit_cvt_format_info_t formats[] = {
    {ESP_AE_BIT8, "u8"},
    {ESP_AE_BIT16, "s16"},
    {ESP_AE_BIT24, "s24"},
    {ESP_AE_BIT32, "s32"}};

static void *bit_cvt_get_data_ptr(uint8_t src_bits, int index)
{
    switch (src_bits) {
        case ESP_AE_BIT8:
            return &test_u8_data[index];
        case ESP_AE_BIT16:
            return &test_s16_data[index];
        case ESP_AE_BIT24:
            return test_s24_data[index];
        case ESP_AE_BIT32:
            return &test_s32_data[index];
        default:
            return NULL;
    }
}

static void bit_cvt_prepare_data(uint8_t src_bits, void *buffer, int channels, int samples, bool is_deinterleaved)
{
    int src_sample_size = src_bits / 8;
    for (int ch = 0; ch < channels; ch++) {
        for (int i = 0; i < samples; i++) {
            void *sample_ptr;
            if (is_deinterleaved) {
                void **channel_buffers = (void **)buffer;
                sample_ptr = (uint8_t *)channel_buffers[ch] + i * src_sample_size;
            } else {
                sample_ptr = (uint8_t *)buffer + (i * channels + ch) * src_sample_size;
            }
            void *src_data_ptr = bit_cvt_get_data_ptr(src_bits, i);
            if (src_data_ptr) {
                memcpy(sample_ptr, src_data_ptr, src_sample_size);
            }
        }
    }
}

static const char *bit_cvt_get_format_name(uint8_t bits)
{
    for (int i = 0; i < BIT_CVT_NUM_FORMATS; i++) {
        if (formats[i].bits == bits) {
            return formats[i].name;
        }
    }
    return "unknown";
}

static void bit_cvt_verify_results(const bit_cvt_dataset_t *dataset, void *buffer,
                                   uint8_t dst_bits, int channels, int dst_sample_size,
                                   bool is_deinterleaved, const char *error_msg)
{
    if (dataset == NULL) {
        return;
    }
    ESP_LOGI(TAG, "Verifying %s conversion results%s:", dataset->desc,
             is_deinterleaved ? " (deinterleaved)" : "");
    for (int ch = 0; ch < channels; ch++) {
        for (int i = 0; i < BIT_CVT_TEST_SAMPLES; i++) {
            void *sample_ptr;
            if (is_deinterleaved) {
                void **channel_buffers = (void **)buffer;
                sample_ptr = (uint8_t *)channel_buffers[ch] + i * dst_sample_size;
            } else {
                sample_ptr = (uint8_t *)buffer + (i * channels + ch) * dst_sample_size;
            }
            int32_t actual_value = ae_test_read_sample(sample_ptr, dst_bits);
            int32_t expected_value = dataset->expected_results[i];
            ESP_LOGI(TAG, "Sample[%d] ch[%d]: expected=%ld, actual=%ld",
                     i, ch, expected_value, actual_value);
            TEST_ASSERT_EQUAL_MESSAGE(expected_value, actual_value, error_msg);
        }
    }
}

static void bit_cvt_test_format_conversion(uint8_t src_bits, uint8_t dst_bits, int sample_rate, int channels, bool use_deinterleaved)
{
    const char *src_name = bit_cvt_get_format_name(src_bits);
    const char *dst_name = bit_cvt_get_format_name(dst_bits);
    ESP_LOGI(TAG, "Testing %s->%s conversion [%dch %s]",
             src_name, dst_name, channels, use_deinterleaved ? "deinterleaved" : "interleaved");
    const bit_cvt_dataset_t *dataset = NULL;
    for (int i = 0; i < BIT_CVT_DATASETS; i++) {
        if (conversion_datasets[i].src_bits == src_bits && conversion_datasets[i].dst_bits == dst_bits) {
            dataset = &conversion_datasets[i];
            break;
        }
    }
    esp_ae_bit_cvt_cfg_t cfg = {
        .sample_rate = sample_rate,
        .channel = channels,
        .src_bits = src_bits,
        .dest_bits = dst_bits};
    esp_ae_bit_cvt_handle_t handle = NULL;
    esp_ae_err_t ret = esp_ae_bit_cvt_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(handle);
    int src_sample_size = src_bits / 8;
    int dst_sample_size = dst_bits / 8;
    int total_src_size = BIT_CVT_TEST_SAMPLES * channels * src_sample_size;
    int total_dst_size = BIT_CVT_TEST_SAMPLES * channels * dst_sample_size;
    if (!use_deinterleaved) {
        void *src_buffer = calloc(total_src_size, 1);
        void *dst_buffer = calloc(total_dst_size, 1);
        TEST_ASSERT_NOT_NULL(src_buffer);
        TEST_ASSERT_NOT_NULL(dst_buffer);
        bit_cvt_prepare_data(src_bits, src_buffer, channels, BIT_CVT_TEST_SAMPLES, false);
        ret = esp_ae_bit_cvt_process(handle, BIT_CVT_TEST_SAMPLES, src_buffer, dst_buffer);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        bit_cvt_verify_results(dataset, dst_buffer, dst_bits, channels, dst_sample_size,
                               false, "Bit conversion result mismatch");
        free(src_buffer);
        free(dst_buffer);
    } else if (channels > 1) {
        void *src_channels[BIT_CVT_MAX_CHANNELS];
        void *dst_channels[BIT_CVT_MAX_CHANNELS];
        for (int ch = 0; ch < channels; ch++) {
            src_channels[ch] = calloc(BIT_CVT_TEST_SAMPLES * src_sample_size, 1);
            dst_channels[ch] = calloc(BIT_CVT_TEST_SAMPLES * dst_sample_size, 1);
            TEST_ASSERT_NOT_NULL(src_channels[ch]);
            TEST_ASSERT_NOT_NULL(dst_channels[ch]);
        }
        bit_cvt_prepare_data(src_bits, src_channels, channels, BIT_CVT_TEST_SAMPLES, true);
        ret = esp_ae_bit_cvt_deintlv_process(handle, BIT_CVT_TEST_SAMPLES,
                                             (esp_ae_sample_t *)src_channels, (esp_ae_sample_t *)dst_channels);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        bit_cvt_verify_results(dataset, dst_channels, dst_bits, channels, dst_sample_size,
                               true, "Bit conversion result mismatch (deinterleaved)");
        for (int ch = 0; ch < channels; ch++) {
            free(src_channels[ch]);
            free(dst_channels[ch]);
        }
    }
    esp_ae_bit_cvt_close(handle);
    ESP_LOGI(TAG, "✓ %s->%s [%dch %s] test passed",
             src_name, dst_name, channels, use_deinterleaved ? "deinterleaved" : "interleaved");
}

static void test_bit_cvt_consistency(uint8_t src_bits, uint8_t dst_bits, uint32_t sample_rate, uint8_t channel)
{
    ESP_LOGI(TAG, "Testing bit cvt consistency: %d->%d bits, %ld Hz, %d channels", src_bits, dst_bits, sample_rate, channel);

    esp_ae_bit_cvt_cfg_t config = {
        .src_bits = src_bits,
        .dest_bits = dst_bits,
        .channel = channel,
        .sample_rate = sample_rate};

    esp_ae_bit_cvt_handle_t hd1 = NULL;
    esp_ae_bit_cvt_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_bit_cvt_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_bit_cvt_open(&config, &hd2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    const int duration_ms = 100;
    int input_bytes_per_sample = (src_bits >> 3) * channel;
    int output_bytes_per_sample = (dst_bits >> 3) * channel;

    uint32_t sample_count = (sample_rate * duration_ms) / 1000;
    void *interlv_in = calloc(sample_count, input_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_in, NULL);
    void *interlv_out = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_out, NULL);
    void *deinterlv_in[4] = {0};
    for (int i = 0; i < channel; i++) {
        deinterlv_in[i] = calloc(sample_count, src_bits >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_in[i], NULL);
    }
    void *deinterlv_out[4] = {0};
    for (int i = 0; i < channel; i++) {
        deinterlv_out[i] = calloc(sample_count, dst_bits >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_out[i], NULL);
    }
    void *deinterlv_out_cmp = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(deinterlv_out_cmp, NULL);

    ae_test_generate_sweep_signal(interlv_in, duration_ms, sample_rate,
                                  0.0f, src_bits, channel);

    ret = esp_ae_deintlv_process(channel, src_bits, sample_count,
                                 (esp_ae_sample_t)interlv_in, (esp_ae_sample_t *)deinterlv_in);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_bit_cvt_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_bit_cvt_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(channel, dst_bits, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_bit_cvt_close(hd1);
    esp_ae_bit_cvt_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("Bit Convert branch test", "AUDIO_EFFECT")
{
    esp_ae_bit_cvt_cfg_t config;
    void *bit_handle = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_bit_cvt_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_open(NULL, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.channel = 0;
    config.src_bits = ESP_AE_BIT8;
    config.dest_bits = ESP_AE_BIT16;
    config.sample_rate = 8000;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.channel = 1;
    config.src_bits = ESP_AE_BIT8;
    config.dest_bits = 64;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    config.channel = 1;
    config.src_bits = 64;
    config.dest_bits = ESP_AE_BIT8;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create bit_covert handle");
    config.sample_rate = 100;
    config.channel = 1;
    config.src_bits = ESP_AE_BIT16;
    config.dest_bits = ESP_AE_BIT8;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    ESP_LOGI(TAG, "esp_ae_bit_cvt_process");
    char in_samples[100];
    char out_samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_process(NULL, sample_num, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_process(bit_handle, sample_num, NULL, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_bit_cvt_process(bit_handle, sample_num, (esp_ae_sample_t)in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_bit_cvt_process(bit_handle, 0, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_bit_cvt_process(bit_handle, 10, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_bit_cvt_deintlv_process");
    char in_samples_1[2][100] = {0};
    char out_samples_1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_deintlv_process(NULL, sample_num,
                                         (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, sample_num, NULL, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, sample_num, (esp_ae_sample_t *)in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, 0,
                                         (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, 10,
                                         (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    esp_ae_bit_cvt_close(bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Bit Convert All Formats Matrix Test", "AUDIO_EFFECT")
{
    ESP_LOGI(TAG, "=== Comprehensive Bit Depth Conversion Matrix Test ===");
    for (int src = 0; src < BIT_CVT_NUM_FORMATS; src++) {
        for (int dst = 0; dst < BIT_CVT_NUM_FORMATS; dst++) {
            ESP_LOGI(TAG, "\n--- Testing %s to %s conversion ---", formats[src].name, formats[dst].name);
            for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
                for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
                    bit_cvt_test_format_conversion(formats[src].bits, formats[dst].bits, sr[i], ch[j], false);
                    bit_cvt_test_format_conversion(formats[src].bits, formats[dst].bits, sr[i], ch[j], true);
                }
            }
        }
    }
}

TEST_CASE("Bit Convert interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    uint8_t src_bits[] = {16, 24, 32};
    uint8_t dst_bits[] = {16, 24, 32};
    for (int src_idx = 0; src_idx < AE_TEST_PARAM_NUM(src_bits); src_idx++) {
        for (int dst_idx = 0; dst_idx < AE_TEST_PARAM_NUM(dst_bits); dst_idx++) {
            for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sr); sr_idx++) {
                for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(ch); ch_idx++) {
                    test_bit_cvt_consistency(src_bits[src_idx], dst_bits[dst_idx], sr[sr_idx], ch[ch_idx]);
                }
            }
        }
    }
}
