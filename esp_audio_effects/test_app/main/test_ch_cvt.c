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
#include "ae_common.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_ch_cvt.h"

#define TAG                 "TEST_CHANNEL_CONVERT"
#define CH_CVT_TEST_SAMPLES 4
#define CH_CVT_MAX_CHANNELS 8
#define CH_CVT_FORMATS_NUM  (sizeof(ch_formats) / sizeof(ch_formats[0]))

typedef struct {
    uint8_t      bits;
    const char  *name;
    int          sample_size;
} ch_cvt_format_info_t;

typedef struct {
    uint8_t      src_ch;
    uint8_t      dst_ch;
    uint8_t      bits;
    const char  *desc;
    float       *weights;
    int32_t     *expected_results;
} ch_cvt_dataset_t;

static int16_t test_s16_mono[] = {1000, 2000, 3000, 4000};
static int32_t test_s32_mono[] = {100000, 200000, 300000, 400000};

static const ch_cvt_format_info_t ch_formats[] = {{ESP_AE_BIT16, "s16", sizeof(int16_t)},
                                                  {ESP_AE_BIT24, "s24", 3},
                                                  {ESP_AE_BIT32, "s32", sizeof(int32_t)}};

static float   mono_to_stereo_weights[]      = {1.0f, 1.0f};
static float   stereo_to_mono_weights[]      = {0.6f, 0.5f};
static float   quad_to_stereo_weights[]      = {1.0f, 0.0f, 0.707f, 0.0f,
                                                0.0f, 1.0f, 0.1f, 0.707f};
static int32_t mono_to_stereo_s16_expected[] = {1000, 1000,
                                                2000, 2000,
                                                3000, 3000,
                                                4000, 4000};

static int32_t stereo_to_mono_s16_expected[] = {1100, 2200, 3300, 4400};

static int32_t quad_to_stereo_s16_expected[] = {1707, 1807,
                                                3414, 3614,
                                                5121, 5421,
                                                6828, 7228};

static int32_t mono_to_stereo_s24_expected[] = {256000, 256000,
                                                512000, 512000,
                                                768000, 768000,
                                                1024000, 1024000};

static int32_t stereo_to_mono_s24_expected[] = {281600,
                                                563200,
                                                844800,
                                                1126400};

static int32_t mono_to_stereo_s32_expected[] = {100000, 100000,
                                                200000, 200000,
                                                300000, 300000,
                                                400000, 400000};

static int32_t stereo_to_mono_s32_expected[] = {110000,
                                                220000,
                                                330000,
                                                440000};

static int32_t quad_to_stereo_s32_expected[] = {170700, 180700,
                                                341400, 361400,
                                                512100, 542100,
                                                682800, 722800};

static const ch_cvt_dataset_t ch_conversion_datasets[] = {
    {1, 2, ESP_AE_BIT16, "mono->stereo (s16)", mono_to_stereo_weights, mono_to_stereo_s16_expected},
    {2, 1, ESP_AE_BIT16, "stereo->mono (s16)", stereo_to_mono_weights, stereo_to_mono_s16_expected},
    {2, 2, ESP_AE_BIT16, "stereo bypass (s16)", NULL, mono_to_stereo_s16_expected},
    {4, 2, ESP_AE_BIT16, "quad->stereo (s16)", quad_to_stereo_weights, quad_to_stereo_s16_expected},
    {1, 2, ESP_AE_BIT24, "mono->stereo (s24)", mono_to_stereo_weights, mono_to_stereo_s24_expected},
    {2, 1, ESP_AE_BIT24, "stereo->mono (s24)", stereo_to_mono_weights, stereo_to_mono_s24_expected},
    {1, 2, ESP_AE_BIT32, "mono->stereo (s32)", mono_to_stereo_weights, mono_to_stereo_s32_expected},
    {2, 1, ESP_AE_BIT32, "stereo->mono (s32)", stereo_to_mono_weights, stereo_to_mono_s32_expected},
    {4, 2, ESP_AE_BIT32, "quad->stereo (s32)", quad_to_stereo_weights, quad_to_stereo_s32_expected}};

static void ch_cvt_prepare_data(uint8_t bits, uint8_t channels, void *buffer, bool is_deinterleaved)
{
    void *get_sample_ptr(void *base, int idx, int ch, int sample_size)
    {
        if (is_deinterleaved) {
            void **channel_buffers = (void **)base;
            return (uint8_t *)channel_buffers[ch] + idx * sample_size;
        }
        return (uint8_t *)base + (idx * channels + ch) * sample_size;
    }
    int sample_size = bits >> 3;
    for (int i = 0; i < CH_CVT_TEST_SAMPLES; i++) {
        int val = (bits == ESP_AE_BIT32) ? test_s32_mono[i] : test_s16_mono[i];
        for (int ch = 0; ch < channels; ch++) {
            void *ptr = get_sample_ptr(buffer, i, ch, sample_size);
            ae_test_write_sample(ptr, val, bits);
        }
    }
}

static bool ch_cvt_verify_values(const ch_cvt_dataset_t *dataset, uint8_t sample_size,
                                 void *output, int samples, bool is_deinterleaved)
{
    if (!dataset || !output || !dataset->expected_results) {
        printf("dataset or output or expected_results is NULL\n");
        return false;
    }
    for (int i = 0; i < samples; i++) {
        for (int ch = 0; ch < dataset->dst_ch; ch++) {
            void *sample_ptr;
            if (is_deinterleaved) {
                void **channel_buffers = (void **)output;
                sample_ptr = (uint8_t *)channel_buffers[ch] + i * sample_size;
            } else {
                sample_ptr = (uint8_t *)output + (i * dataset->dst_ch + ch) * sample_size;
            }
            int32_t actual_value = ae_test_read_sample(sample_ptr, dataset->bits);
            int32_t expected_value = dataset->expected_results[i * dataset->dst_ch + ch];
            int32_t diff = abs(actual_value - expected_value);
            int32_t tolerance = 10;
            if (diff > tolerance) {
                ESP_LOGE(TAG, "Value mismatch at sample %d ch %d: expected=%ld, actual=%ld, diff=%ld",
                         i, ch, expected_value, actual_value, diff);
                return false;
            }
        }
    }
    return true;
}

static void ch_cvt_test(uint8_t bits, uint8_t src_ch, uint8_t dst_ch, uint32_t sample_rate,
                        float *weights, bool use_deinterleaved)
{
    const char *format_name = "unknown";
    uint8_t format_idx = 0;
    for (uint8_t i = 0; i < CH_CVT_FORMATS_NUM; i++) {
        if (ch_formats[i].bits == bits) {
            format_name = ch_formats[i].name;
            format_idx = i;
            break;
        }
    }
    ESP_LOGI(TAG, "Testing %s %dch->%dch conversion [%s%s]", format_name, src_ch, dst_ch,
             use_deinterleaved ? "deinterleaved" : "interleaved", weights ? " with custom weights" : "");
    const ch_cvt_dataset_t *dataset = NULL;
    if (ch_conversion_datasets[format_idx].src_ch == src_ch &&
        ch_conversion_datasets[format_idx].dst_ch == dst_ch &&
        ch_conversion_datasets[format_idx].bits == bits) {
        dataset = &ch_conversion_datasets[format_idx];
    }
    esp_ae_ch_cvt_cfg_t cfg = {
        .sample_rate = sample_rate,
        .bits_per_sample = bits,
        .src_ch = src_ch,
        .dest_ch = dst_ch,
        .weight = weights,
        .weight_len = weights ? (src_ch * dst_ch) : 0};

    esp_ae_ch_cvt_handle_t handle = NULL;
    esp_ae_err_t ret = esp_ae_ch_cvt_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(handle);
    int sample_size = ch_formats[format_idx].sample_size;
    int src_total_size = CH_CVT_TEST_SAMPLES * src_ch * sample_size;
    int dst_total_size = CH_CVT_TEST_SAMPLES * dst_ch * sample_size;
    if (!use_deinterleaved) {
        void *src_buffer = calloc(src_total_size, 1);
        void *dst_buffer = calloc(dst_total_size, 1);
        TEST_ASSERT_NOT_NULL(src_buffer);
        TEST_ASSERT_NOT_NULL(dst_buffer);
        ch_cvt_prepare_data(bits, src_ch, src_buffer, false);
        ret = esp_ae_ch_cvt_process(handle, CH_CVT_TEST_SAMPLES, src_buffer, dst_buffer);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        if (dataset) {
            bool values_match = ch_cvt_verify_values(dataset, sample_size, dst_buffer, CH_CVT_TEST_SAMPLES, false);
            TEST_ASSERT_TRUE_MESSAGE(values_match, "Channel conversion values mismatch");
        }
        free(src_buffer);
        free(dst_buffer);
    } else if (src_ch <= CH_CVT_MAX_CHANNELS && dst_ch <= CH_CVT_MAX_CHANNELS) {
        void *src_channels[CH_CVT_MAX_CHANNELS];
        void *dst_channels[CH_CVT_MAX_CHANNELS];
        for (int ch = 0; ch < src_ch; ch++) {
            src_channels[ch] = calloc(CH_CVT_TEST_SAMPLES * sample_size, 1);
            TEST_ASSERT_NOT_NULL(src_channels[ch]);
        }
        for (int ch = 0; ch < dst_ch; ch++) {
            dst_channels[ch] = calloc(CH_CVT_TEST_SAMPLES * sample_size, 1);
            TEST_ASSERT_NOT_NULL(dst_channels[ch]);
        }
        ch_cvt_prepare_data(bits, src_ch, src_channels, true);
        ret = esp_ae_ch_cvt_deintlv_process(handle, CH_CVT_TEST_SAMPLES,
                                            (esp_ae_sample_t *)src_channels, (esp_ae_sample_t *)dst_channels);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        if (dataset) {
            bool values_match = ch_cvt_verify_values(dataset, sample_size, dst_channels, CH_CVT_TEST_SAMPLES, true);
            TEST_ASSERT_TRUE_MESSAGE(values_match, "Channel conversion values mismatch (deinterleaved)");
        }
        for (int ch = 0; ch < src_ch; ch++) {
            free(src_channels[ch]);
        }
        for (int ch = 0; ch < dst_ch; ch++) {
            free(dst_channels[ch]);
        }
    }
    esp_ae_ch_cvt_close(handle);
}

static void test_ch_cvt_consistency(uint8_t src_ch, uint8_t dst_ch, uint8_t bits_per_sample, uint32_t sample_rate)
{
    ESP_LOGI(TAG, "Testing ch cvt consistency: %d->%d channels, %d bits, %ld Hz",
             src_ch, dst_ch, bits_per_sample, sample_rate);
    esp_ae_ch_cvt_cfg_t config = {
        .sample_rate = sample_rate,
        .bits_per_sample = bits_per_sample,
        .src_ch = src_ch,
        .dest_ch = dst_ch,
        .weight = NULL,
        .weight_len = 0};
    esp_ae_ch_cvt_handle_t hd1 = NULL;
    esp_ae_ch_cvt_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_ch_cvt_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_ch_cvt_open(&config, &hd2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    const int duration_ms = 100;
    int input_bytes_per_sample = (bits_per_sample >> 3) * src_ch;
    int output_bytes_per_sample = (bits_per_sample >> 3) * dst_ch;

    uint32_t sample_count = (sample_rate * duration_ms) / 1000;
    void *interlv_in = calloc(sample_count, input_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_in, NULL);
    void *interlv_out = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_out, NULL);
    void *deinterlv_in[8] = {NULL};
    void *deinterlv_out[8] = {NULL};
    for (int i = 0; i < src_ch; i++) {
        deinterlv_in[i] = calloc(sample_count, bits_per_sample >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_in[i], NULL);
    }
    for (int i = 0; i < dst_ch; i++) {
        deinterlv_out[i] = calloc(sample_count, bits_per_sample >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_out[i], NULL);
    }
    void *deinterlv_out_cmp = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(deinterlv_out_cmp, NULL);

    ae_test_generate_sweep_signal(interlv_in, duration_ms, sample_rate,
                                  0.0f, bits_per_sample, src_ch);

    ret = esp_ae_deintlv_process(src_ch, bits_per_sample, sample_count,
                                 (esp_ae_sample_t)interlv_in, (esp_ae_sample_t *)deinterlv_in);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_ch_cvt_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_ch_cvt_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(dst_ch, bits_per_sample, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_ch_cvt_close(hd1);
    esp_ae_ch_cvt_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < src_ch; i++) {
        free(deinterlv_in[i]);
    }
    for (int i = 0; i < dst_ch; i++) {
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("Channel Convert branch test", "AUDIO_EFFECT")
{
    esp_ae_ch_cvt_cfg_t config = {0};
    void *channel_handle = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_ch_cvt_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_open(NULL, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.sample_rate = 8000;
    config.bits_per_sample = 16;
    config.dest_ch = 0;
    config.src_ch = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.dest_ch = 2;
    config.src_ch = 0;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    config.bits_per_sample = 8;
    config.dest_ch = 2;
    config.src_ch = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    config.bits_per_sample = 16;
    float w1 = 123.0f;
    config.weight = &w1;
    config.weight_len = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    float wei[2] = {1, 1.1};
    config.bits_per_sample = 16;
    config.weight = wei;
    config.weight_len = 2;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_ch_cvt_close(channel_handle);
    ESP_LOGI(TAG, "create channel_covert handle");
    config.bits_per_sample = 16;
    config.dest_ch = 3;
    config.src_ch = 8;
    config.sample_rate = 100;
    config.weight = NULL;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_ch_cvt_process");
    char in_samples[100];
    char out_samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_process(NULL, sample_num, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_process(channel_handle, sample_num, NULL, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_ch_cvt_process(channel_handle, sample_num, (esp_ae_sample_t)in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_ch_cvt_process(channel_handle, 0, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_ch_cvt_process(channel_handle, 10, (esp_ae_sample_t)in_samples, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_ch_cvt_deintlv_process");
    char in_samples_1[2][100] = {0};
    char out_samples_1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_deintlv_process(NULL, sample_num,
                                        (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, sample_num, NULL, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, sample_num, (esp_ae_sample_t *)in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, 0,
                                        (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, 10,
                                        (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)out_samples_1);
    esp_ae_ch_cvt_close(channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Channel Convert Basic Functionality Test", "AUDIO_EFFECT")
{
    ESP_LOGI(TAG, "=== Channel Convert Basic Functionality Test ===");
    struct {
        uint8_t bits;
        uint8_t src_ch;
        uint8_t dst_ch;
        uint32_t sample_rate;
        float *weights;
        const char *desc;
    } test_cases[] = {
        {ESP_AE_BIT16, 1, 2, 44100, mono_to_stereo_weights, "Mono to Stereo (16-bit)"},
        {ESP_AE_BIT16, 2, 1, 400, stereo_to_mono_weights, "Stereo to Mono (16-bit)"},
        {ESP_AE_BIT16, 2, 2, 44100, NULL, "Stereo bypass (16-bit)"},
        {ESP_AE_BIT16, 4, 2, 44100, quad_to_stereo_weights, "4-channel to Stereo (16-bit)"},
        {ESP_AE_BIT24, 1, 2, 400, mono_to_stereo_weights, "Mono to Stereo (24-bit)"},
        {ESP_AE_BIT24, 2, 1, 44100, stereo_to_mono_weights, "Stereo to Mono (24-bit)"},
        {ESP_AE_BIT32, 1, 2, 400, mono_to_stereo_weights, "Mono to Stereo (32-bit)"},
        {ESP_AE_BIT32, 2, 1, 44100, stereo_to_mono_weights, "Stereo to Mono (32-bit)"},
        {ESP_AE_BIT32, 4, 2, 44100, quad_to_stereo_weights, "5.1 to Stereo (32-bit)"}};

    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        ESP_LOGI(TAG, "Testing: %s", test_cases[i].desc);
        ch_cvt_test(test_cases[i].bits, test_cases[i].src_ch,
                    test_cases[i].dst_ch, test_cases[i].sample_rate, test_cases[i].weights, false);
        if (test_cases[i].src_ch <= CH_CVT_MAX_CHANNELS && test_cases[i].dst_ch <= CH_CVT_MAX_CHANNELS) {
            ch_cvt_test(test_cases[i].bits, test_cases[i].src_ch,
                        test_cases[i].dst_ch, test_cases[i].sample_rate, test_cases[i].weights, true);
        }
    }
}

TEST_CASE("Channel Convert interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    uint8_t src_channels[] = {1, 2, 4};
    uint8_t dst_channels[] = {1, 2, 4};
    uint8_t bits[] = {16, 24, 32};
    uint32_t sample_rates[] = {8000, 16000, 44100, 48000};
    for (int src_idx = 0; src_idx < AE_TEST_PARAM_NUM(src_channels); src_idx++) {
        for (int dst_idx = 0; dst_idx < AE_TEST_PARAM_NUM(dst_channels); dst_idx++) {
            if (src_channels[src_idx] != dst_channels[dst_idx]) {
                for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits); bit_idx++) {
                    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sample_rates); sr_idx++) {
                        test_ch_cvt_consistency(src_channels[src_idx], dst_channels[dst_idx], bits[bit_idx], sample_rates[sr_idx]);
                    }
                }
            }
        }
    }
}
