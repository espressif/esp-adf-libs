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
#include "esp_ae_data_weaver.h"
#include "esp_ae_eq.h"
#include "ae_common.h"
#include "esp_dsp.h"

#define TAG "TEST_EQ"

#define EQ_MAX_TEST_CH_NUM  (3)
#define EQ_TEST_DURATION_MS 5000
#define EQ_FFT_SIZE         2048
#define EQ_FREQ_BINS        (EQ_FFT_SIZE / 2)
#define EQ_SET_FILTER_PARA(_filter_type, _fc, _q, _gain) \
    (esp_ae_eq_filter_para_t)                            \
    {                                                    \
        .filter_type = (_filter_type),                   \
        .fc          = (_fc),                            \
        .q           = (_q),                             \
        .gain        = (_gain),                          \
    }
#define EQ_SET_CFG(_srate, _ch, _bits, _filter_num, _para) \
    (esp_ae_eq_cfg_t)                                      \
    {                                                      \
        .sample_rate     = (_srate),                       \
        .channel         = (_ch),                          \
        .bits_per_sample = (_bits),                        \
        .filter_num      = (_filter_num),                  \
        .para            = (_para),                        \
    }

static uint32_t           sample_rate[]     = {8000, 48000};
static uint8_t            bits_per_sample[] = {16, 24, 32};
static uint8_t            channel[]         = {3, 1};
static float              gain_value[]      = {-15.0f, 0, 15.0f};
static float              q_value[]         = {0.1, 2.0, 20.0};
static uint8_t           *input_buffer      = NULL;
static uint8_t           *output_buffer     = NULL;
static float             *input_spectrum    = NULL;
static float             *output_spectrum   = NULL;
static esp_ae_eq_handle_t eq_handle         = NULL;

static void eq_verify_filter_response(esp_ae_eq_cfg_t *config, esp_ae_eq_filter_para_t *eq_para, bool *is_enable,
                                      int num_samples, bool is_interleaved)
{
    for (int i = 0; i < config->channel; i++) {
        uint8_t *inbuf = is_interleaved ? (input_buffer + i * (config->bits_per_sample >> 3)) :
                                          (input_buffer + i * (config->bits_per_sample >> 3) * num_samples);
        uint8_t *outbuf = is_interleaved ? (output_buffer + i * (config->bits_per_sample >> 3)) :
                                           (output_buffer + i * (config->bits_per_sample >> 3) * num_samples);
        uint8_t skip = is_interleaved ? config->channel : 1;
        ae_test_analyze_frequency_response(inbuf, num_samples, 2048, config->bits_per_sample, skip, input_spectrum);
        ae_test_analyze_frequency_response(outbuf, num_samples, 2048, config->bits_per_sample, skip, output_spectrum);
        for (int k = 0; k < EQ_FREQ_BINS; k++) {
            int freq = k * config->sample_rate / EQ_FFT_SIZE;
            ESP_LOGD(TAG, "ch = %d, freq = %dHz, input_spectrum[%d] = %f, output_spectrum[%d] = %f, gain_curve[%d] = %f",
                     i, freq, k, input_spectrum[k], k, output_spectrum[k], k, output_spectrum[k] - input_spectrum[k]);
        }
        esp_ae_eq_filter_para_t para[10] = {0};
        memcpy(para, eq_para, sizeof(esp_ae_eq_filter_para_t) * config->filter_num);
        for (int j = 0; j < config->filter_num; j++) {
            float fc = (float)para[j].fc;
            if (is_enable[j]) {
                if (para[j].filter_type == ESP_AE_EQ_FILTER_LOW_PASS) {
                    int low_bin = (int)(fc * 0.5f * EQ_FFT_SIZE / config->sample_rate);
                    int high_bin = (int)(fc * 2.0f * EQ_FFT_SIZE / config->sample_rate);
                    if (low_bin < EQ_FREQ_BINS && high_bin < EQ_FREQ_BINS) {
                        float low_gain = output_spectrum[low_bin] - input_spectrum[low_bin];
                        float high_gain = output_spectrum[high_bin] - input_spectrum[high_bin];
                        ESP_LOGD(TAG, "Low-pass: low_freq=%.2fdB, high_freq=%.2fdB", low_gain, high_gain);
                        TEST_ASSERT_TRUE(low_gain > high_gain);
                    }
                } else if (para[j].filter_type == ESP_AE_EQ_FILTER_HIGH_PASS) {
                    int low_bin = (int)(fc * 0.5f * EQ_FFT_SIZE / config->sample_rate);
                    int high_bin = (int)(fc * 2.0f * EQ_FFT_SIZE / config->sample_rate);
                    if (low_bin < EQ_FREQ_BINS && high_bin < EQ_FREQ_BINS) {
                        float low_gain = output_spectrum[low_bin] - input_spectrum[low_bin];
                        float high_gain = output_spectrum[high_bin] - input_spectrum[high_bin];
                        ESP_LOGD(TAG, "High-pass: low_freq=%.2fdB, high_freq=%.2fdB", low_gain, high_gain);
                        TEST_ASSERT_TRUE(high_gain > low_gain);
                    }
                } else if (para[j].filter_type == ESP_AE_EQ_FILTER_PEAK) {
                    int fc_bin = (int)(fc * EQ_FFT_SIZE / config->sample_rate);
                    int low_bin = (int)(fc * 0.5f * EQ_FFT_SIZE / config->sample_rate);
                    int high_bin = (int)(fc * 2.0f * EQ_FFT_SIZE / config->sample_rate);
                    float fc_gain = output_spectrum[fc_bin] - input_spectrum[fc_bin];
                    ESP_LOGD(TAG, "Peak%d: Output gain=%.2fdB, Input gain=%.2fdB, fc_gain=%.2fdB, gain=%.2fdB", j,
                             output_spectrum[fc_bin], input_spectrum[fc_bin], fc_gain, para[j].gain);
                    TEST_ASSERT_FLOAT_WITHIN(3.0f, fc_gain, para[j].gain);
                    if (config->filter_num == 1) {
                        float low_gain = output_spectrum[low_bin] - input_spectrum[low_bin];
                        float high_gain = output_spectrum[high_bin] - input_spectrum[high_bin];
                        ESP_LOGD(TAG, "Peak: gain=%.2fdB, low=%.2fdB, high=%.2fdB", fc_gain, low_gain, high_gain);
                        if (para[j].gain > 0) {
                            TEST_ASSERT_TRUE(fc_gain > low_gain && fc_gain > high_gain);
                        } else if (para[j].gain < 0) {
                            TEST_ASSERT_TRUE(fc_gain < low_gain && fc_gain < high_gain);
                        } else {
                            TEST_ASSERT_FLOAT_WITHIN(0.1, fc_gain, low_gain);
                            TEST_ASSERT_FLOAT_WITHIN(0.1, fc_gain, high_gain);
                        }
                    }
                } else if (para[j].filter_type == ESP_AE_EQ_FILTER_HIGH_SHELF) {
                    int low_bin = (int)(fc * 0.5f * EQ_FFT_SIZE / config->sample_rate);
                    int high_bin = (int)(fc * 2.0f * EQ_FFT_SIZE / config->sample_rate);
                    float low_gain = output_spectrum[low_bin] - input_spectrum[low_bin];
                    float high_gain = output_spectrum[high_bin] - input_spectrum[high_bin];
                    ESP_LOGD(TAG, "High-shelf: gain=%.2fdB, low_freq=%.2fdB, high_freq=%.2fdB", para[j].gain, low_gain, high_gain);
                    TEST_ASSERT_FLOAT_WITHIN(3.0f, high_gain, para[j].gain);
                } else if (para[j].filter_type == ESP_AE_EQ_FILTER_LOW_SHELF) {
                    int low_bin = (int)(fc * 0.5f * EQ_FFT_SIZE / config->sample_rate);
                    int high_bin = (int)(fc * 2.0f * EQ_FFT_SIZE / config->sample_rate);
                    float low_gain = output_spectrum[low_bin] - input_spectrum[low_bin];
                    float high_gain = output_spectrum[high_bin] - input_spectrum[high_bin];
                    ESP_LOGD(TAG, "Low-shelf: gain=%.2fdB, low_freq=%.2fdB, high_freq=%.2fdB", para[j].gain, low_gain, high_gain);
                    TEST_ASSERT_FLOAT_WITHIN(3.0f, low_gain, para[j].gain);
                }
            } else {
                int fc_bin = (int)(fc * EQ_FFT_SIZE / config->sample_rate);
                TEST_ASSERT_FLOAT_WITHIN(3.0f, output_spectrum[fc_bin], input_spectrum[fc_bin]);
            }
        }
    }
}

static void eq_test_init(esp_ae_eq_cfg_t *config, int num_samples)
{
    esp_ae_eq_open(config, &eq_handle);
    TEST_ASSERT_NOT_NULL(eq_handle);

    input_buffer = (uint8_t *)calloc(num_samples * config->channel, (config->bits_per_sample >> 3));
    output_buffer = (uint8_t *)calloc(num_samples * config->channel, (config->bits_per_sample >> 3));
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    input_spectrum = (float *)calloc(EQ_FREQ_BINS, sizeof(float));
    output_spectrum = (float *)calloc(EQ_FREQ_BINS, sizeof(float));
    TEST_ASSERT_NOT_NULL(input_spectrum);
    TEST_ASSERT_NOT_NULL(output_spectrum);
}

void eq_test_process(esp_ae_eq_cfg_t *config, esp_ae_eq_filter_para_t *para, bool *is_enable,
                     int num_samples, bool is_interleaved, bool is_inplace)
{
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    if (is_interleaved) {
        ae_test_generate_sweep_signal(input_buffer, EQ_TEST_DURATION_MS, config->sample_rate, -20.0f, config->bits_per_sample, config->channel);
        if (is_inplace) {
            memcpy(output_buffer, input_buffer, num_samples * config->channel * (config->bits_per_sample >> 3));
            ret = esp_ae_eq_process(eq_handle, num_samples, (esp_ae_sample_t)output_buffer, (esp_ae_sample_t)output_buffer);
        } else {
            ret = esp_ae_eq_process(eq_handle, num_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        }
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        eq_verify_filter_response(config, para, is_enable, num_samples, true);
    } else {
        uint8_t *in_deinter[EQ_MAX_TEST_CH_NUM] = {0};
        uint8_t *out_deinter[EQ_MAX_TEST_CH_NUM] = {0};
        for (int i = 0; i < config->channel; i++) {
            int offset = i * (config->bits_per_sample >> 3) * num_samples;
            ae_test_generate_sweep_signal(input_buffer + offset, EQ_TEST_DURATION_MS,
                                          config->sample_rate, -20.0f, config->bits_per_sample, 1);
            in_deinter[i] = input_buffer + offset;
            out_deinter[i] = output_buffer + offset;
        }
        if (is_inplace) {
            for (int i = 0; i < config->channel; i++) {
                memcpy(out_deinter[i], in_deinter[i], num_samples * (config->bits_per_sample >> 3));
            }
            ret = esp_ae_eq_deintlv_process(eq_handle, num_samples, (esp_ae_sample_t *)out_deinter, (esp_ae_sample_t *)out_deinter);
        } else {
            ret = esp_ae_eq_deintlv_process(eq_handle, num_samples, (esp_ae_sample_t *)in_deinter, (esp_ae_sample_t *)out_deinter);
        }
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        eq_verify_filter_response(config, para, is_enable, num_samples, false);
    }
}

void eq_test_deinit()
{
    free(input_buffer);
    input_buffer = NULL;
    free(output_buffer);
    output_buffer = NULL;
    free(input_spectrum);
    input_spectrum = NULL;
    free(output_spectrum);
    output_spectrum = NULL;
    esp_ae_eq_close(eq_handle);
    eq_handle = NULL;
}

static void test_eq_consistency(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channel)
{
    ESP_LOGI(TAG, "Testing EQ consistency: %ld Hz, %d bits, %d channels", sample_rate, bits_per_sample, channel);
    esp_ae_eq_filter_para_t para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 1000, 2.0f, 6.0f);
    esp_ae_eq_cfg_t config = EQ_SET_CFG(sample_rate, channel, bits_per_sample, 1, &para);
    esp_ae_eq_handle_t hd1 = NULL;
    esp_ae_eq_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_eq_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_eq_open(&config, &hd2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    const int duration_ms = 100;
    int input_bytes_per_sample = (bits_per_sample >> 3) * channel;
    int output_bytes_per_sample = (bits_per_sample >> 3) * channel;

    uint32_t sample_count = (sample_rate * duration_ms) / 1000;
    void *interlv_in = calloc(sample_count, input_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_in, NULL);
    void *interlv_out = calloc(sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_out, NULL);
    void *deinterlv_in[4] = {0};
    for (int i = 0; i < channel; i++) {
        deinterlv_in[i] = calloc(sample_count, bits_per_sample >> 3);
        TEST_ASSERT_NOT_EQUAL(deinterlv_in[i], NULL);
    }
    void *deinterlv_out[4] = {0};
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

    ret = esp_ae_eq_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_eq_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(channel, bits_per_sample, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_eq_close(hd1);
    esp_ae_eq_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("EQ branch test", "AUDIO_EFFECT")
{
    esp_ae_eq_handle_t eq_hd = NULL;
    int ret;
    esp_ae_eq_cfg_t *eq_config = calloc(1, sizeof(esp_ae_eq_cfg_t));
    TEST_ASSERT_NOT_NULL(eq_config);
    eq_config->bits_per_sample = 16;
    eq_config->sample_rate = 8000;
    eq_config->channel = 1;
    eq_config->filter_num = 2;
    eq_config->para = calloc(1, sizeof(esp_ae_eq_filter_para_t) * eq_config->filter_num);
    TEST_ASSERT_NOT_NULL(eq_config->para);
    esp_ae_eq_filter_para_t *para1 = eq_config->para + 1;
    memcpy(eq_config->para, &(EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_HIGH_PASS, 500, 1.0f, 5.0f)),
           sizeof(esp_ae_eq_filter_para_t));
    memcpy(eq_config->para + 1, &(EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_HIGH_SHELF, 1000, 1.0f, 5.0f)),
           sizeof(esp_ae_eq_filter_para_t));
    ESP_LOGI(TAG, "esp_ae_eq_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_open(NULL, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_open(eq_config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    eq_config->bits_per_sample = 8;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    eq_config->bits_per_sample = 16;
    eq_config->sample_rate = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    eq_config->sample_rate = 8000;
    eq_config->channel = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    eq_config->channel = 1;
    eq_config->filter_num = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    eq_config->filter_num = 2;
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_INVALID;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_MAX;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test9");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 5000;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test10");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test11");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 2000;
    eq_config->para->q = 0.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test12");
    eq_config->para->fc = 5000;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test13");
    eq_config->para->fc = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test14");
    eq_config->para->fc = 2000;
    eq_config->para->q = 0.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test15");
    eq_config->para->q = 1.0;
    para1->gain = 16.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test16");
    para1->gain = -16.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test17");
    para1->gain = -14.0;
    para1->fc = 20;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_eq_close(eq_hd);
    ESP_LOGI(TAG, "test18");
    para1->fc = 1000;
    para1->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_eq_close(eq_hd);
    ESP_LOGI(TAG, "create eq handle");
    eq_config->para->gain = 5.0;
    eq_config->sample_rate = 100;
    eq_config->para->fc = 20;
    eq_config->filter_num = 1;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    char in_samples[100];
    char in_samples_1[2][100] = {0};
    int sample_num = 10;
    ESP_LOGI(TAG, "esp_ae_eq_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_process(NULL, sample_num, in_samples, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_process(eq_hd, sample_num, NULL, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_process(eq_hd, sample_num, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_process(eq_hd, 0, in_samples, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_eq_process(eq_hd, 10, in_samples, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_eq_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_deintlv_process(NULL, sample_num,
                                    (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, NULL, (esp_ae_sample_t *)in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_deintlv_process(eq_hd, 0,
                                    (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, (esp_ae_sample_t *)in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, (esp_ae_sample_t *)in_samples_1, (esp_ae_sample_t *)in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_eq_set_filter_para");
    esp_ae_eq_filter_para_t para_1;
    para_1.filter_type = ESP_AE_EQ_FILTER_PEAK;
    para_1.fc = 50;
    para_1.q = 1.0;
    para_1.gain = -5.0;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_set_filter_para(NULL, 0, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_set_filter_para(eq_hd, 2, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, &para_1);
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_eq_get_filter_para");
    esp_ae_eq_filter_para_t fi_para = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_get_filter_para(NULL, 0, &fi_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_get_filter_para(eq_hd, 2, &fi_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_get_filter_para(eq_hd, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_get_filter_para(eq_hd, 0, &fi_para);
    TEST_ASSERT_EQUAL(fi_para.filter_type, ESP_AE_EQ_FILTER_PEAK);
    TEST_ASSERT_EQUAL(fi_para.fc, 50);
    TEST_ASSERT_EQUAL(fi_para.q, 1.0);
    TEST_ASSERT_EQUAL(fi_para.gain, -5.0);
    ESP_LOGI(TAG, "%d %d %02f %02f", fi_para.filter_type, (int)fi_para.fc, fi_para.q, fi_para.gain);
    ESP_LOGI(TAG, "esp_ae_eq_enable_filter");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_enable_filter(NULL, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_enable_filter(eq_hd, 2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_eq_disable_filter");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_disable_filter(NULL, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_disable_filter(eq_hd, 2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_eq_close(eq_hd);
    free(eq_config->para);
    free(eq_config);
}

TEST_CASE("EQ Unity Gain Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                uint32_t srate = sample_rate[i];
                uint8_t bits = bits_per_sample[j];
                uint8_t ch = channel[k];
                const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                bool is_enable[1] = {true};
                ESP_LOGI(TAG, "Testing Interleaved Unity Gain with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
                esp_ae_eq_filter_para_t filter_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, srate / 4, 1.0f, 0.0f);
                esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 1, &filter_para);
                eq_test_init(&config, num_samples);
                eq_test_process(&config, &filter_para, is_enable, num_samples, true, false);
                eq_test_deinit();
                ESP_LOGI(TAG, "Testing Deinterleaved Unity Gain with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
                eq_test_init(&config, num_samples);
                eq_test_process(&config, &filter_para, is_enable, num_samples, false, false);
                eq_test_deinit();
            }
        }
    }
}

TEST_CASE("EQ Peak Filter Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int gain_id = 0; gain_id < AE_TEST_PARAM_NUM(gain_value); gain_id++) {
                    for (int q_id = 0; q_id < AE_TEST_PARAM_NUM(q_value); q_id++) {
                        uint32_t srate = sample_rate[i];
                        uint8_t bits = bits_per_sample[j];
                        uint8_t ch = channel[k];
                        float gain = gain_value[gain_id];
                        float q = q_value[q_id];
                        const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                        bool is_enable[1] = {true};
                        ESP_LOGI(TAG, "Testing Interleaved Peak with srate=%ld, bits=%d, ch=%d gain=%02f, q=%02f", srate, bits, ch, gain, q);
                        esp_ae_eq_filter_para_t filter_para_plus = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, srate / 4, q, gain);
                        esp_ae_eq_cfg_t config1 = EQ_SET_CFG(srate, ch, bits, 1, &filter_para_plus);
                        eq_test_init(&config1, num_samples);
                        eq_test_process(&config1, &filter_para_plus, is_enable, num_samples, true, false);
                        eq_test_deinit();

                        ESP_LOGI(TAG, "Testing Deinterleaved Peak with srate=%ld, bits=%d, ch=%d gain=%02f, q=%02f", srate, bits, ch, gain, q);
                        eq_test_init(&config1, num_samples);
                        eq_test_process(&config1, &filter_para_plus, is_enable, num_samples, false, false);
                        eq_test_deinit();
                    }
                }
            }
        }
    }
}

TEST_CASE("EQ Low Pass Filter Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int q_id = 0; q_id < AE_TEST_PARAM_NUM(q_value); q_id++) {
                    uint32_t srate = sample_rate[i];
                    uint8_t bits = bits_per_sample[j];
                    uint8_t ch = channel[k];
                    float q = q_value[q_id];
                    const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                    bool is_enable[1] = {true};
                    ESP_LOGI(TAG, "Testing Interleaved Low Pass with srate=%ld, bits=%d, ch=%d, q=%02f", srate, bits, ch, q);
                    esp_ae_eq_filter_para_t filter_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_LOW_PASS, srate / 4, q, 0.0f);
                    esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 1, &filter_para);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &filter_para, is_enable, num_samples, true, false);
                    eq_test_deinit();
                    ESP_LOGI(TAG, "Testing Deinterleaved Low Pass with srate=%ld, bits=%d, ch=%d, q=%02f", srate, bits, ch, q);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &filter_para, is_enable, num_samples, false, false);
                    eq_test_deinit();
                }
            }
        }
    }
}

TEST_CASE("EQ High Pass Filter Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int q_id = 0; q_id < AE_TEST_PARAM_NUM(q_value); q_id++) {
                    uint32_t srate = sample_rate[i];
                    uint8_t bits = bits_per_sample[j];
                    uint8_t ch = channel[k];
                    float q = q_value[q_id];
                    const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                    bool is_enable[1] = {true};
                    ESP_LOGI(TAG, "Testing Interleaved High Pass with srate=%ld, bits=%d, ch=%d, q=%02f", srate, bits, ch, q);
                    esp_ae_eq_filter_para_t filter_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_HIGH_PASS, srate / 4, q, 0.0f);
                    esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 1, &filter_para);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &filter_para, is_enable, num_samples, true, false);
                    eq_test_deinit();
                    ESP_LOGI(TAG, "Testing Deinterleaved High Pass with srate=%ld, bits=%d, ch=%d, q=%02f", srate, bits, ch, q);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &filter_para, is_enable, num_samples, false, false);
                    eq_test_deinit();
                }
            }
        }
    }
}

TEST_CASE("EQ High Shelf Filter Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int gain_id = 0; gain_id < AE_TEST_PARAM_NUM(gain_value); gain_id++) {
                    uint32_t srate = sample_rate[i];
                    uint8_t bits = bits_per_sample[j];
                    uint8_t ch = channel[k];
                    float gain = gain_value[gain_id];
                    const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                    bool is_enable[1] = {true};
                    ESP_LOGI(TAG, "Testing Interleaved High Shelf with srate=%ld, bits=%d, ch=%d, gain=%02f", srate, bits, ch, gain);
                    esp_ae_eq_filter_para_t high_shelf_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_HIGH_SHELF, 1000, 2.0f, gain);
                    esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 1, &high_shelf_para);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &high_shelf_para, is_enable, num_samples, true, false);
                    eq_test_deinit();
                    ESP_LOGI(TAG, "Testing Deinterleaved High Shelf with srate=%ld, bits=%d, ch=%d, gain=%02f", srate, bits, ch, gain);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &high_shelf_para, is_enable, num_samples, false, false);
                    eq_test_deinit();
                }
            }
        }
    }
}

TEST_CASE("EQ Low Shelf Filter Test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int gain_id = 0; gain_id < AE_TEST_PARAM_NUM(gain_value); gain_id++) {
                    bool is_enable[1] = {true};
                    uint32_t srate = sample_rate[i];
                    uint8_t bits = bits_per_sample[j];
                    uint8_t ch = channel[k];
                    float gain = gain_value[gain_id];
                    ESP_LOGI(TAG, "Testing Interleaved Low Shelf with srate=%ld, bits=%d, ch=%d, gain=%02f",
                                sample_rate[i], bits_per_sample[j], channel[k], gain);
                    const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
                    esp_ae_eq_filter_para_t low_shelf_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_LOW_SHELF, srate / 5, 2.0f, gain);
                    esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 1, &low_shelf_para);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &low_shelf_para, is_enable, num_samples, true, false);
                    eq_test_deinit();
                    ESP_LOGI(TAG, "Testing Deinterleaved Low Shelf with srate=%ld, bits=%d, ch=%d, gain=%02f",
                            sample_rate[i], bits_per_sample[j], channel[k], gain);
                    eq_test_init(&config, num_samples);
                    eq_test_process(&config, &low_shelf_para, is_enable, num_samples, false, false);
                    eq_test_deinit();
                }
            }
        }
    }
}

TEST_CASE("EQ Multi-Filters Test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
            bool is_enable[10] = {true, true, true, true, true, true, true, true, true, true};
            uint32_t srate = 48000;
            uint8_t bits = bits_per_sample[j];
            uint8_t ch = channel[k];
            const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
            static uint32_t fc[10] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
            static float gain[10] = {9, 8, 5, 2, 1, 0, -3, -4, -3, 0};
            ESP_LOGI(TAG, "Testing Interleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
            esp_ae_eq_filter_para_t para[10] = {0};
            for (int i = 0; i < 10; i++) {
                para[i].filter_type = ESP_AE_EQ_FILTER_PEAK;
                para[i].fc = fc[i];
                para[i].q = 2.0f;
                para[i].gain = gain[i];
            }
            esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 10, para);
            eq_test_init(&config, num_samples);
            eq_test_process(&config, para, is_enable, num_samples, true, false);
            eq_test_deinit();
            ESP_LOGI(TAG, "Testing Deinterleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
            eq_test_init(&config, num_samples);
            eq_test_process(&config, para, is_enable, num_samples, false, false);
            eq_test_deinit();
        }
    }
}

TEST_CASE("EQ in place Test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        bool is_enable[10] = {true, true, true, true, true, true, true, true, true, true};
        uint32_t srate = 48000;
        uint8_t bits = bits_per_sample[j];
        uint8_t ch = 2;
        const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
        static uint32_t fc[10] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        static float gain[10] = {9, 8, 5, 2, 1, 0, -3, -4, -3, 0};
        ESP_LOGI(TAG, "Testing Interleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        esp_ae_eq_filter_para_t para[10] = {0};
        for (int i = 0; i < 10; i++) {
            para[i].filter_type = ESP_AE_EQ_FILTER_PEAK;
            para[i].fc = fc[i];
            para[i].q = 2.0f;
            para[i].gain = gain[i];
        }
        esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 10, para);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para, is_enable, num_samples, true, true);
        eq_test_deinit();
        ESP_LOGI(TAG, "Testing Deinterleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para, is_enable, num_samples, false, true);
        eq_test_deinit();
    }
}

TEST_CASE("EQ set filter para Test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        uint32_t srate = 48000;
        uint8_t bits = bits_per_sample[j];
        uint8_t ch = 2;
        bool is_enable[10] = {true, true, true, true, true, true, true, true, true, true};
        const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
        static uint32_t fc[10] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        static float gain[10] = {9, 8, 5, 2, 1, 0, -3, -4, -3, 0};
        esp_ae_eq_filter_para_t eq_para = EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 2000, 1.0f, 6.0f);
        ESP_LOGI(TAG, "Testing Interleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        esp_ae_eq_filter_para_t para[10] = {0};
        for (int i = 0; i < 10; i++) {
            para[i].filter_type = ESP_AE_EQ_FILTER_PEAK;
            para[i].fc = fc[i];
            para[i].q = 2.0f;
            para[i].gain = gain[i];
        }
        esp_ae_eq_filter_para_t para_tmp[10] = {0};
        memcpy(para_tmp, para, sizeof(esp_ae_eq_filter_para_t) * 10);
        esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 10, para_tmp);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para_tmp, is_enable, num_samples, true, false);
        esp_ae_eq_set_filter_para(eq_handle, 6, &eq_para);
        memcpy(&para[6], &eq_para, sizeof(esp_ae_eq_filter_para_t));
        eq_test_process(&config, para, is_enable, num_samples, true, false);
        eq_test_deinit();
        ESP_LOGI(TAG, "Testing Deinterleaved Multi-Filters Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para_tmp, is_enable, num_samples, false, false);
        esp_ae_eq_set_filter_para(eq_handle, 6, &eq_para);
        eq_test_process(&config, para, is_enable, num_samples, false, false);
        eq_test_deinit();
    }
}

TEST_CASE("EQ enable filter band Test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        uint32_t srate = 48000;
        uint8_t bits = bits_per_sample[j];
        uint8_t ch = 2;
        bool is_enable[10] = {true, true, true, true, true, true, true, true, true, true};
        const int num_samples = EQ_TEST_DURATION_MS * srate / 1000;
        static uint32_t fc[10] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        static float gain[10] = {9, 8, 5, 2, 1, 0, -3, -4, -3, 0};
        ESP_LOGI(TAG, "Testing Interleaved Enable Filter Band Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        esp_ae_eq_filter_para_t para[10] = {0};
        for (int i = 0; i < 10; i++) {
            para[i].filter_type = ESP_AE_EQ_FILTER_PEAK;
            para[i].fc = fc[i];
            para[i].q = 2.0f;
            para[i].gain = gain[i];
        }
        esp_ae_eq_cfg_t config = EQ_SET_CFG(srate, ch, bits, 10, para);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para, is_enable, num_samples, true, false);
        esp_ae_eq_disable_filter(eq_handle, 5);
        esp_ae_eq_disable_filter(eq_handle, 6);
        is_enable[5] = false;
        is_enable[6] = false;
        eq_test_process(&config, para, is_enable, num_samples, true, false);
        is_enable[5] = true;
        is_enable[6] = true;
        esp_ae_eq_enable_filter(eq_handle, 5);
        esp_ae_eq_enable_filter(eq_handle, 6);
        eq_test_process(&config, para, is_enable, num_samples, true, false);
        eq_test_deinit();
        ESP_LOGI(TAG, "Testing Deinterleaved Enable Filter Band Test with srate=%ld, bits=%d, ch=%d", srate, bits, ch);
        eq_test_init(&config, num_samples);
        eq_test_process(&config, para, is_enable, num_samples, false, false);
        esp_ae_eq_disable_filter(eq_handle, 5);
        esp_ae_eq_disable_filter(eq_handle, 6);
        is_enable[5] = false;
        is_enable[6] = false;
        eq_test_process(&config, para, is_enable, num_samples, false, false);
        esp_ae_eq_enable_filter(eq_handle, 5);
        esp_ae_eq_enable_filter(eq_handle, 6);
        is_enable[5] = true;
        is_enable[6] = true;
        eq_test_process(&config, para, is_enable, num_samples, false, false);
        eq_test_deinit();
    }
}

TEST_CASE("EQ reset test", "AUDIO_EFFECT")
{
    uint32_t srate = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = EQ_TEST_DURATION_MS * srate / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    uint8_t *input_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);
    esp_ae_eq_filter_para_t para[3] = {
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 1000, 2.0f, 6.0f),
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 4000, 2.0f, -3.0f),
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 8000, 2.0f, 2.0f),
    };
    esp_ae_eq_cfg_t eq_config = EQ_SET_CFG(srate, ch, bits, 3, para);
    esp_ae_eq_handle_t eq_handle = NULL;
    ret = esp_ae_eq_open(&eq_config, &eq_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(eq_handle);
    ae_test_generate_sweep_signal(input_buffer, EQ_TEST_DURATION_MS, srate, -6.0f, bits, ch);
    ret = esp_ae_eq_process(eq_handle, num_samples / 2,
                            (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                            (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_eq_reset(eq_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_eq_process(eq_handle, num_samples / 2,
                            (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                            (esp_ae_sample_t)output_buffer_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, num_samples / 2 * ch * (bits >> 3));

    esp_ae_eq_close(eq_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("EQ interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sample_rate); sr_idx++) {
        for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits_per_sample); bit_idx++) {
            for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(channel); ch_idx++) {
                test_eq_consistency(sample_rate[sr_idx], bits_per_sample[bit_idx], channel[ch_idx]);
            }
        }
    }
}
