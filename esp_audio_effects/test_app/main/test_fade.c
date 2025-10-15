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
#include "esp_ae_fade.h"
#include "esp_ae_data_weaver.h"
#include "ae_common.h"

#define TAG "TEST_FADE"

#define FADE_TEST_DURATION_MS  5000
#define FADE_TIME_MS           1000
#define FADE_AMPLITUDE_DB     -6.0f

static uint32_t             sample_rate[]         = {8000, 44100};
static uint8_t              bits_per_sample[]     = {16, 24, 32};
static uint8_t              channel[]             = {1, 2};
static esp_ae_fade_curve_t  curves[]              = {ESP_AE_FADE_CURVE_LINE, ESP_AE_FADE_CURVE_QUAD, ESP_AE_FADE_CURVE_SQRT};
static esp_ae_fade_mode_t   modes[]               = {ESP_AE_FADE_MODE_FADE_IN, ESP_AE_FADE_MODE_FADE_OUT};
static uint8_t             *input_buffer          = NULL;
static uint8_t             *output_buffer         = NULL;
static uint8_t             *in_deinterlv[3]       = {0};
static uint8_t             *out_deinterlv[3]      = {0};
static esp_ae_fade_handle_t fade_handle           = NULL;
static esp_ae_fade_handle_t fade_deinterlv_handle = NULL;

static void fade_test(esp_ae_fade_cfg_t *config, int num_samples, esp_ae_fade_mode_t mode, bool is_inplace)
{
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    ae_test_generate_sine_signal(input_buffer, FADE_TEST_DURATION_MS, config->sample_rate, FADE_AMPLITUDE_DB,
                               config->bits_per_sample, config->channel, 1000.0f);
    esp_ae_deintlv_process(config->channel, config->bits_per_sample, num_samples,
                           (esp_ae_sample_t)input_buffer, (esp_ae_sample_t *)in_deinterlv);
    if (is_inplace) {
        memcpy(output_buffer, input_buffer, num_samples * (config->bits_per_sample >> 3) * config->channel);
        ret = esp_ae_fade_process(fade_handle, num_samples, (esp_ae_sample_t)output_buffer, (esp_ae_sample_t)output_buffer);
    } else {
        ret = esp_ae_fade_process(fade_handle, num_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
    }
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    int fade_samples = (config->transit_time * config->sample_rate) / 1000;
    int stable_samples = num_samples - fade_samples;

    if (fade_samples > 0) {
        float fade_level1 = 3.0f + ae_test_calculate_rms_dbfs(output_buffer, fade_samples / 2, config->bits_per_sample, config->channel);
        float fade_level2 = 3.0f + ae_test_calculate_rms_dbfs(output_buffer + fade_samples / 2 * (config->bits_per_sample >> 3) * config->channel,
                                                              fade_samples / 2, config->bits_per_sample, config->channel);
        ESP_LOGD(TAG, "Fade period level: %.2fdBFS %.2fdBFS (expected: %.2fdBFS)", fade_level1, fade_level2, FADE_AMPLITUDE_DB);

        TEST_ASSERT_TRUE(fade_level1 < FADE_AMPLITUDE_DB);
        TEST_ASSERT_TRUE(fade_level2 < FADE_AMPLITUDE_DB);

        if (mode == ESP_AE_FADE_MODE_FADE_IN) {
            TEST_ASSERT_TRUE(fade_level1 < fade_level2);
        } else if (mode == ESP_AE_FADE_MODE_FADE_OUT) {
            TEST_ASSERT_TRUE(fade_level1 > fade_level2);
        }
    }

    if (stable_samples > 0) {
        uint8_t *stable_buffer = output_buffer + fade_samples * (config->bits_per_sample >> 3) * config->channel;
        float stable_level = 3.0f + ae_test_calculate_rms_dbfs(stable_buffer, stable_samples, config->bits_per_sample, config->channel);
        ESP_LOGD(TAG, "Stable period level: %.2fdBFS (expected: %.2fdBFS)", stable_level, FADE_AMPLITUDE_DB);
        if (mode == ESP_AE_FADE_MODE_FADE_IN) {
            TEST_ASSERT_FLOAT_WITHIN(0.5f, FADE_AMPLITUDE_DB, stable_level);
        } else if (mode == ESP_AE_FADE_MODE_FADE_OUT) {
            TEST_ASSERT_LESS_THAN(-100.0f, stable_level);
        }
    }

    if (is_inplace) {
        for (int i = 0; i < config->channel; i++) {
            memcpy(out_deinterlv[i], in_deinterlv[i], num_samples * (config->bits_per_sample >> 3));
        }
        ret = esp_ae_fade_deintlv_process(fade_deinterlv_handle, num_samples, (esp_ae_sample_t *)out_deinterlv, (esp_ae_sample_t *)out_deinterlv);
    } else {
        ret = esp_ae_fade_deintlv_process(fade_deinterlv_handle, num_samples, (esp_ae_sample_t *)in_deinterlv, (esp_ae_sample_t *)out_deinterlv);
    }
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_intlv_process(config->channel, config->bits_per_sample, num_samples,
                               (esp_ae_sample_t *)out_deinterlv, (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    if (fade_samples > 0) {
        float fade_level1 = 3.0f + ae_test_calculate_rms_dbfs(output_buffer, fade_samples / 2, config->bits_per_sample, config->channel);
        float fade_level2 = 3.0f + ae_test_calculate_rms_dbfs(output_buffer + fade_samples / 2 * (config->bits_per_sample >> 3) * config->channel,
                                                              fade_samples / 2, config->bits_per_sample, config->channel);
        ESP_LOGD(TAG, "Fade period level: %.2fdBFS %.2fdBFS (expected: %.2fdBFS)", fade_level1, fade_level2, FADE_AMPLITUDE_DB);
        TEST_ASSERT_TRUE(fade_level1 < FADE_AMPLITUDE_DB);
        TEST_ASSERT_TRUE(fade_level2 < FADE_AMPLITUDE_DB);

        if (mode == ESP_AE_FADE_MODE_FADE_IN) {
            TEST_ASSERT_TRUE(fade_level1 < fade_level2);
        } else if (mode == ESP_AE_FADE_MODE_FADE_OUT) {
            TEST_ASSERT_TRUE(fade_level1 > fade_level2);
        }
    }
    if (stable_samples > 0) {
        uint8_t *stable_buffer = output_buffer + fade_samples * (config->bits_per_sample >> 3) * config->channel;
        float stable_level = 3.0f + ae_test_calculate_rms_dbfs(stable_buffer, stable_samples, config->bits_per_sample, config->channel);
        ESP_LOGD(TAG, "Stable period level: %.2fdBFS (expected: %.2fdBFS)", stable_level, FADE_AMPLITUDE_DB);
        if (mode == ESP_AE_FADE_MODE_FADE_IN) {
            TEST_ASSERT_FLOAT_WITHIN(0.5f, FADE_AMPLITUDE_DB, stable_level);
        } else if (mode == ESP_AE_FADE_MODE_FADE_OUT) {
            TEST_ASSERT_LESS_THAN(-100.0f, stable_level);
        }
    }
}

static void test_fade_consistency(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channel)
{
    ESP_LOGI(TAG, "Testing fade consistency: %ld Hz, %d bits, %d channels", sample_rate, bits_per_sample, channel);

    esp_ae_fade_cfg_t config = {
        .sample_rate = sample_rate,
        .channel = channel,
        .bits_per_sample = bits_per_sample,
        .mode = ESP_AE_FADE_MODE_FADE_OUT,
        .curve = ESP_AE_FADE_CURVE_LINE,
        .transit_time = 20,
    };

    esp_ae_fade_handle_t hd1 = NULL;
    esp_ae_fade_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_fade_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_fade_open(&config, &hd2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    const int duration_ms = 100;
    int input_bytes_per_sample = (bits_per_sample >> 3) * channel;
    int output_bytes_per_sample = (bits_per_sample >> 3) * channel;

    uint32_t sample_count = (sample_rate * duration_ms) / 1000;
    void *interlv_in = calloc(sample_count, input_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_in, NULL);
    void *interlv_out = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_out, NULL);
    void *deinterlv_in[2] = {0};
    for (int i = 0; i < channel; i++) {
        deinterlv_in[i] = calloc(sample_count, bits_per_sample >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_in[i], NULL);
    }
    void *deinterlv_out[2] = {0};
    for (int i = 0; i < channel; i++) {
        deinterlv_out[i] = calloc(sample_count, bits_per_sample >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_out[i], NULL);
    }
    void *deinterlv_out_cmp = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(deinterlv_out_cmp, NULL);

    ae_test_generate_sweep_signal(interlv_in, duration_ms, sample_rate,
                                  0.0f, bits_per_sample, channel);

    ret = esp_ae_deintlv_process(channel, bits_per_sample, sample_count,
                                 (esp_ae_sample_t)interlv_in, (esp_ae_sample_t *)deinterlv_in);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_fade_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_fade_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(channel, bits_per_sample, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_fade_close(hd1);
    esp_ae_fade_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("Fade branch test", "AUDIO_EFFECT")
{
    esp_ae_fade_cfg_t config;
    config.mode = ESP_AE_FADE_MODE_FADE_IN;
    config.curve = ESP_AE_FADE_CURVE_LINE;
    config.transit_time = 1000;
    config.sample_rate = 44100;
    config.channel = 1;
    config.bits_per_sample = 16;

    esp_ae_fade_handle_t fade_handle = NULL;
    int ret = 0;

    ESP_LOGI(TAG, "esp_ae_fade_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_open(NULL, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3");
    config.mode = ESP_AE_FADE_MODE_INVALID;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test4");
    config.mode = ESP_AE_FADE_MODE_FADE_IN;
    config.curve = ESP_AE_FADE_CURVE_INVALID;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test5");
    config.curve = ESP_AE_FADE_CURVE_LINE;
    config.channel = 0;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test6");
    config.channel = 1;
    config.bits_per_sample = 8;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test7");
    config.bits_per_sample = 16;
    config.sample_rate = 0;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    // 创建fade_handle
    ESP_LOGI(TAG, "create fade_handle");
    config.sample_rate = 100;
    config.transit_time = 2;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_fade_set_mode");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_set_mode(NULL, ESP_AE_FADE_MODE_FADE_OUT);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_set_mode(fade_handle, ESP_AE_FADE_MODE_INVALID);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_set_mode(fade_handle, ESP_AE_FADE_MODE_FADE_OUT);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_fade_get_mode");
    ESP_LOGI(TAG, "test1");
    esp_ae_fade_mode_t mode;
    ret = esp_ae_fade_get_mode(NULL, &mode);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_get_mode(fade_handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_get_mode(fade_handle, &mode);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(mode, ESP_AE_FADE_MODE_FADE_OUT);

    ESP_LOGI(TAG, "esp_ae_fade_reset");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_reset(NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_reset(fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_fade_process");
    char samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_process(NULL, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_process(fade_handle, sample_num, NULL, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_process(fade_handle, sample_num, samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test4");
    ret = esp_ae_fade_process(fade_handle, 0, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test5");
    for (int i = 0; i < 50; i++) {
        ret = esp_ae_fade_process(fade_handle, 50, samples, samples);
        TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    }

    ESP_LOGI(TAG, "esp_ae_fade_deintlv_process");
    char samples1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_deintlv_process(NULL, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_deintlv_process(fade_handle, sample_num, NULL, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_deintlv_process(fade_handle, sample_num, (esp_ae_sample_t *)samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test4");
    ret = esp_ae_fade_deintlv_process(fade_handle, 0, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    esp_ae_fade_close(fade_handle);
}

TEST_CASE("Fade basic function test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int l = 0; l < AE_TEST_PARAM_NUM(curves); l++) {
                    for (int m = 0; m < AE_TEST_PARAM_NUM(modes); m++) {
                        esp_ae_fade_cfg_t config = {
                            .sample_rate = sample_rate[i],
                            .channel = channel[k],
                            .bits_per_sample = bits_per_sample[j],
                            .mode = modes[m],
                            .curve = curves[l],
                            .transit_time = FADE_TIME_MS,
                        };
                        const int num_samples = FADE_TEST_DURATION_MS * config.sample_rate / 1000;
                        esp_ae_fade_open(&config, &fade_handle);
                        TEST_ASSERT_NOT_NULL(fade_handle);
                        esp_ae_fade_open(&config, &fade_deinterlv_handle);
                        TEST_ASSERT_NOT_NULL(fade_deinterlv_handle);
                        input_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
                        output_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
                        TEST_ASSERT_NOT_NULL(input_buffer);
                        TEST_ASSERT_NOT_NULL(output_buffer);
                        for (int i = 0; i < config.channel; i++) {
                            in_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
                            out_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
                            TEST_ASSERT_NOT_NULL(in_deinterlv[i]);
                            TEST_ASSERT_NOT_NULL(out_deinterlv[i]);
                        }
                        fade_test(&config, num_samples, modes[m], false);
                        free(input_buffer);
                        free(output_buffer);
                        for (int i = 0; i < config.channel; i++) {
                            free(in_deinterlv[i]);
                            free(out_deinterlv[i]);
                        }
                        esp_ae_fade_close(fade_deinterlv_handle);
                        esp_ae_fade_close(fade_handle);
                    }
                }
            }
        }
    }
}

TEST_CASE("Fade inplace test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        esp_ae_fade_cfg_t config = {
            .sample_rate = 44100,
            .channel = 2,
            .bits_per_sample = bits_per_sample[j],
            .mode = ESP_AE_FADE_MODE_FADE_IN,
            .curve = ESP_AE_FADE_CURVE_LINE,
            .transit_time = FADE_TIME_MS,
        };
        const int num_samples = FADE_TEST_DURATION_MS * config.sample_rate / 1000;
        esp_ae_fade_open(&config, &fade_handle);
        TEST_ASSERT_NOT_NULL(fade_handle);
        esp_ae_fade_open(&config, &fade_deinterlv_handle);
        TEST_ASSERT_NOT_NULL(fade_deinterlv_handle);
        input_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
        output_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
        TEST_ASSERT_NOT_NULL(input_buffer);
        TEST_ASSERT_NOT_NULL(output_buffer);
        for (int i = 0; i < config.channel; i++) {
            in_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
            out_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
            TEST_ASSERT_NOT_NULL(in_deinterlv[i]);
            TEST_ASSERT_NOT_NULL(out_deinterlv[i]);
        }
        fade_test(&config, num_samples, ESP_AE_FADE_MODE_FADE_IN, true);
        free(input_buffer);
        free(output_buffer);
        for (int i = 0; i < config.channel; i++) {
            free(in_deinterlv[i]);
            free(out_deinterlv[i]);
        }
        esp_ae_fade_close(fade_deinterlv_handle);
        esp_ae_fade_close(fade_handle);
    }
}

TEST_CASE("Fade mode change test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits_per_sample); i++) {
        esp_ae_fade_cfg_t config = {
            .sample_rate = 8000,
            .channel = 2,
            .bits_per_sample = bits_per_sample[i],
            .mode = ESP_AE_FADE_MODE_FADE_OUT,
            .curve = ESP_AE_FADE_CURVE_QUAD,
            .transit_time = FADE_TIME_MS,
        };
        const int num_samples = FADE_TEST_DURATION_MS * config.sample_rate / 1000;
        esp_ae_fade_open(&config, &fade_handle);
        TEST_ASSERT_NOT_NULL(fade_handle);
        esp_ae_fade_open(&config, &fade_deinterlv_handle);
        TEST_ASSERT_NOT_NULL(fade_deinterlv_handle);
        input_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
        output_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
        TEST_ASSERT_NOT_NULL(input_buffer);
        TEST_ASSERT_NOT_NULL(output_buffer);
        for (int i = 0; i < config.channel; i++) {
            in_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
            out_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
            TEST_ASSERT_NOT_NULL(in_deinterlv[i]);
            TEST_ASSERT_NOT_NULL(out_deinterlv[i]);
        }
        fade_test(&config, num_samples, ESP_AE_FADE_MODE_FADE_OUT, false);
        esp_ae_fade_set_mode(fade_handle, ESP_AE_FADE_MODE_FADE_IN);
        esp_ae_fade_set_mode(fade_deinterlv_handle, ESP_AE_FADE_MODE_FADE_IN);
        fade_test(&config, num_samples, ESP_AE_FADE_MODE_FADE_IN, false);
        esp_ae_fade_reset(fade_handle);
        esp_ae_fade_reset(fade_deinterlv_handle);
        fade_test(&config, num_samples, ESP_AE_FADE_MODE_FADE_OUT, false);
        free(input_buffer);
        free(output_buffer);
        for (int i = 0; i < config.channel; i++) {
            free(in_deinterlv[i]);
            free(out_deinterlv[i]);
        }
        esp_ae_fade_close(fade_deinterlv_handle);
        esp_ae_fade_close(fade_handle);
    }
}

TEST_CASE("Fade transition time test", "AUDIO_EFFECT")
{
    esp_ae_fade_cfg_t config = {
        .sample_rate = 8000,
        .channel = 2,
        .bits_per_sample = 16,
        .mode = ESP_AE_FADE_MODE_FADE_IN,
        .curve = ESP_AE_FADE_CURVE_QUAD,
        .transit_time = 200,
    };
    uint32_t num_samples = 1024;
    esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_NOT_NULL(fade_handle);
    esp_ae_fade_open(&config, &fade_deinterlv_handle);
    TEST_ASSERT_NOT_NULL(fade_deinterlv_handle);
    input_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
    TEST_ASSERT_NOT_NULL(input_buffer);
    output_buffer = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3) * config.channel);
    TEST_ASSERT_NOT_NULL(output_buffer);
    for (int i = 0; i < num_samples * config.channel; i++) {
        ((int16_t *)input_buffer)[i] = 2000;
    }
    for (int i = 0; i < config.channel; i++) {
        in_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
        TEST_ASSERT_NOT_NULL(in_deinterlv[i]);
        for (int j = 0; j < num_samples; j++) {
            ((int16_t *)in_deinterlv[i])[j] = 2000;
        }
        out_deinterlv[i] = (uint8_t *)malloc(num_samples * (config.bits_per_sample >> 3));
        TEST_ASSERT_NOT_NULL(out_deinterlv[i]);
    }
    uint32_t transit_samples_inter_l = 0;
    uint32_t transit_samples_inter_r = 0;
    uint32_t transit_samples_deinter_l = 0;
    uint32_t transit_samples_deinter_r = 0;
    uint32_t transit_samples_expected = config.transit_time * config.sample_rate / 1000;
    for (int i = 0; i < 10; i++) {
        esp_ae_fade_process(fade_handle, num_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        int16_t *output_buffer_16 = (int16_t *)output_buffer;
        for (int j = 0; j < num_samples; j++) {
            if (output_buffer_16[j * 2] != 2000) {
                transit_samples_inter_l++;
            }
            if (output_buffer_16[j * 2 + 1] != 2000) {
                transit_samples_inter_r++;
            }
        }
    }
    TEST_ASSERT_EQUAL(transit_samples_inter_l, transit_samples_expected);
    TEST_ASSERT_EQUAL(transit_samples_inter_r, transit_samples_expected);
    for (int i = 0; i < 10; i++) {
        esp_ae_fade_deintlv_process(fade_deinterlv_handle, num_samples, (esp_ae_sample_t *)in_deinterlv, (esp_ae_sample_t *)out_deinterlv);
        for (int j = 0; j < num_samples; j++) {
            int16_t *out_deinterlv_16 = (int16_t *)out_deinterlv[0];
            if (out_deinterlv_16[j] != 2000) {
                transit_samples_deinter_l++;
            }
            out_deinterlv_16 = (int16_t *)out_deinterlv[1];
            if (out_deinterlv_16[j] != 2000) {
                transit_samples_deinter_r++;
            }
        }
    }
    TEST_ASSERT_EQUAL(transit_samples_deinter_l, transit_samples_expected);
    TEST_ASSERT_EQUAL(transit_samples_deinter_r, transit_samples_expected);

    free(input_buffer);
    free(output_buffer);
    for (int i = 0; i < config.channel; i++) {
        free(in_deinterlv[i]);
        free(out_deinterlv[i]);
    }
    esp_ae_fade_close(fade_deinterlv_handle);
    esp_ae_fade_close(fade_handle);
}

TEST_CASE("Fade reset test", "AUDIO_EFFECT")
{
    uint32_t srate = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = FADE_TEST_DURATION_MS * srate / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    uint8_t *input_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);

    esp_ae_fade_cfg_t fade_config = {
        .sample_rate = srate,
        .channel = ch,
        .bits_per_sample = bits,
        .mode = ESP_AE_FADE_MODE_FADE_OUT,
        .curve = ESP_AE_FADE_CURVE_LINE,
        .transit_time = FADE_TIME_MS,
    };
    esp_ae_fade_handle_t fade_handle = NULL;
    ret = esp_ae_fade_open(&fade_config, &fade_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(fade_handle);

    ae_test_generate_sweep_signal(input_buffer, FADE_TEST_DURATION_MS, srate, FADE_AMPLITUDE_DB, bits, ch);

    ret = esp_ae_fade_process(fade_handle, num_samples / 2,
                              (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                              (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_fade_reset(fade_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_fade_process(fade_handle, num_samples / 2,
                              (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                              (esp_ae_sample_t)output_buffer_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, num_samples / 2 * ch * (bits >> 3));

    esp_ae_fade_close(fade_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("Fade interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sample_rate); sr_idx++) {
        for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits_per_sample); bit_idx++) {
            for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(channel); ch_idx++) {
                test_fade_consistency(sample_rate[sr_idx], bits_per_sample[bit_idx], channel[ch_idx]);
            }
        }
    }
}
