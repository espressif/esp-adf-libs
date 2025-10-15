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
#include <stdint.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "test_common.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_drc.h"
#include "ae_common.h"

#define TAG                  "TEST_DRC"
#define DRC_TEST_DURATION_MS 2000

static uint32_t               sr[]         = {8000, 44100};
static uint8_t                bits[]       = {16, 24, 32};
static uint8_t                ch[]         = {1, 2};
static esp_ae_drc_curve_point limter[]     = {{.x = 0, .y = -40}, {.x = -40, .y = -40}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point com_down[]   = {{.x = 0, .y = -20}, {.x = -40, .y = -40}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point com_up[]     = {{.x = 0, .y = 0}, {.x = -20, .y = -20}, {.x = -100, .y = -80}};
static esp_ae_drc_curve_point exp_up[]     = {{.x = 0, .y = 0}, {.x = -20, .y = 0}, {.x = -40, .y = -40}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point exp_down[]   = {{.x = 0, .y = 0}, {.x = -20, .y = -20}, {.x = -80, .y = -100}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point noise_gate[] = {{.x = 0, .y = 0}, {.x = -60, .y = -60}, {.x = -61, .y = -100}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point p1[]         = {{.x = 0, .y = 0}, {.x = -20, .y = -15}, {.x = -60, .y = -55}, {.x = -100, .y = -100}};
static esp_ae_drc_curve_point p2[]         = {{.x = 0, .y = 0}, {.x = -20, .y = -25}, {.x = -60, .y = -65}, {.x = -100, .y = -100}};

static float drc_calculate_expected_output(float input_db, esp_ae_drc_curve_point *points, int point_num)
{
    for (int i = 0; i < point_num - 1; i++) {
        if (input_db >= points[i + 1].x && input_db <= points[i].x) {
            float x1 = points[i].x, y1 = points[i].y;
            float x2 = points[i + 1].x, y2 = points[i + 1].y;
            float output_db = y1 + (y2 - y1) * (input_db - x1) / (x2 - x1);
            return output_db;
        }
    }
    if (input_db > points[0].x) {
        return points[0].y;
    }
    if (input_db < points[point_num - 1].x) {
        return points[point_num - 1].y;
    }
    return input_db;
}

static void drc_effects_test(const char *test_name, esp_ae_drc_curve_point *points, int point_num,
                             uint16_t attack_time, uint16_t release_time,
                             float *input_levels, int num_tests, float tolerance,
                             int bits_per_sample, int channels, int sample_rate, bool is_inplace)
{
    ESP_LOGI(TAG, "Testing %s [%dbit %dch %dHz]", test_name, bits_per_sample, channels, sample_rate);
    esp_ae_drc_cfg_t drc_cfg = {
        .sample_rate = sample_rate,
        .channel = channels,
        .bits_per_sample = bits_per_sample,
        .drc_para = {
            .point = points,
            .point_num = point_num,
            .knee_width = 0.0f,
            .attack_time = attack_time,
            .release_time = release_time,
            .hold_time = 0,
            .makeup_gain = 1.0f
        }
    };
    int bytes_per_sample = bits_per_sample >> 3;
    int frame_size = channels * bytes_per_sample;
    int total_samples = DRC_TEST_DURATION_MS * drc_cfg.sample_rate / 1000;
    int buffer_size = total_samples * frame_size;
    esp_ae_sample_t input_signal = calloc(buffer_size, 1);
    TEST_ASSERT_NOT_NULL(input_signal);
    esp_ae_sample_t output_signal = NULL;
    if (is_inplace) {
        output_signal = input_signal;
    } else {
        output_signal = calloc(buffer_size, 1);
        TEST_ASSERT_NOT_NULL(output_signal);
    }
    esp_ae_drc_handle_t drc_hd = NULL;
    esp_ae_err_t ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "DRC Points Configuration:");
    for (int i = 0; i < point_num; i++) {
        ESP_LOGI(TAG, "  Point %d: (%.1f dB, %.1f dB)", i, points[i].x, points[i].y);
    }
    esp_ae_sample_t *deintlv_input_channels = NULL;
    esp_ae_sample_t *deintlv_output_channels = NULL;
    esp_ae_sample_t deintlv_output_signal = NULL;
    if (channels > 1) {
        deintlv_input_channels = (void **)calloc(channels, sizeof(void *));
        deintlv_output_channels = (void **)calloc(channels, sizeof(void *));
        deintlv_output_signal = calloc(buffer_size, 1);
        TEST_ASSERT_NOT_NULL(deintlv_input_channels);
        TEST_ASSERT_NOT_NULL(deintlv_output_channels);
        TEST_ASSERT_NOT_NULL(deintlv_output_signal);
        for (int ch = 0; ch < channels; ch++) {
            deintlv_input_channels[ch] = calloc(total_samples * bytes_per_sample, 1);
            TEST_ASSERT_NOT_NULL(deintlv_input_channels[ch]);
            if (is_inplace) {
                deintlv_output_channels[ch] = deintlv_input_channels[ch];
            } else {
                deintlv_output_channels[ch] = calloc(total_samples * bytes_per_sample, 1);
                TEST_ASSERT_NOT_NULL(deintlv_output_channels[ch]);
            }
        }
    }
    for (int test = 0; test < num_tests; test++) {
        ESP_LOGI(TAG, "\n--- Testing input: %.1f dB [%dbit %dch] ---",
                 input_levels[test], bits_per_sample, channels);
        ae_test_generate_sine_signal(input_signal, DRC_TEST_DURATION_MS, drc_cfg.sample_rate, input_levels[test],
                                   bits_per_sample, channels, drc_cfg.sample_rate / 5);
        float input_db_measured = 3.0f + ae_test_calculate_rms_dbfs(input_signal, total_samples, bits_per_sample, channels);
        ret = esp_ae_drc_process(drc_hd, total_samples, input_signal, output_signal);
        TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
        float output_db_measured = 3.0f + ae_test_calculate_rms_dbfs(output_signal, total_samples, bits_per_sample, channels);
        float expected_output_db = drc_calculate_expected_output(input_levels[test], points, point_num) + drc_cfg.drc_para.makeup_gain;
        ESP_LOGI(TAG, "Interleaved - Input: %.1f dB (measured: %.1f dB)", input_levels[test], input_db_measured);
        ESP_LOGI(TAG, "Interleaved - Output: %.1f dB (expected: %.1f dB)", output_db_measured, expected_output_db);
        ESP_LOGI(TAG, "Interleaved - Error: %.1f dB", fabsf(output_db_measured - expected_output_db));
        TEST_ASSERT_FLOAT_WITHIN(2.0f, input_levels[test], input_db_measured);
        if (expected_output_db > -90.0f) {
            TEST_ASSERT_FLOAT_WITHIN(tolerance, expected_output_db, output_db_measured);
        } else {
            // Noise gate case
            TEST_ASSERT_LESS_THAN(-90.0f, output_db_measured);
        }
        if (channels > 1) {
            ae_test_generate_sine_signal(input_signal, DRC_TEST_DURATION_MS, drc_cfg.sample_rate, input_levels[test],
                                       bits_per_sample, channels, drc_cfg.sample_rate / 5);
            ret = esp_ae_deintlv_process(channels, bits_per_sample, total_samples, input_signal, deintlv_input_channels);
            TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
            ret = esp_ae_drc_deintlv_process(drc_hd, total_samples, deintlv_input_channels, deintlv_output_channels);
            TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
            ret = esp_ae_intlv_process(channels, bits_per_sample, total_samples, deintlv_output_channels, deintlv_output_signal);
            TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
            float deintlv_output_db_measured = 3.0f + ae_test_calculate_rms_dbfs(deintlv_output_signal, total_samples, bits_per_sample, channels);
            ESP_LOGI(TAG, "Deinterleaved - Output: %.1f dB (expected: %.1f dB)", deintlv_output_db_measured, expected_output_db);
            ESP_LOGI(TAG, "Deinterleaved - Error: %.1f dB", fabsf(deintlv_output_db_measured - expected_output_db));
            if (expected_output_db > -90.0f) {
                TEST_ASSERT_FLOAT_WITHIN(tolerance, expected_output_db, deintlv_output_db_measured);
            } else {
                // Noise gate case
                TEST_ASSERT_LESS_THAN(-90.0f, deintlv_output_db_measured);
            }
        }
    }
    if (channels > 1 && deintlv_input_channels && deintlv_output_channels) {
        for (int ch = 0; ch < channels; ch++) {
            if (deintlv_input_channels[ch]) {
                free(deintlv_input_channels[ch]);
            }
            if (!is_inplace && deintlv_output_channels[ch]) {
                free(deintlv_output_channels[ch]);
            }
        }
        if (deintlv_input_channels) {
            free(deintlv_input_channels);
        }
        if (deintlv_output_channels) {
            free(deintlv_output_channels);
        }
        if (deintlv_output_signal) {
            free(deintlv_output_signal);
        }
    }
    if (input_signal) {
        free(input_signal);
    }
    if (!is_inplace && output_signal) {
        free(output_signal);
    }
    esp_ae_drc_close(drc_hd);
    ESP_LOGI(TAG, "=== %s Test [%dbit %dch] Completed Successfully ===",
             test_name, bits_per_sample, channels);
}

static void test_drc_consistency(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channel)
{
    ESP_LOGI(TAG, "Testing DRC consistency: %ld Hz, %d bits, %d channels", sample_rate, bits_per_sample, channel);
    esp_ae_drc_para_t drc_para = {
        .point = com_down,
        .point_num = 3,
        .knee_width = 2.0,
        .attack_time = 10,
        .release_time = 100,
        .hold_time = 2,
        .makeup_gain = 1.0,
    };
    esp_ae_drc_cfg_t config = {
        .sample_rate = sample_rate,
        .channel = channel,
        .bits_per_sample = bits_per_sample,
        .drc_para = drc_para,
    };
    esp_ae_drc_handle_t hd1 = NULL;
    esp_ae_drc_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_drc_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_drc_open(&config, &hd2);
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

    ret = esp_ae_drc_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_drc_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(channel, bits_per_sample, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_drc_close(hd1);
    esp_ae_drc_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("DRC branch test", "AUDIO_EFFECT")
{
    esp_ae_drc_curve_point point[4];
    memcpy(point, com_up, sizeof(esp_ae_drc_curve_point) * 3);
    esp_ae_drc_para_t drc_para = {
        .point = point,
        .point_num = 3,
        .knee_width = 0,
        .attack_time = 10,
        .release_time = 50,
        .hold_time = 1,
        .makeup_gain = -10};
    esp_ae_drc_cfg_t drc_cfg = {
        .sample_rate = 8000,
        .channel = 1,
        .bits_per_sample = 16,
        .drc_para = {
            .point = point,
            .point_num = 3,
            .knee_width = 0,
            .attack_time = 10,
            .release_time = 50,
            .hold_time = 1,
            .makeup_gain = -10}};
    esp_ae_drc_handle_t drc_hd = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "Test `esp_ae_drc_open` interface");
    ret = esp_ae_drc_open(NULL, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_open(&drc_cfg, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.channel = 0;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.channel = 1;
    drc_cfg.bits_per_sample = 8;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.bits_per_sample = 16;
    drc_cfg.sample_rate = 0;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.sample_rate = 44100;
    drc_cfg.drc_para.point = NULL;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    memcpy(&drc_cfg.drc_para, &drc_para, sizeof(esp_ae_drc_para_t));
    drc_cfg.drc_para.makeup_gain = -11;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.makeup_gain = 11;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.makeup_gain = 10;
    drc_cfg.drc_para.knee_width = -1;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.knee_width = 11;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.knee_width = 0;
    drc_cfg.drc_para.attack_time = 501;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.attack_time = 500;
    drc_cfg.drc_para.release_time = 501;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.release_time = 500;
    drc_cfg.drc_para.hold_time = 101;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.hold_time = 100;
    drc_cfg.drc_para.point_num = 1;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point_num = 7;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point_num = 3;
    drc_cfg.drc_para.point[0].x = 1;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point[0].x = 0;
    drc_cfg.drc_para.point[0].y = 1;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point[0].x = -10;
    drc_cfg.drc_para.point[0].y = 0;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point[0].x = 0;
    drc_cfg.drc_para.point[1].x = 0;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_cfg.drc_para.point[1].x = -10;
    drc_cfg.drc_para.point[2].x = -90;
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    drc_para.point = com_up;
    drc_para.point_num = 3;
    drc_cfg.sample_rate = 100;
    memcpy(&drc_cfg.drc_para, &drc_para, sizeof(esp_ae_drc_para_t));
    ret = esp_ae_drc_open(&drc_cfg, &drc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "Test `esp_ae_drc_process` interface");
    char samples[100];
    int sample_num = 10;
    ret = esp_ae_drc_process(NULL, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_process(drc_hd, sample_num, NULL, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_process(drc_hd, sample_num, samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_process(drc_hd, 0, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_process(drc_hd, 10, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "Test `esp_ae_drc_deintlv_process` interface");
    char samples1[2][100] = {0};
    ret = esp_ae_drc_deintlv_process(NULL, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_deintlv_process(drc_hd, sample_num, NULL, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_deintlv_process(drc_hd, sample_num, (esp_ae_sample_t *)samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_deintlv_process(drc_hd, 0, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_deintlv_process(drc_hd, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_attack_time` interface");
    uint16_t attack_time = 504;
    ret = esp_ae_drc_set_attack_time(NULL, attack_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_attack_time(drc_hd, attack_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_attack_time(drc_hd, 500);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_attack_time` interface");
    ret = esp_ae_drc_get_attack_time(NULL, &attack_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_attack_time(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_attack_time(drc_hd, &attack_time);
    TEST_ASSERT_EQUAL(attack_time, 500);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_release_time` interface");
    uint16_t release_time = 504;
    ret = esp_ae_drc_set_release_time(NULL, release_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_release_time(drc_hd, release_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_release_time(drc_hd, 500);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_release_time` interface");
    ret = esp_ae_drc_get_release_time(NULL, &release_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_release_time(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_release_time(drc_hd, &release_time);
    TEST_ASSERT_EQUAL(release_time, 500);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_hold_time` interface");
    uint16_t hold_time = 504;
    ret = esp_ae_drc_set_hold_time(NULL, hold_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_hold_time(drc_hd, hold_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_hold_time(drc_hd, 100);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_hold_time` interface");
    ret = esp_ae_drc_get_hold_time(NULL, &hold_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_hold_time(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_hold_time(drc_hd, &hold_time);
    TEST_ASSERT_EQUAL(hold_time, 100);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_makeup_gain` interface");
    float makeup_gain = -11.0;
    ret = esp_ae_drc_set_makeup_gain(NULL, makeup_gain);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_makeup_gain(drc_hd, makeup_gain);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_makeup_gain(drc_hd, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_makeup_gain` interface");
    ret = esp_ae_drc_get_makeup_gain(NULL, &makeup_gain);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_makeup_gain(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_makeup_gain(drc_hd, &makeup_gain);
    TEST_ASSERT_EQUAL(makeup_gain, 0);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_knee_width` interface");
    float knee_width = -1.0;
    ret = esp_ae_drc_set_knee_width(NULL, knee_width);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_knee_width(drc_hd, knee_width);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_knee_width(drc_hd, 1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_knee_width` interface");
    ret = esp_ae_drc_get_knee_width(NULL, &knee_width);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_knee_width(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_knee_width(drc_hd, &knee_width);
    TEST_ASSERT_EQUAL(knee_width, 1);

    ESP_LOGI(TAG, "Test `esp_ae_drc_set_curve_points` interface");
    esp_ae_drc_curve_point ptest[3] = {{.x = 0, .y = -10}, {.x = -50, .y = -50}, {.x = -100, .y = -100}};
    ret = esp_ae_drc_set_curve_points(NULL, ptest, 1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_curve_points(drc_hd, NULL, 1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_curve_points(drc_hd, ptest, 1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_set_curve_points(drc_hd, ptest, 3);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_curve_point_num` interface");
    uint8_t point_num = 0;
    ret = esp_ae_drc_get_curve_point_num(NULL, &point_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_curve_point_num(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_curve_point_num(drc_hd, &point_num);
    TEST_ASSERT_EQUAL(point_num, 3);

    ESP_LOGI(TAG, "Test `esp_ae_drc_get_curve_points` interface");
    esp_ae_drc_curve_point ptest2[3] = {0};
    ret = esp_ae_drc_get_curve_points(NULL, ptest2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_curve_points(drc_hd, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_drc_get_curve_points(drc_hd, ptest2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(ptest2[0].x, 0);
    TEST_ASSERT_EQUAL(ptest2[0].y, -10);
    TEST_ASSERT_EQUAL(ptest2[1].x, -50);
    TEST_ASSERT_EQUAL(ptest2[1].y, -50);
    TEST_ASSERT_EQUAL(ptest2[2].x, -100);
    TEST_ASSERT_EQUAL(ptest2[2].y, -100);

    esp_ae_drc_close(drc_hd);
}

TEST_CASE("DRC Compressor UP test", "[drc][compressor_up]")
{
    float input_levels[] = {-5.0f, -20.0f, -40.0f, -60.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Compressor UP", com_up, AE_TEST_PARAM_NUM(com_up),
                                 1, 50, input_levels, num_tests, 3.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC Compressor DOWN test", "[drc][compressor_down]")
{
    float input_levels[] = {-5.0f, -15.0f, -20.0f, -40.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Compressor DOWN", com_down, AE_TEST_PARAM_NUM(com_down),
                                 1, 50, input_levels, num_tests, 3.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC Expander UP test", "[drc][expander_up]")
{
    float input_levels[] = {-5.0f, -15.0f, -25.0f, -50.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Expander UP", exp_up, AE_TEST_PARAM_NUM(exp_up),
                                 50, 1, input_levels, num_tests, 3.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC Expander DOWN test", "[drc][expander_down]")
{
    float input_levels[] = {-5.0f, -15.0f, -30.0f, -50.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Expander DOWN", exp_down, AE_TEST_PARAM_NUM(exp_down),
                                 1, 50, input_levels, num_tests, 3.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC Noise Gate test", "[drc][noise_gate]")
{
    float input_levels[] = {-5.0f, -15.0f, -30.0f, -50.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Noise Gate", noise_gate, AE_TEST_PARAM_NUM(noise_gate),
                                 1, 20, input_levels, num_tests, 5.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC Limiter test", "[drc][limiter]")
{
    float input_levels[] = {-5.0f, -15.0f, -30.0f, -55.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("Limiter", limter, AE_TEST_PARAM_NUM(limter),
                                 1, 20, input_levels, num_tests, 5.0f, bits[k], ch[j], sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC joint test", "AUDIO_EFFECT")
{
    float input_levels[] = {-5.0f, -15.0f, -30.0f, -55.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sr); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(ch); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                drc_effects_test("joint", p1, AE_TEST_PARAM_NUM(p1), 1, 20,
                                 input_levels, num_tests, 5.0f, bits[k], 2, sr[i], false);
                drc_effects_test("joint", p2, AE_TEST_PARAM_NUM(p2), 1, 20,
                                 input_levels, num_tests, 5.0f, bits[k], 2, sr[i], false);
            }
        }
    }
}

TEST_CASE("DRC inplace test", "AUDIO_EFFECT")
{
    float input_levels[] = {-5.0f, -15.0f, -30.0f, -55.0f, -70.0f};
    int num_tests = sizeof(input_levels) / sizeof(input_levels[0]);
    for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
        drc_effects_test("DRC inplace", p1, AE_TEST_PARAM_NUM(p1), 1, 20, input_levels,
                         num_tests, 5.0f, bits[k], 2, 8000, true);
    }
}

TEST_CASE("DRC attack, release and hold time response test", "AUDIO_EFFECT")
{
    esp_ae_drc_para_t fast_attack_para = {
        .point = com_down,
        .point_num = 3,
        .knee_width = 0.0,
        .attack_time = 1,
        .release_time = 50,
        .hold_time = 10,
        .makeup_gain = 0};
    esp_ae_drc_para_t slow_attack_para = {
        .point = com_down,
        .point_num = 3,
        .knee_width = 0.0,
        .attack_time = 50,
        .release_time = 1,
        .hold_time = 10,
        .makeup_gain = 0};
    esp_ae_drc_cfg_t drc_cfg = {
        .sample_rate = 8000,
        .channel = 1,
        .bits_per_sample = 16};
    memcpy(&drc_cfg.drc_para, &fast_attack_para, sizeof(esp_ae_drc_para_t));
    esp_ae_drc_handle_t fast_drc = NULL;
    int ret = esp_ae_drc_open(&drc_cfg, &fast_drc);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    memcpy(&drc_cfg.drc_para, &slow_attack_para, sizeof(esp_ae_drc_para_t));
    esp_ae_drc_handle_t slow_drc = NULL;
    ret = esp_ae_drc_open(&drc_cfg, &slow_drc);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    const int signal_length = 8820 * 2;
    const int burst_start = 2205;
    const int burst_end = 6615;
    int16_t *input_signal = calloc(signal_length, sizeof(int16_t));
    int16_t *fast_output = calloc(signal_length, sizeof(int16_t));
    int16_t *slow_output = calloc(signal_length, sizeof(int16_t));
    TEST_ASSERT_NOT_NULL(input_signal);
    TEST_ASSERT_NOT_NULL(fast_output);
    TEST_ASSERT_NOT_NULL(slow_output);
    int ret_burst = ae_test_generate_burst_signal(input_signal, signal_length, drc_cfg.sample_rate,
                                                  1000.0f, 5000, burst_start, burst_end);
    TEST_ASSERT_EQUAL(ret_burst, 0);
    ret = esp_ae_drc_process(fast_drc, signal_length, input_signal, fast_output);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_drc_process(slow_drc, signal_length, input_signal, slow_output);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    int fast_attack_sample = -1;
    int slow_attack_sample = -1;
    for (int i = burst_start; i < burst_end; i++) {
        float input_db = (float)(20.0 * log10(fabs((float)input_signal[i]) / 32767.0f));
        float fast_db = (float)(20.0 * log10(fabs((float)fast_output[i]) / 32767.0f));
        float slow_db = (float)(20.0 * log10(fabs((float)slow_output[i]) / 32767.0f));
        float fast_ratio = (input_db + 40.0) / (fast_db + 40.0);
        float slow_ratio = (input_db + 40.0) / (slow_db + 40.0);
        if (fast_attack_sample == -1 && fast_ratio > 1.5f) {
            fast_attack_sample = i - burst_start;
        }
        if (slow_attack_sample == -1 && slow_ratio > 1.5f) {
            slow_attack_sample = i - burst_start;
        }
    }
    float fast_attack_time = fast_attack_sample * 1000.0f / drc_cfg.sample_rate;
    float slow_attack_time = slow_attack_sample * 1000.0f / drc_cfg.sample_rate;
    ESP_LOGI(TAG, "Fast attack 90%% time: %d samples (%.1fms)",
             fast_attack_sample, fast_attack_time);
    ESP_LOGI(TAG, "Slow attack 90%% time: %d samples (%.1fms)",
             slow_attack_sample, slow_attack_time);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, fast_attack_time, fast_attack_para.attack_time);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, slow_attack_time, slow_attack_para.attack_time);

    int fast_release_sample = -1;
    int slow_release_sample = -1;
    for (int i = burst_end; i < burst_end + 8820; i++) {
        float input_db = (float)(20.0 * log10(fabs((float)input_signal[i]) / 32767.0f));
        float fast_db = (float)(20.0 * log10(fabs((float)fast_output[i]) / 32767.0f));
        float slow_db = (float)(20.0 * log10(fabs((float)slow_output[i]) / 32767.0f));
        float fast_ratio = (input_db + 40.0) / (fast_db + 40.0);
        float slow_ratio = (input_db + 40.0) / (slow_db + 40.0);
        if (fast_release_sample == -1 && fast_ratio > 0.85f) {
            fast_release_sample = i - burst_end;
        }
        if (slow_release_sample == -1 && slow_ratio > 0.85f) {
            slow_release_sample = i - burst_end;
        }
    }
    float fast_release_time = fast_release_sample * 1000.0f / drc_cfg.sample_rate - fast_attack_para.hold_time;
    float slow_release_time = slow_release_sample * 1000.0f / drc_cfg.sample_rate - slow_attack_para.hold_time;
    ESP_LOGI(TAG, "Fast release 90%% time: %d samples (%.1fms)",
             fast_release_sample, fast_release_time);
    ESP_LOGI(TAG, "Slow release 90%% time: %d samples (%.1fms)",
             slow_release_sample, slow_release_time);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, fast_release_time, fast_attack_para.release_time);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, slow_release_time, slow_attack_para.release_time);
    free(input_signal);
    free(fast_output);
    free(slow_output);
    esp_ae_drc_close(fast_drc);
    esp_ae_drc_close(slow_drc);
}

TEST_CASE("DRC reset test", "AUDIO_EFFECT")
{
    uint32_t srate = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = DRC_TEST_DURATION_MS * srate / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;

    int16_t *input_buffer = (int16_t *)calloc(num_samples * ch, sizeof(int16_t));
    int16_t *output_buffer = (int16_t *)calloc(num_samples * ch, sizeof(int16_t));
    int16_t *output_buffer_reset = (int16_t *)calloc(num_samples * ch, sizeof(int16_t));
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);
    esp_ae_drc_para_t drc_para = {
        .point = com_down,
        .point_num = 3,
        .knee_width = 2.0,
        .attack_time = 10,
        .release_time = 100,
        .hold_time = 2,
        .makeup_gain = 1.0,
    };
    esp_ae_drc_cfg_t drc_cfg = {
        .sample_rate = srate,
        .channel = ch,
        .bits_per_sample = bits,
    };
    memcpy(&drc_cfg.drc_para, &drc_para, sizeof(esp_ae_drc_para_t));
    esp_ae_drc_handle_t drc_handle = NULL;
    ret = esp_ae_drc_open(&drc_cfg, &drc_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ae_test_generate_sweep_signal(input_buffer, DRC_TEST_DURATION_MS, srate, -6.0f, bits, ch);
    ret = esp_ae_drc_process(drc_handle, num_samples / 2,
                             (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)), (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_drc_reset(drc_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_drc_process(drc_handle, num_samples / 2,
                             (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)), (esp_ae_sample_t)output_buffer_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, num_samples / 2 * ch * (bits >> 3));
    esp_ae_drc_close(drc_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("DRC interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sr); sr_idx++) {
        for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits); bit_idx++) {
            for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(ch); ch_idx++) {
                test_drc_consistency(sr[sr_idx], bits[bit_idx], ch[ch_idx]);
            }
        }
    }
}
