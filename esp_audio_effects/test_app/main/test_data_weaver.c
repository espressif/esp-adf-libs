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
#include "esp_timer.h"
#include "test_common.h"
#include "ae_common.h"
#include "esp_ae_data_weaver.h"

#define TAG "TEST_CROSS_DATA"

typedef struct {
    int          bits;
    int          channels;
    int          samples;
    const char  *name;
} data_weaver_test_cfg_t;

static void data_weaver_prepare_data(void *buffer, int samples, int channels, int bits)
{
    int sample_size = bits >> 3;

    for (int i = 0; i < samples; i++) {
        for (int ch = 0; ch < channels; ch++) {
            int value = i * channels + ch;
            uint8_t *ptr = (uint8_t *)buffer + (i * channels + ch) * sample_size;
            ae_test_write_sample(ptr, value, bits);
        }
    }
}

static bool data_weaver_verify_deinterlv_data(void **deinterleaved_bufs, void *original_buf,
                                              int samples, int channels, int bits)
{
    int sample_size = bits >> 3;

    for (int ch = 0; ch < channels; ch++) {
        for (int i = 0; i < samples; i++) {
            uint8_t *orig_ptr = (uint8_t *)original_buf + (i * channels + ch) * sample_size;
            uint8_t *deint_ptr = (uint8_t *)deinterleaved_bufs[ch] + i * sample_size;

            int32_t orig_value = ae_test_read_sample(orig_ptr, bits);
            int32_t deint_value = ae_test_read_sample(deint_ptr, bits);

            if (orig_value != deint_value) {
                ESP_LOGE(TAG, "Mismatch at sample %d channel %d: orig=%ld deint=%ld",
                         i, ch, orig_value, deint_value);
                return false;
            }
        }
    }
    return true;
}

static bool data_weaver_verify_interlv_data(void *interleaved_buf, void **deinterleaved_bufs,
                                            int samples, int channels, int bits)
{
    int sample_size = bits >> 3;

    for (int i = 0; i < samples; i++) {
        for (int ch = 0; ch < channels; ch++) {
            uint8_t *int_ptr = (uint8_t *)interleaved_buf + (i * channels + ch) * sample_size;
            uint8_t *deint_ptr = (uint8_t *)deinterleaved_bufs[ch] + i * sample_size;

            int32_t int_value = ae_test_read_sample(int_ptr, bits);
            int32_t deint_value = ae_test_read_sample(deint_ptr, bits);

            if (int_value != deint_value) {
                ESP_LOGE(TAG, "Mismatch at sample %d channel %d: int=%ld deint=%ld",
                         i, ch, int_value, deint_value);
                return false;
            }
        }
    }
    return true;
}

static void test_data_weaver_format(const data_weaver_test_cfg_t *config)
{
    ESP_LOGI(TAG, "Testing %s: %d-bit %d channels %d samples",
             config->name, config->bits, config->channels, config->samples);

    int sample_size = config->bits >> 3;
    int total_size = config->samples * config->channels * sample_size;
    void *input_buf = heap_caps_calloc(1, total_size, MALLOC_CAP_DEFAULT);
    TEST_ASSERT_NOT_NULL(input_buf);
    void *output_buf = heap_caps_calloc(1, total_size, MALLOC_CAP_DEFAULT);
    TEST_ASSERT_NOT_NULL(output_buf);
    void **deint_bufs = heap_caps_calloc(config->channels, sizeof(void *), MALLOC_CAP_DEFAULT);
    TEST_ASSERT_NOT_NULL(deint_bufs);
    for (int i = 0; i < config->channels; i++) {
        deint_bufs[i] = heap_caps_calloc(1, config->samples * sample_size, MALLOC_CAP_DEFAULT);
        TEST_ASSERT_NOT_NULL(deint_bufs[i]);
    }
    data_weaver_prepare_data(input_buf, config->samples, config->channels, config->bits);
    int ret = esp_ae_deintlv_process(config->channels, config->bits, config->samples,
                                     input_buf, deint_bufs);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(data_weaver_verify_deinterlv_data(deint_bufs, input_buf,
                                                       config->samples, config->channels, config->bits));
    ret = esp_ae_intlv_process(config->channels, config->bits, config->samples,
                               deint_bufs, output_buf);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(data_weaver_verify_interlv_data(output_buf, deint_bufs,
                                                     config->samples, config->channels, config->bits));
    TEST_ASSERT_EQUAL_MEMORY(input_buf, output_buf, total_size);
    free(input_buf);
    free(output_buf);
    for (int i = 0; i < config->channels; i++) {
        free(deint_bufs[i]);
    }
    free(deint_bufs);
}

TEST_CASE("Data Weaver branch test", "AUDIO_EFFECT")
{
    char in_samples[100];
    char out_samples[100];
    char *in_samples1[2][100] = {0};
    char *out_samples1[2][100] = {0};
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_deintlv_process(2, 16, 10, NULL, (esp_ae_sample_t *)out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_deintlv_process(2, 16, 0, (esp_ae_sample_t)in_samples, (esp_ae_sample_t *)out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_deintlv_process(0, 16, 10, (esp_ae_sample_t)in_samples, (esp_ae_sample_t *)out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_deintlv_process(2, 8, 10, (esp_ae_sample_t)in_samples, (esp_ae_sample_t *)out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    ret = esp_ae_deintlv_process(2, 16, 10, (esp_ae_sample_t)in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    ret = esp_ae_deintlv_process(2, 16, 10, (esp_ae_sample_t)in_samples, (esp_ae_sample_t *)out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_intlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_intlv_process(2, 16, 10, NULL, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_intlv_process(2, 16, 0, (esp_ae_sample_t *)in_samples1, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_intlv_process(0, 16, 10, (esp_ae_sample_t *)in_samples1, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_intlv_process(2, 8, 10, (esp_ae_sample_t *)in_samples1, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    ret = esp_ae_intlv_process(2, 16, 10, (esp_ae_sample_t *)in_samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    ret = esp_ae_intlv_process(2, 16, 10, (esp_ae_sample_t *)in_samples1, (esp_ae_sample_t)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Data Weaver Format Test", "AUDIO_EFFECT")
{
    const data_weaver_test_cfg_t test_configs[] = {
        {ESP_AE_BIT16, 2, 1024, "Stereo 16-bit"},
        {ESP_AE_BIT16, 3, 1024, "3-channel 16-bit"},
        {ESP_AE_BIT24, 2, 1024, "Stereo 24-bit"},
        {ESP_AE_BIT24, 3, 1024, "3-channel 24-bit"},
        {ESP_AE_BIT32, 2, 1024, "Stereo 32-bit"},
        {ESP_AE_BIT32, 3, 1024, "3-channel 32-bit"},
    };

    for (int i = 0; i < sizeof(test_configs) / sizeof(test_configs[0]); i++) {
        test_data_weaver_format(&test_configs[i]);
    }
}
