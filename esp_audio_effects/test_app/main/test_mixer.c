/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "test_common.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_mixer.h"
#include "ae_common.h"

#define TAG "TEST_MIXER"

#define MIXER_TEST_SOURCE_NUM    3
#define MIXER_TEST_SAMPLE_RATE   44100
#define MIXER_TEST_CHANNEL       2
#define MIXER_TEST_DURATION_MS   5000
#define MIXER_TEST_AMPLITUDE_DB  -6.0f
#define MIXER_TEST_TOTAL_SAMPLES MIXER_TEST_DURATION_MS * MIXER_TEST_SAMPLE_RATE / 1000
#define MIXER_TEST_GET_WEIGHT(w) (w * w)
#define MIXER_TEST_INFO_INIT(w1, w2, t) \
    (esp_ae_mixer_info_t) { .weight1 = (w1), .weight2 = (w2), .transit_time = (t) }
#define MIXER_TEST_CFG_INIT(sr, ch, bps, num, src_info_ptr)             \
    (esp_ae_mixer_cfg_t)                                                \
    {                                                                   \
        .sample_rate = (sr), .channel = (ch), .bits_per_sample = (bps), \
        .src_num = (num),                                               \
        .src_info = (src_info_ptr)                                      \
    }
#define MIXER_TEST_SET_WEIGHT(w1, w2, w3) {            \
    mixer_dest_weight[0] = MIXER_TEST_GET_WEIGHT(w1);  \
    mixer_dest_weight[1] = MIXER_TEST_GET_WEIGHT(w2);  \
    mixer_dest_weight[2] = MIXER_TEST_GET_WEIGHT(w3);  \
}
#define MIXER_TEST_SET_IN_BUF(in1, in2, in3) {  \
    mixer_in_samples[0] = in1;                  \
    mixer_in_samples[1] = in2;                  \
    mixer_in_samples[2] = in3;                  \
}
static esp_ae_mixer_handle_t mixer_handle = NULL;
static void                 *in_samples_temp[MIXER_TEST_SOURCE_NUM];
static void                 *out_samples;
static uint8_t               bits[3]       = {16, 24, 32};
static esp_ae_mixer_info_t   mixer_info[3] = {
    MIXER_TEST_INFO_INIT(0.3f, 0.8f, 1000),
    MIXER_TEST_INFO_INIT(0.5f, 1.0f, 1500),
    MIXER_TEST_INFO_INIT(0.2f, 0.6f, 2000),
};
static float mixer_stream_db[3] = {-6.0, -7.0, -8.0};
static float mixer_dest_weight[3];
static void *mixer_in_samples[3];

static float mixer_calculate_output_dbfs(const float *levels_db, const float *weights, int num_inputs)
{
    float sum = 0.0f;
    for (int i = 0; i < num_inputs; i++) {
        float amp = powf(10.0f, (levels_db[i]) / 20.0f);
        sum += amp * weights[i];
    }
    return 20.0f * log10f(sum + 1e-20);
}

static void mixer_verify_output(void *out_samples, int total_samples, float *stream_db,
                                float *dest_weight, int src_num, uint8_t channel, uint8_t bits, bool is_interlv)
{
    float actual_output_dbfs;
    float expected_output_dbfs = mixer_calculate_output_dbfs(stream_db, dest_weight, src_num);
    if (is_interlv) {
        actual_output_dbfs = 3.0f + ae_test_calculate_rms_dbfs((void *)((uint8_t *)out_samples + total_samples / 2 * channel * (bits >> 3)),
                                                               total_samples / 2, bits, channel);
        ESP_LOGI(TAG, "Actual output: %.2f dBFS, Expected output: %.2f dBFS, Diff: %.2f dB",
                 actual_output_dbfs, expected_output_dbfs, fabsf(actual_output_dbfs - expected_output_dbfs));
        if (actual_output_dbfs > -300.0f && expected_output_dbfs > -300.0f) {
            TEST_ASSERT_FLOAT_WITHIN(1.0f, expected_output_dbfs, actual_output_dbfs);
        } else {
            TEST_ASSERT_LESS_THAN(-300.0f, actual_output_dbfs);
        }
    } else {
        void **out_channels = (void **)out_samples;
        for (int ch = 0; ch < channel; ch++) {
            actual_output_dbfs = 3.0f + ae_test_calculate_rms_dbfs((void *)((uint8_t *)(out_channels[ch]) + total_samples / 2 * (bits >> 3)),
                                                                   total_samples / 2, bits, 1);
            ESP_LOGI(TAG, "Actual output: %.2f dBFS, Expected output: %.2f dBFS, Diff: %.2f dB",
                     actual_output_dbfs, expected_output_dbfs, fabsf(actual_output_dbfs - expected_output_dbfs));
            if (actual_output_dbfs > -300.0f && expected_output_dbfs > -300.0f) {
                TEST_ASSERT_FLOAT_WITHIN(1.0f, expected_output_dbfs, actual_output_dbfs);
            } else {
                TEST_ASSERT_LESS_THAN(-300.0f, actual_output_dbfs);
            }
        }
    }
}

static void mixer_data_buf_init(esp_ae_mixer_cfg_t *cfg, int total_samples,
                                float amplitude_db, uint8_t src_num, bool is_interlv,
                                void **in, void **out)
{
    if (is_interlv) {
        const int buffer_bytes = total_samples * cfg->channel * cfg->bits_per_sample >> 3;
        for (int i = 0; i < src_num; i++) {
            in[i] = heap_caps_aligned_calloc(16, 1, buffer_bytes, MALLOC_CAP_SPIRAM);
            TEST_ASSERT_NOT_NULL(in[i]);
            int gen = ae_test_generate_sine_signal(in[i], MIXER_TEST_DURATION_MS, cfg->sample_rate,
                                                 amplitude_db - i, cfg->bits_per_sample, cfg->channel, 1000);
            TEST_ASSERT_EQUAL(total_samples * cfg->channel, gen);
        }
        *out = heap_caps_aligned_calloc(16, 1, buffer_bytes, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_NULL(*out);
    } else {
        const int per_ch_bytes = total_samples * cfg->bits_per_sample >> 3;
        void ***in_buffer = (void ***)in;
        for (int i = 0; i < src_num; i++) {
            in_buffer[i] = (void **)heap_caps_aligned_calloc(16, 1, cfg->channel * sizeof(void *), MALLOC_CAP_SPIRAM);
            TEST_ASSERT_NOT_NULL(in_buffer[i]);
            for (int ch = 0; ch < cfg->channel; ch++) {
                in_buffer[i][ch] = heap_caps_aligned_calloc(16, 1, per_ch_bytes, MALLOC_CAP_SPIRAM);
                TEST_ASSERT_NOT_NULL(in_buffer[i][ch]);
                int gen = ae_test_generate_sine_signal(in_buffer[i][ch], MIXER_TEST_DURATION_MS, cfg->sample_rate,
                                                     amplitude_db - i, cfg->bits_per_sample, 1, 1000);
                TEST_ASSERT_EQUAL(total_samples, gen);
            }
        }
        void **out_buffer = heap_caps_aligned_calloc(16, 1, cfg->channel * sizeof(void *), MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_NULL(out_buffer);
        for (int ch = 0; ch < cfg->channel; ch++) {
            out_buffer[ch] = heap_caps_aligned_calloc(16, 1, per_ch_bytes, MALLOC_CAP_SPIRAM);
            TEST_ASSERT_NOT_NULL(out_buffer[ch]);
        }
        *out = out_buffer;
    }
}

static void mixer_create(uint8_t bits, bool is_interlv)
{
    ESP_LOGI(TAG, "Testing %d-bit %s mixer process", bits, is_interlv ? "interleaved" : "deinterleaved");

    esp_ae_mixer_cfg_t cfg = MIXER_TEST_CFG_INIT(MIXER_TEST_SAMPLE_RATE, MIXER_TEST_CHANNEL, bits, MIXER_TEST_SOURCE_NUM, mixer_info);
    esp_ae_mixer_open(&cfg, &mixer_handle);
    TEST_ASSERT_NOT_NULL(mixer_handle);
    mixer_data_buf_init(&cfg, MIXER_TEST_TOTAL_SAMPLES, MIXER_TEST_AMPLITUDE_DB, MIXER_TEST_SOURCE_NUM,
                        is_interlv, in_samples_temp, (void **)&out_samples);
}

static void run_mixer_test_scenario(void **in_samples, int valid_num, bool is_interlv)
{
    esp_ae_err_t ret;
    if (is_interlv) {
        ret = esp_ae_mixer_process(mixer_handle, MIXER_TEST_TOTAL_SAMPLES, in_samples, out_samples);
    } else {
        ret = esp_ae_mixer_deintlv_process(mixer_handle, MIXER_TEST_TOTAL_SAMPLES, (void ***)in_samples, (void **)(out_samples));
    }
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
}

static void mixer_data_buf_deinit(bool is_interlv, uint8_t src_num, uint8_t channel, void **in, void *out)
{
    if (is_interlv) {
        for (int i = 0; i < src_num; i++) {
            heap_caps_free(in[i]);
        }
        heap_caps_free(out);
    } else {
        void ***in_buffer = (void ***)in;
        void **out_buffer = (void **)out;
        for (int i = 0; i < src_num; i++) {
            for (int ch = 0; ch < channel; ch++) {
                heap_caps_free(in_buffer[i][ch]);
            }
            heap_caps_free(in_buffer[i]);
        }
        for (int ch = 0; ch < channel; ch++) {
            heap_caps_free(out_buffer[ch]);
        }
        heap_caps_free(out);
    }
}

static void mixer_destory(bool is_interlv)
{
    mixer_data_buf_deinit(is_interlv, MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, (void **)in_samples_temp, out_samples);
    esp_ae_mixer_close(mixer_handle);
}

static void run_mixer_single_stream_test(bool is_interlv)
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits); i++) {
        esp_ae_err_t ret;
        ESP_LOGI(TAG, "Testing %d-bit %s single stream mixer process",
                 bits[i], is_interlv ? "interleaved" : "deinterleaved");
        esp_ae_mixer_info_t info = MIXER_TEST_INFO_INIT(0.5f, 0.8f, 1000);
        esp_ae_mixer_cfg_t cfg = MIXER_TEST_CFG_INIT(MIXER_TEST_SAMPLE_RATE, MIXER_TEST_CHANNEL, bits[i], 1, &info);
        esp_ae_mixer_open(&cfg, &mixer_handle);
        TEST_ASSERT_NOT_NULL(mixer_handle);
        ret = esp_ae_mixer_set_mode(mixer_handle, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        void *in_samples[1];
        void *out_samples;
        mixer_data_buf_init(&cfg, MIXER_TEST_TOTAL_SAMPLES, MIXER_TEST_AMPLITUDE_DB, 1, is_interlv, in_samples, (void **)&out_samples);
        if (is_interlv) {
            ret = esp_ae_mixer_process(mixer_handle, MIXER_TEST_TOTAL_SAMPLES, in_samples, out_samples);
        } else {
            ret = esp_ae_mixer_deintlv_process(mixer_handle, MIXER_TEST_TOTAL_SAMPLES,
                                               (esp_ae_sample_t **)in_samples, (esp_ae_sample_t *)out_samples);
        }
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        float stream_db[1] = {MIXER_TEST_AMPLITUDE_DB};
        float dest_weight[1] = {MIXER_TEST_GET_WEIGHT(0.8f)};
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, stream_db, dest_weight, 1, MIXER_TEST_CHANNEL, bits[i], is_interlv);
        mixer_data_buf_deinit(is_interlv, 1, MIXER_TEST_CHANNEL, in_samples, out_samples);
        esp_ae_mixer_close(mixer_handle);
    }
}

TEST_CASE("Mixer branch test", "AUDIO_EFFECT")
{
    esp_ae_mixer_info_t source_info[3] = {0};
    int channel = 2;
    esp_ae_mixer_info_t info1 = MIXER_TEST_INFO_INIT(0.5, 1.0, 3000);
    esp_ae_mixer_info_t info2 = MIXER_TEST_INFO_INIT(0.0, 0.5, 6000);
    source_info[0] = info1;
    source_info[1] = info2;
    esp_ae_mixer_cfg_t downmix_cfg = MIXER_TEST_CFG_INIT(44100, channel, 16, 2, source_info);
    void *downmix_handle = NULL;
    int ret;
    ESP_LOGI(TAG, "esp_ae_mixer_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_open(NULL, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_open(&downmix_cfg, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    downmix_cfg.channel = 0;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    downmix_cfg.channel = channel;
    downmix_cfg.bits_per_sample = 8;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    downmix_cfg.bits_per_sample = 16;
    downmix_cfg.src_num = 0;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    downmix_cfg.src_num = 2;
    info2.transit_time = 0;
    source_info[1] = info2;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create mixer handle");
    downmix_cfg.sample_rate = 100;
    info1.transit_time = 2;
    info2.transit_time = 2;
    source_info[0] = info1;
    source_info[1] = info2;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    char inbuf1[100] = {0};
    char inbuf2[100] = {0};
    char *in_samples[2] = {inbuf1, inbuf2};
    char out_samples[100] = {0};
    char *in_samples_1[2][2] = {0};
    char *out_samples_1[2] = {0};
    int sample_num = 10;
    ESP_LOGI(TAG, "esp_ae_mixer_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_process(NULL, sample_num, (void **)in_samples, (void *)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_process(downmix_handle, sample_num, NULL, (void *)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_process(downmix_handle, sample_num, (void **)in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mixer_process(downmix_handle, 0, (void **)in_samples, (void *)out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    esp_ae_mixer_set_mode(downmix_handle, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    esp_ae_mixer_set_mode(downmix_handle, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
    for (int i = 0; i < 100; i++) {
        ret = esp_ae_mixer_process(downmix_handle, 10, (void **)in_samples, (void *)out_samples);
        TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    }
    ESP_LOGI(TAG, "esp_ae_mixer_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_deintlv_process(NULL, sample_num, (void ***)in_samples_1, (void **)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, sample_num, NULL, (void **)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, sample_num, (void ***)in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 0, (void ***)in_samples_1, (void **)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 10, (void ***)in_samples_1, (void **)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    void **inbuf[3];
    void *in1[10] = {0};
    in1[0] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
    in1[1] = NULL;
    inbuf[0] = in1;
    inbuf[1] = NULL;
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 10, (void ***)inbuf, (void **)out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_mixer_get_transit_time");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_set_mode(NULL, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_set_mode(downmix_handle, 2, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_set_mode(downmix_handle, 0, ESP_AE_MIXER_MODE_MAX);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    free(in1[0]);
    esp_ae_mixer_close(downmix_handle);
}

TEST_CASE("Mixer single stream process test - [INTERLEAVED]", "AUDIO_EFFECT")
{
    run_mixer_single_stream_test(true);
}

TEST_CASE("Mixer single stream process test - [DEINTERLEAVED]", "AUDIO_EFFECT")
{
    run_mixer_single_stream_test(false);
}

TEST_CASE("Mixer disable stream test - [INTERLEAVED]", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits); i++) {
        mixer_create(bits[i], true);
        esp_ae_err_t ret = esp_ae_mixer_set_mode(mixer_handle, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        ret = esp_ae_mixer_set_mode(mixer_handle, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        ret = esp_ae_mixer_set_mode(mixer_handle, 2, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        // Enable all stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 3, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Disable first stream
        MIXER_TEST_SET_IN_BUF(NULL, in_samples_temp[1], in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 2, true);
        MIXER_TEST_SET_WEIGHT(0.0f, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Disable second stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], NULL, in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 2, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, 0.0f, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Disable third stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], NULL);
        run_mixer_test_scenario(mixer_in_samples, 2, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, 0.0f);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Disable two stream
        MIXER_TEST_SET_IN_BUF(NULL, in_samples_temp[1], NULL);
        run_mixer_test_scenario(mixer_in_samples, 1, true);
        MIXER_TEST_SET_WEIGHT(0.0f, mixer_info[1].weight2, 0.0f);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Disable all stream
        MIXER_TEST_SET_IN_BUF(NULL, NULL, NULL);
        run_mixer_test_scenario(mixer_in_samples, 0, true);
        MIXER_TEST_SET_WEIGHT(0.0f, 0.0f, 0.0f);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        mixer_destory(true);
    }
}

TEST_CASE("Mixer disable stream test - [DEINTERLEAVED]", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits); i++) {
        mixer_create(bits[i], false);
        esp_ae_err_t ret = esp_ae_mixer_set_mode(mixer_handle, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        ret = esp_ae_mixer_set_mode(mixer_handle, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        ret = esp_ae_mixer_set_mode(mixer_handle, 2, ESP_AE_MIXER_MODE_FADE_UPWARD);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        // Enable all stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 3, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Disable first stream
        MIXER_TEST_SET_IN_BUF(NULL, in_samples_temp[1], in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 2, false);
        MIXER_TEST_SET_WEIGHT(0, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Disable second stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], NULL, in_samples_temp[2]);
        run_mixer_test_scenario(mixer_in_samples, 2, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, 0, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Disable third stream
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], NULL);
        run_mixer_test_scenario(mixer_in_samples, 2, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, 0);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Disable two stream
        MIXER_TEST_SET_IN_BUF(NULL, in_samples_temp[1], NULL);
        run_mixer_test_scenario(mixer_in_samples, 1, false);
        MIXER_TEST_SET_WEIGHT(0, mixer_info[1].weight2, 0);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Disable all stream
        MIXER_TEST_SET_IN_BUF(NULL, NULL, NULL);
        run_mixer_test_scenario(mixer_in_samples, 0, false);
        MIXER_TEST_SET_WEIGHT(0, 0, 0);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        mixer_destory(false);
    }
}

TEST_CASE("Mixer set mode and reset stream test - [INTERLEAVED]", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits); i++) {
        esp_ae_err_t ret;
        mixer_create(bits[i], true);
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], in_samples_temp[2]);
        // Set all streams to FADE_UPWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_UPWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Reset all streams to initialized state
        ret = esp_ae_mixer_reset(mixer_handle);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        run_mixer_test_scenario(mixer_in_samples, 3, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight1, mixer_info[1].weight1, mixer_info[2].weight1);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Set all streams to FADE_UPWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_UPWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        // Set all streams to FADE_DOWNWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, true);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight1, mixer_info[1].weight1, mixer_info[2].weight1);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], true);
        mixer_destory(true);
    }
}

TEST_CASE("Mixer set mode and reset stream test - [DEINTERLEAVED]", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(bits); i++) {
        esp_ae_err_t ret;
        mixer_create(bits[i], false);
        MIXER_TEST_SET_IN_BUF(in_samples_temp[0], in_samples_temp[1], in_samples_temp[2]);
        // Set all streams to FADE_UPWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_UPWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Reset all streams to initialized state
        ret = esp_ae_mixer_reset(mixer_handle);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        run_mixer_test_scenario(mixer_in_samples, 3, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight1, mixer_info[1].weight1, mixer_info[2].weight1);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Set all streams to FADE_UPWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_UPWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight2, mixer_info[1].weight2, mixer_info[2].weight2);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        // Set all streams to FADE_DOWNWARD mode
        for (int j = 0; j < 3; j++) {
            ret = esp_ae_mixer_set_mode(mixer_handle, j, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        }
        run_mixer_test_scenario(mixer_in_samples, 3, false);
        MIXER_TEST_SET_WEIGHT(mixer_info[0].weight1, mixer_info[1].weight1, mixer_info[2].weight1);
        mixer_verify_output(out_samples, MIXER_TEST_TOTAL_SAMPLES, mixer_stream_db, mixer_dest_weight,
                            MIXER_TEST_SOURCE_NUM, MIXER_TEST_CHANNEL, bits[i], false);
        mixer_destory(false);
    }
}

TEST_CASE("Mixer transition time test", "AUDIO_EFFECT")
{
    esp_ae_mixer_info_t info = MIXER_TEST_INFO_INIT(0.2, 0.8, 500);
    esp_ae_mixer_cfg_t cfg = MIXER_TEST_CFG_INIT(8000, 2, 16, 1, &info);
    esp_ae_mixer_handle_t mixer_hd_interlv = NULL;
    esp_ae_mixer_handle_t mixer_hd_deinterlv = NULL;
    esp_ae_err_t ret = esp_ae_mixer_open(&cfg, &mixer_hd_interlv);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_mixer_open(&cfg, &mixer_hd_deinterlv);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    void *in[1] = {0};
    void **in_deinterlv[1] = {0};
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_mixer_set_mode(mixer_hd_interlv, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_mixer_set_mode(mixer_hd_deinterlv, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    in[0] = heap_caps_aligned_calloc(16, sizeof(short), 1024 * cfg.channel, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(in[0], NULL);
    for (int j = 0; j < 1024 * cfg.channel; j++) {
        ((short *)in[0])[j] = 1000;
    }
    in_deinterlv[0] = (void **)heap_caps_aligned_calloc(16, 1, cfg.channel * sizeof(void *), MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(in_deinterlv[0], NULL);
    for (int j = 0; j < cfg.channel; j++) {
        in_deinterlv[0][j] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(in_deinterlv[0][j], NULL);
        for (int k = 0; k < 1024; k++) {
            ((short *)in_deinterlv[0][j])[k] = 1000;
        }
    }
    void *out = heap_caps_aligned_calloc(16, sizeof(short), 1024 * cfg.channel, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(out, NULL);
    void *out_deinterlv[2] = {0};
    for (int j = 0; j < cfg.channel; j++) {
        out_deinterlv[j] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(out_deinterlv[0], NULL);
    }
    
    int32_t transit_samples_l = 0;
    int32_t transit_samples_r = 0;
    int32_t transit_samples_expected = info.transit_time * cfg.sample_rate / 1000;
    int32_t stable_val = (int32_t)(1000.0 * MIXER_TEST_GET_WEIGHT(info.weight2));
    for (int i = 0; i < 10; i++) {
        ret = esp_ae_mixer_process(mixer_hd_interlv, 1024, in, out);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        for (int j = 0; j < 1024; j++) {
            if (abs(((short *)out)[j * cfg.channel] - stable_val) > 2) {
                transit_samples_l++;
            }
            if (abs(((short *)out)[j * cfg.channel + 1] - stable_val) > 2) {
                transit_samples_r++;
            }
        }
    }
    TEST_ASSERT_EQUAL(transit_samples_l, transit_samples_r);
    int32_t diff = abs(transit_samples_l - transit_samples_expected);
    TEST_ASSERT_LESS_THAN(10, diff);
    transit_samples_l = 0;
    transit_samples_r = 0;
    for (int i = 0; i < 10; i++) {
        ret = esp_ae_mixer_deintlv_process(mixer_hd_deinterlv, 1024, in_deinterlv, out_deinterlv);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        for (int j = 0; j < 1024; j++) {
            if (abs(((short *)out_deinterlv[0])[j] - stable_val) > 2) {
                transit_samples_l++;
            }
            if (abs(((short *)out_deinterlv[1])[j] - stable_val) > 2) {
                transit_samples_r++;
            }
        }
    }
    TEST_ASSERT_EQUAL(transit_samples_l, transit_samples_r);
    diff = abs(transit_samples_l - transit_samples_expected);
    TEST_ASSERT_LESS_THAN(10, diff);
    free(in[0]);
    free(out);
    for (int j = 0; j < cfg.channel; j++) {
        free(out_deinterlv[j]);
        free(in_deinterlv[0][j]);
    }
    free(in_deinterlv[0]);
    esp_ae_mixer_close(mixer_hd_interlv);
    esp_ae_mixer_close(mixer_hd_deinterlv);
}

TEST_CASE("Mixer interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    uint32_t sample_rates[] = {8000, 16000, 44100, 48000};
    uint8_t channels[] = {1, 2};
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sample_rates); sr_idx++) {
        for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits); bit_idx++) {
            for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(channels); ch_idx++) {
                ESP_LOGI(TAG, "Mixer interleave vs deinterleave consistency test---sr: %ld, ch: %d, bits: %d",
                         sample_rates[sr_idx], channels[ch_idx], bits[bit_idx]);
                esp_ae_mixer_info_t info = MIXER_TEST_INFO_INIT(0.2, 0.8, 500);
                esp_ae_mixer_cfg_t cfg = MIXER_TEST_CFG_INIT(sample_rates[sr_idx], channels[ch_idx], bits[bit_idx], 1, &info);
                esp_ae_mixer_handle_t mixer_hd_interlv = NULL;
                esp_ae_mixer_handle_t mixer_hd_deinterlv = NULL;
                esp_ae_err_t ret = esp_ae_mixer_open(&cfg, &mixer_hd_interlv);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_mixer_open(&cfg, &mixer_hd_deinterlv);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                uint32_t total_samples = 100 * sample_rates[sr_idx] / 1000;

                void *in[1] = {0};
                void **in_deinterlv[1] = {0};
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_mixer_set_mode(mixer_hd_interlv, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_mixer_set_mode(mixer_hd_deinterlv, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                in[0] = heap_caps_aligned_calloc(16, cfg.bits_per_sample >> 3, total_samples * cfg.channel, MALLOC_CAP_SPIRAM);
                TEST_ASSERT_NOT_EQUAL(in[0], NULL);
                in_deinterlv[0] = (void **)heap_caps_aligned_calloc(16, 1, cfg.channel * sizeof(void *), MALLOC_CAP_SPIRAM);
                TEST_ASSERT_NOT_EQUAL(in_deinterlv[0], NULL);
                for (int j = 0; j < cfg.channel; j++) {
                    in_deinterlv[0][j] = heap_caps_aligned_calloc(16, cfg.bits_per_sample >> 3, total_samples, MALLOC_CAP_SPIRAM);
                    TEST_ASSERT_NOT_EQUAL(in_deinterlv[0][j], NULL);
                }
                void *out = heap_caps_aligned_calloc(16, cfg.bits_per_sample >> 3, total_samples * cfg.channel, MALLOC_CAP_SPIRAM);
                TEST_ASSERT_NOT_EQUAL(out, NULL);
                void *out_deinterlv[2] = {0};
                for (int j = 0; j < cfg.channel; j++) {
                    out_deinterlv[j] = heap_caps_aligned_calloc(16, cfg.bits_per_sample >> 3, total_samples, MALLOC_CAP_SPIRAM);
                    TEST_ASSERT_NOT_EQUAL(out_deinterlv[0], NULL);
                }
                void *out_deinterlv_cmp = heap_caps_aligned_calloc(16, cfg.bits_per_sample >> 3, total_samples * cfg.channel, MALLOC_CAP_SPIRAM);
                TEST_ASSERT_NOT_EQUAL(out_deinterlv_cmp, NULL);

                ae_test_generate_sweep_signal(in[0], 100, cfg.sample_rate, 0.0f, cfg.bits_per_sample, cfg.channel);
                esp_ae_deintlv_process(cfg.channel, cfg.bits_per_sample, total_samples, (esp_ae_sample_t)in[0], (esp_ae_sample_t *)in_deinterlv[0]);

                for (int i = 0; i < 10; i++) {
                    ret = esp_ae_mixer_process(mixer_hd_interlv, total_samples, in, out);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    ret = esp_ae_mixer_deintlv_process(mixer_hd_deinterlv, total_samples, in_deinterlv, out_deinterlv);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    esp_ae_intlv_process(cfg.channel, cfg.bits_per_sample, total_samples, out_deinterlv, out_deinterlv_cmp);
                    TEST_ASSERT_EQUAL_MEMORY(out, out_deinterlv_cmp, total_samples * (cfg.bits_per_sample >> 3) * cfg.channel);
                    if (cfg.channel > 1) {
                        TEST_ASSERT_EQUAL_MEMORY(out_deinterlv[1], out_deinterlv[0], total_samples * (cfg.bits_per_sample >> 3));
                    }
                }
                free(in[0]);
                free(out);
                for (int j = 0; j < cfg.channel; j++) {
                    free(out_deinterlv[j]);
                    free(in_deinterlv[0][j]);
                }
                free(in_deinterlv[0]);
                free(out_deinterlv_cmp);
                esp_ae_mixer_close(mixer_hd_interlv);
                esp_ae_mixer_close(mixer_hd_deinterlv);
            }
        }
    }
}
