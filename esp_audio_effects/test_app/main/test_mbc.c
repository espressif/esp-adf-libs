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
#include "esp_ae_bit_cvt.h"
#include "esp_ae_mbc.h"
#include "ae_common.h"

#define TAG                  "TEST_MBC"
#define MBC_TEST_DURATION_MS 5000

static uint8_t  bits[]     = {16, 24, 32};
static uint32_t sr[]       = {48000, 8000};
static uint8_t  chan[]     = {1, 2};
static uint32_t fc[2][3]   = {{500, 5000, 15000}, {200, 1000, 3000}};
static uint32_t fft_size[] = {2048, 1024};

static float mbc_verify_band_fft(uint8_t *input_buffer, int num_samples, uint32_t sample_rate, uint8_t bits, uint8_t skip,
                                 uint32_t *fc_array, uint32_t fft_size, uint8_t band_idx, bool band_stop_test)
{
    float *spectrum = (float *)calloc(fft_size / 2, sizeof(float));
    TEST_ASSERT_NOT_EQUAL(spectrum, NULL);
    ae_test_analyze_frequency_response(input_buffer, num_samples, fft_size, bits, skip, spectrum);
    uint32_t down_fc[] = {20, fc_array[0], fc_array[1], fc_array[2]};
    uint32_t up_fc[] = {fc_array[0], fc_array[1], fc_array[2], sample_rate / 2};
    float passband_peak_db = -200.0f;
    for (int k = 0; k < fft_size / 2; k++) {
        int freq = k * sample_rate / fft_size;
        if (freq >= down_fc[band_idx] && freq < up_fc[band_idx]) {
            if (spectrum[k] > passband_peak_db) {
                passband_peak_db = spectrum[k];
            }
        }
    }
    if (band_stop_test) {
        float stopband_db = -200.0f;
        for (int i = 0; i < 4 && i != band_idx; i++) {
            float total_db = 0.0f;
            int count = 0;
            for (int k = 0; k < fft_size / 2; k++) {
                int freq = k * sample_rate / fft_size;
                if (i == band_idx) {
                    if (freq >= down_fc[i] && freq < up_fc[i]) {
                        if (spectrum[k] > passband_peak_db) {
                            passband_peak_db = spectrum[k];
                        }
                    }
                } else {
                    if (freq >= down_fc[i] && freq < up_fc[i]) {
                        total_db += spectrum[k];
                        count++;
                    }
                }
            }
            TEST_ASSERT_TRUE(count > 0);
            stopband_db = (count > 0) ? total_db / count : -200.0f;
            TEST_ASSERT_TRUE(stopband_db <= -25.0f);
        }
    }
    free(spectrum);
    spectrum = NULL;
    return passband_peak_db;
}

TEST_CASE("MBC branch test", "AUDIO_EFFECT")
{
    esp_ae_mbc_config_t mbc_cfg = {
        .sample_rate = 48000,
        .bits_per_sample = 32,
        .channel = 2,
        .fc = {80, 250, 1000},
        .mbc_para = {
            {.makeup_gain = 6, .attack_time = 1, .release_time = 200, .hold_time = 10, .ratio = 2.5, .knee_width = 5, .threshold = -20},
            {.makeup_gain = 6, .attack_time = 1, .release_time = 200, .hold_time = 10, .ratio = 2, .knee_width = 5, .threshold = -18},
            {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 10, .ratio = 1, .knee_width = 5, .threshold = -0.1},
            {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 10, .ratio = 1, .knee_width = 5, .threshold = -0.1},
        }};
    esp_ae_mbc_handle_t mbc_hd = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_mbc_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_open(NULL, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_open(&mbc_cfg, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    mbc_cfg.channel = 0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    mbc_cfg.channel = 1;
    mbc_cfg.bits_per_sample = 8;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    mbc_cfg.bits_per_sample = 16;
    mbc_cfg.sample_rate = 0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    mbc_cfg.sample_rate = 44100;
    mbc_cfg.fc[0] = 500;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    mbc_cfg.fc[1] = 1000;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    mbc_cfg.fc[2] = 900;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test9");
    mbc_cfg.fc[2] = 1500;
    mbc_cfg.mbc_para[1].threshold = 2.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test10");
    mbc_cfg.mbc_para[1].threshold = -102;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test11");
    mbc_cfg.mbc_para[1].threshold = -60;
    mbc_cfg.mbc_para[1].ratio = 0.5;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test12");
    mbc_cfg.mbc_para[1].ratio = 1.5;
    mbc_cfg.mbc_para[1].makeup_gain = 11.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    mbc_cfg.mbc_para[1].makeup_gain = -11.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test13");
    mbc_cfg.mbc_para[1].makeup_gain = 5.0;
    mbc_cfg.mbc_para[1].attack_time = 501;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test14");
    mbc_cfg.mbc_para[1].attack_time = 500;
    mbc_cfg.mbc_para[1].release_time = 501;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test15");
    mbc_cfg.mbc_para[1].release_time = 500;
    mbc_cfg.mbc_para[1].hold_time = 101;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test16");
    mbc_cfg.mbc_para[1].hold_time = 100;
    mbc_cfg.mbc_para[1].knee_width = -1.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    mbc_cfg.mbc_para[1].knee_width = 20.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "create handle");
    mbc_cfg.mbc_para[1].knee_width = 10.0;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_mbc_process");
    char samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_process(NULL, sample_num, (esp_ae_sample_t)samples, (esp_ae_sample_t)samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_process(mbc_hd, sample_num, NULL, (esp_ae_sample_t)samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_process(mbc_hd, sample_num, (esp_ae_sample_t)samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_process(mbc_hd, 0, (esp_ae_sample_t)samples, (esp_ae_sample_t)samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_deintlv_process");
    char samples1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_deintlv_process(NULL, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_deintlv_process(mbc_hd, sample_num, NULL, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_deintlv_process(mbc_hd, sample_num, (esp_ae_sample_t *)samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_deintlv_process(mbc_hd, 0, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_set_para");
    esp_ae_mbc_para_t mbc_para = {
        .makeup_gain = 0,
        .attack_time = 10,
        .release_time = 100,
        .hold_time = 10,
        .ratio = 1,
        .knee_width = 5,
        .threshold = -0.1};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_set_para(NULL, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    mbc_para.attack_time = 501;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    mbc_para.attack_time = 10;
    mbc_para.release_time = 501;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    mbc_para.release_time = 100;
    mbc_para.hold_time = 101;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    mbc_para.hold_time = 10;
    mbc_para.ratio = 0.5;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    mbc_para.ratio = 1.5;
    mbc_para.threshold = 100;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test9");
    mbc_para.threshold = -50;
    mbc_para.makeup_gain = 11;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    mbc_para.makeup_gain = -11;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test10");
    mbc_para.makeup_gain = 1;
    mbc_para.knee_width = -1;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    mbc_para.knee_width = 11;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_get_para");
    esp_ae_mbc_para_t mbc_para_tmp = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_get_para(NULL, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para_tmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_get_para(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, &mbc_para_tmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_get_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    mbc_para.knee_width = 0;
    ret = esp_ae_mbc_set_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_mbc_get_para(mbc_hd, ESP_AE_MBC_BAND_IDX_HIGH, &mbc_para_tmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(mbc_para_tmp.knee_width, mbc_para.knee_width);

    ESP_LOGI(TAG, "esp_ae_mbc_set_fc");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_set_fc(NULL, ESP_AE_MBC_FC_IDX_MID, 200);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_MAX, 200);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_LOW, 1001);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_MID, 500);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_MID, 1500);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_HIGH, 1000);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_get_fc");
    uint32_t fc;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_get_fc(NULL, ESP_AE_MBC_FC_IDX_HIGH, &fc);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_get_fc(mbc_hd, ESP_AE_MBC_FC_IDX_MAX, &fc);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_get_fc(mbc_hd, ESP_AE_MBC_FC_IDX_HIGH, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_set_fc(mbc_hd, ESP_AE_MBC_FC_IDX_HIGH, 1100);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_mbc_get_fc(mbc_hd, ESP_AE_MBC_FC_IDX_HIGH, &fc);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(fc, 1100);

    ESP_LOGI(TAG, "esp_ae_mbc_set_solo");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_set_solo(NULL, ESP_AE_MBC_BAND_IDX_MID_HIGH, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_set_solo(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_get_solo");
    bool enable_solo;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_get_solo(NULL, ESP_AE_MBC_BAND_IDX_MID_HIGH, &enable_solo);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_get_solo(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, &enable_solo);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_get_solo(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_set_solo(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_mbc_get_solo(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, &enable_solo);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(enable_solo, true);

    ESP_LOGI(TAG, "esp_ae_mbc_set_bypass");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_set_bypass(NULL, ESP_AE_MBC_BAND_IDX_MID_HIGH, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_set_bypass(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_mbc_get_bypass");
    bool enable_bypass;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mbc_get_bypass(NULL, ESP_AE_MBC_BAND_IDX_MID_HIGH, &enable_bypass);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mbc_get_bypass(mbc_hd, ESP_AE_MBC_BAND_IDX_MAX, &enable_bypass);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mbc_get_bypass(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mbc_set_bypass(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, true);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_mbc_get_bypass(mbc_hd, ESP_AE_MBC_BAND_IDX_MID_HIGH, &enable_bypass);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(enable_bypass, true);

    esp_ae_mbc_close(mbc_hd);
}

TEST_CASE("MBC crossover and inplace test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sr); sr_idx++) {
        for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(chan); ch_idx++) {
            for (int bits_idx = 0; bits_idx < AE_TEST_PARAM_NUM(bits); bits_idx++) {
                esp_ae_mbc_config_t mbc_cfg = {
                    .sample_rate = sr[sr_idx],
                    .bits_per_sample = bits[bits_idx],
                    .channel = chan[ch_idx],
                    .fc = {500, 1000, 3000},
                    .mbc_para = {
                        {.makeup_gain = 6, .attack_time = 1, .release_time = 200, .hold_time = 10, .ratio = 2.5, .knee_width = 5, .threshold = -20},
                        {.makeup_gain = 6, .attack_time = 1, .release_time = 200, .hold_time = 10, .ratio = 2, .knee_width = 5, .threshold = -18},
                        {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 10, .ratio = 1, .knee_width = 5, .threshold = -0.1},
                        {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 10, .ratio = 1, .knee_width = 5, .threshold = -0.1},
                    }};
                esp_ae_mbc_handle_t mbc_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_hd, NULL);
                esp_ae_mbc_handle_t mbc_deinter_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_deinter_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_deinter_hd, NULL);
                for (int j = 0; j < 4; j++) {
                    esp_ae_mbc_set_bypass(mbc_hd, j, true);
                    esp_ae_mbc_set_bypass(mbc_deinter_hd, j, true);
                }
                uint32_t total_samples = MBC_TEST_DURATION_MS * sr[sr_idx] / 1000;
                const int buffer_bytes = total_samples * (bits[bits_idx] >> 3);
                char *inbuf = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                ae_test_generate_sweep_signal(inbuf, MBC_TEST_DURATION_MS, sr[sr_idx], -6.0f, bits[bits_idx], chan[ch_idx]);
                char *inbuf_deinter[10] = {0};
                for (int j = 0; j < chan[ch_idx]; j++) {
                    inbuf_deinter[j] = calloc(1, buffer_bytes);
                    TEST_ASSERT_NOT_EQUAL(inbuf_deinter[j], NULL);
                }
                ae_test_stats_accumulator_t orig_acc = {0};
                ae_test_stats_accumulator_t proc_acc = {0};
                ae_test_stats_accumulator_t proc_acc_deinter = {0};
                ae_test_accumulate_audio_stats(&orig_acc, (uint8_t *)inbuf, total_samples * chan[ch_idx] * (bits[bits_idx] >> 3), bits[bits_idx]);
                esp_ae_deintlv_process(chan[ch_idx], bits[bits_idx], total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t *)inbuf_deinter);
                esp_ae_mbc_process(mbc_hd, total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t)inbuf);
                ae_test_accumulate_audio_stats(&proc_acc, (uint8_t *)inbuf, total_samples * chan[ch_idx] * (bits[bits_idx] >> 3), bits[bits_idx]);
                esp_ae_mbc_deintlv_process(mbc_deinter_hd, total_samples, (esp_ae_sample_t *)inbuf_deinter, (esp_ae_sample_t *)inbuf_deinter);
                esp_ae_intlv_process(chan[ch_idx], bits[bits_idx], total_samples, (esp_ae_sample_t *)inbuf_deinter, (esp_ae_sample_t)inbuf);
                ae_test_accumulate_audio_stats(&proc_acc_deinter, (uint8_t *)inbuf, total_samples * chan[ch_idx] * (bits[bits_idx] >> 3), bits[bits_idx]);
                bool is_pass = ae_test_analyze_audio_quality(&orig_acc, &proc_acc);
                TEST_ASSERT_EQUAL(is_pass, true);
                bool is_pass_deinter = ae_test_analyze_audio_quality(&orig_acc, &proc_acc_deinter);
                TEST_ASSERT_EQUAL(is_pass_deinter, true);
                free(inbuf);
                for (int j = 0; j < chan[ch_idx]; j++) {
                    free(inbuf_deinter[j]);
                }
                esp_ae_mbc_close(mbc_hd);
                esp_ae_mbc_close(mbc_deinter_hd);
            }
        }
    }
}

TEST_CASE("MBC basic function test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sr); sr_idx++) {
        for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(chan); ch_idx++) {
            for (int bits_idx = 0; bits_idx < AE_TEST_PARAM_NUM(bits); bits_idx++) {
                esp_ae_mbc_config_t mbc_cfg = {
                    .sample_rate = sr[sr_idx],
                    .bits_per_sample = bits[bits_idx],
                    .channel = chan[ch_idx],
                    .mbc_para = {
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -20},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -20},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -20},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -20},
                    }};
                memcpy(mbc_cfg.fc, fc[sr_idx], sizeof(uint32_t) * 3);
                esp_ae_mbc_handle_t mbc_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_hd, NULL);
                esp_ae_mbc_handle_t mbc_deinter_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_deinter_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_deinter_hd, NULL);
                // malloc
                uint32_t total_samples = MBC_TEST_DURATION_MS * sr[sr_idx] / 1000;
                const int buffer_bytes = total_samples * (bits[bits_idx] >> 3);
                uint8_t *inbuf = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                uint8_t *outbuf = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                ae_test_generate_sweep_signal(inbuf, MBC_TEST_DURATION_MS, sr[sr_idx], -6.0f, bits[bits_idx], chan[ch_idx]);
                uint8_t *inbuf_deinter[10] = {0};
                uint8_t *outbuf_deinter[10] = {0};
                for (int j = 0; j < chan[ch_idx]; j++) {
                    inbuf_deinter[j] = outbuf + j * buffer_bytes;
                    outbuf_deinter[j] = inbuf + j * buffer_bytes;
                }
                ESP_LOGI(TAG, "Interleave data compression test---bits: %d", bits[bits_idx]);
                for (int j = 0; j < 4; j++) {
                    esp_ae_mbc_set_solo(mbc_hd, j, true);
                    esp_ae_mbc_process(mbc_hd, total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t)outbuf);
                    esp_ae_mbc_set_solo(mbc_hd, j, false);
                    for (int l = 0; l < chan[ch_idx]; l++) {
                        float passband_indb = mbc_verify_band_fft(inbuf + l * (bits[bits_idx] >> 3), total_samples, sr[sr_idx], bits[bits_idx], chan[ch_idx],
                                                                  mbc_cfg.fc, fft_size[sr_idx], j, false);
                        float passband_outdb = mbc_verify_band_fft(outbuf + l * (bits[bits_idx] >> 3), total_samples, sr[sr_idx], bits[bits_idx], chan[ch_idx],
                                                                   mbc_cfg.fc, fft_size[sr_idx], j, true);
                        float ratio = (passband_indb - mbc_cfg.mbc_para[j].threshold) / (passband_outdb - mbc_cfg.mbc_para[j].threshold);
                        ESP_LOGD(TAG, "Interleave: Band %d, ratio: %f", j, ratio);
                        TEST_ASSERT_FLOAT_WITHIN(0.5f, mbc_cfg.mbc_para[j].ratio, ratio);
                    }
                }
                esp_ae_deintlv_process(chan[ch_idx], bits[bits_idx], total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t *)inbuf_deinter);
                for (int j = 0; j < 4; j++) {
                    esp_ae_mbc_set_solo(mbc_deinter_hd, j, true);
                    esp_ae_mbc_deintlv_process(mbc_deinter_hd, total_samples, (esp_ae_sample_t *)inbuf_deinter, (esp_ae_sample_t *)outbuf_deinter);
                    esp_ae_mbc_set_solo(mbc_deinter_hd, j, false);
                    for (int l = 0; l < chan[ch_idx]; l++) {
                        float passband_indb = mbc_verify_band_fft(inbuf_deinter[l], total_samples, sr[sr_idx], bits[bits_idx], 1,
                                                                  mbc_cfg.fc, fft_size[sr_idx], j, false);
                        float passband_outdb = mbc_verify_band_fft(outbuf_deinter[l], total_samples, sr[sr_idx], bits[bits_idx], 1,
                                                                   mbc_cfg.fc, fft_size[sr_idx], j, true);
                        float ratio = (passband_indb - mbc_cfg.mbc_para[j].threshold) / (passband_outdb - mbc_cfg.mbc_para[j].threshold);
                        ESP_LOGD(TAG, "Deinterleave: Band %d, ratio: %f", j, ratio);
                        TEST_ASSERT_FLOAT_WITHIN(0.5f, mbc_cfg.mbc_para[j].ratio, ratio);
                    }
                }
                free(inbuf);
                free(outbuf);
                esp_ae_mbc_close(mbc_hd);
                esp_ae_mbc_close(mbc_deinter_hd);
            }
        }
    }
}

TEST_CASE("MBC reset test", "AUDIO_EFFECT")
{
    uint32_t srate = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = MBC_TEST_DURATION_MS * srate / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;

    uint8_t *input_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);

    esp_ae_mbc_config_t mbc_cfg = {
        .sample_rate = srate,
        .bits_per_sample = bits,
        .channel = ch,
        .fc = {500, 5000, 15000},
        .mbc_para = {
            {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -20},
            {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -10},
            {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
            {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
        }};

    esp_ae_mbc_handle_t mbc_handle = NULL;
    ret = esp_ae_mbc_open(&mbc_cfg, &mbc_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(mbc_handle);

    ae_test_generate_sweep_signal(input_buffer, MBC_TEST_DURATION_MS, srate, -6.0f, bits, ch);

    ret = esp_ae_mbc_process(mbc_handle, num_samples / 2,
                             (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                             (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_mbc_reset(mbc_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_mbc_process(mbc_handle, num_samples / 2,
                             (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)),
                             (esp_ae_sample_t)output_buffer_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, num_samples / 2 * ch * (bits >> 3));

    esp_ae_mbc_close(mbc_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("MBC interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sr); sr_idx++) {
        for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(chan); ch_idx++) {
            for (int bits_idx = 0; bits_idx < AE_TEST_PARAM_NUM(bits); bits_idx++) {
                ESP_LOGI(TAG, "MBC crossover and inplace test---sr: %ld, ch: %d, bits: %d", sr[sr_idx], chan[ch_idx], bits[bits_idx]);
                esp_ae_mbc_config_t mbc_cfg = {
                    .sample_rate = sr[sr_idx],
                    .bits_per_sample = bits[bits_idx],
                    .channel = chan[ch_idx],
                    .fc = {500, 1000, 3000},
                    .mbc_para = {
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -20},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 2, .knee_width = 0, .threshold = -10},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
                        {.makeup_gain = 0, .attack_time = 1, .release_time = 50, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
                    }};
                esp_ae_mbc_handle_t mbc_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_hd, NULL);
                esp_ae_mbc_handle_t mbc_deinter_hd = NULL;
                esp_ae_mbc_open(&mbc_cfg, &mbc_deinter_hd);
                TEST_ASSERT_NOT_EQUAL(mbc_deinter_hd, NULL);
                uint32_t total_samples = 300 * sr[sr_idx] / 1000;
                const int buffer_bytes = total_samples * (bits[bits_idx] >> 3);
                char *inbuf = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *outbuf = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                ae_test_generate_sweep_signal(inbuf, 300, sr[sr_idx], -6.0f, bits[bits_idx], chan[ch_idx]);
                char *inbuf_deinter[10] = {0};
                for (int j = 0; j < chan[ch_idx]; j++) {
                    inbuf_deinter[j] = calloc(1, buffer_bytes);
                    TEST_ASSERT_NOT_EQUAL(inbuf_deinter[j], NULL);
                }
                char *outbuf_deinter[10] = {0};
                for (int j = 0; j < chan[ch_idx]; j++) {
                    outbuf_deinter[j] = calloc(1, buffer_bytes);
                    TEST_ASSERT_NOT_EQUAL(outbuf_deinter[j], NULL);
                }
                char *inbuf_deinter_cmp = calloc(1, buffer_bytes * chan[ch_idx]);
                TEST_ASSERT_NOT_EQUAL(inbuf_deinter_cmp, NULL);
                esp_ae_deintlv_process(chan[ch_idx], bits[bits_idx], total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t *)inbuf_deinter);
                esp_ae_mbc_process(mbc_hd, total_samples, (esp_ae_sample_t)inbuf, (esp_ae_sample_t)outbuf);

                esp_ae_mbc_deintlv_process(mbc_deinter_hd, total_samples, (esp_ae_sample_t *)inbuf_deinter, (esp_ae_sample_t *)outbuf_deinter);
                esp_ae_intlv_process(chan[ch_idx], bits[bits_idx], total_samples, (esp_ae_sample_t *)outbuf_deinter, (esp_ae_sample_t)inbuf_deinter_cmp);
                if (chan[ch_idx] > 1) {
                    TEST_ASSERT_EQUAL_MEMORY(outbuf_deinter[0], outbuf_deinter[1], total_samples * (bits[bits_idx] >> 3));
                }
                free(inbuf);
                free(outbuf);
                for (int j = 0; j < chan[ch_idx]; j++) {
                    free(inbuf_deinter[j]);
                    free(outbuf_deinter[j]);
                }
                free(inbuf_deinter_cmp);
                esp_ae_mbc_close(mbc_hd);
                esp_ae_mbc_close(mbc_deinter_hd);
            }
        }
    }
}
