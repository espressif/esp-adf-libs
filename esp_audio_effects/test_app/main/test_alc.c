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
#include "esp_ae_alc.h"
#include "esp_ae_data_weaver.h"
#include "ae_common.h"

#define TAG              "TEST_ALC"
#define TEST_DURATION_MS 5000

static uint32_t            sample_rate[]        = {8000, 44100};
static uint8_t             bits_per_sample[]    = {16, 24, 32};
static uint8_t             channel[]            = {1, 2};
static float               amplitude[]          = {-75.0, -63.0, -30.0, 0.0};
static uint8_t            *input_buffer         = NULL;
static uint8_t            *output_buffer        = NULL;
static uint8_t            *in_deinterlv[3]      = {0};
static uint8_t            *out_deinterlv[3]     = {0};
static esp_ae_alc_handle_t alc_handle           = NULL;
static esp_ae_alc_handle_t alc_deinterlv_handle = NULL;

typedef struct {
    uint32_t  sample_rate;
    uint8_t   bits_per_sample;
    uint8_t   channel;
    float     amplitude;
    int       num_samples;
    bool      is_inplace;
} alc_test_config_t;

typedef struct {
    int  gains[8];
    int  count;
} alc_test_gain_params_t;

static void alc_test_env_init(const alc_test_config_t *config)
{
    input_buffer = (uint8_t *)malloc(config->num_samples * (config->bits_per_sample >> 3) * config->channel);
    TEST_ASSERT_NOT_NULL(input_buffer);
    if (config->is_inplace) {
        output_buffer = input_buffer;
    } else {
        output_buffer = (uint8_t *)malloc(config->num_samples * (config->bits_per_sample >> 3) * config->channel);
        TEST_ASSERT_NOT_NULL(output_buffer);
    }
    for (int i = 0; i < config->channel; i++) {
        in_deinterlv[i] = (uint8_t *)malloc(config->num_samples * (config->bits_per_sample >> 3));
        TEST_ASSERT_NOT_NULL(in_deinterlv[i]);
        if (config->is_inplace) {
            out_deinterlv[i] = in_deinterlv[i];
        } else {
            out_deinterlv[i] = (uint8_t *)malloc(config->num_samples * (config->bits_per_sample >> 3));
            TEST_ASSERT_NOT_NULL(out_deinterlv[i]);
        }
    }
    esp_ae_alc_cfg_t alc_config = {
        .sample_rate = config->sample_rate,
        .channel = config->channel,
        .bits_per_sample = config->bits_per_sample,
    };
    esp_ae_alc_open(&alc_config, &alc_handle);
    TEST_ASSERT_NOT_NULL(alc_handle);
    esp_ae_alc_open(&alc_config, &alc_deinterlv_handle);
    TEST_ASSERT_NOT_NULL(alc_deinterlv_handle);
}

static void alc_test_env_deinit(bool is_inplace)
{
    if (input_buffer) {
        free(input_buffer);
        input_buffer = NULL;
    }
    if (!is_inplace && output_buffer) {
        free(output_buffer);
        output_buffer = NULL;
    }
    for (int i = 0; i < 3; i++) {
        if (in_deinterlv[i]) {
            free(in_deinterlv[i]);
            in_deinterlv[i] = NULL;
        }
        if (!is_inplace && out_deinterlv[i]) {
            free(out_deinterlv[i]);
            out_deinterlv[i] = NULL;
        }
    }
    if (alc_handle) {
        esp_ae_alc_close(alc_handle);
        alc_handle = NULL;
    }
    if (alc_deinterlv_handle) {
        esp_ae_alc_close(alc_deinterlv_handle);
        alc_deinterlv_handle = NULL;
    }
}

static void alc_volume_test(uint32_t sample_rate, uint8_t bits, uint8_t ch,
                            int num_samples, int gain, float amplitude)
{
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    ESP_LOGI(TAG, "Test alc sample_rate: %ld, bits: %d, ch: %d, gain: %d dB, amplitude: %f dBFS",
             sample_rate, bits, ch, gain, amplitude);
    ae_test_generate_sine_signal(input_buffer, TEST_DURATION_MS, sample_rate, amplitude, bits, ch, sample_rate / 5);
    esp_ae_deintlv_process(ch, bits, num_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t *)in_deinterlv);
    for (int i = 0; i < ch; i++) {
        ret = esp_ae_alc_set_gain(alc_handle, i, gain);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    }
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_alc_process(alc_handle, num_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    float output_level = ae_test_calculate_rms_dbfs(output_buffer + num_samples / 2 * (bits >> 3) * ch, num_samples / 2, bits, ch) + 3.0f;
    ESP_LOGI(TAG, "INTERLEAVED: Input: -6dBFS, Output: %.2fdBFS (expected: ~%02f dBFS)",
             output_level, (float)gain + amplitude > 0 ? 0 : (float)gain + amplitude);
    if (gain + amplitude > -70.0f && amplitude > -70.0f) {
        TEST_ASSERT_FLOAT_WITHIN(1.0f, (float)gain + amplitude > 0 ? 0 : (float)gain + amplitude, output_level);
    } else {
        TEST_ASSERT_LESS_THAN(-70.0f, output_level);
    }
    for (int i = 0; i < ch; i++) {
        ret = esp_ae_alc_set_gain(alc_deinterlv_handle, i, gain);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    }
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_alc_deintlv_process(alc_deinterlv_handle, num_samples,
                                     (esp_ae_sample_t *)in_deinterlv, (esp_ae_sample_t *)out_deinterlv);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_intlv_process(ch, bits, num_samples, (esp_ae_sample_t *)out_deinterlv, (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    output_level = ae_test_calculate_rms_dbfs(output_buffer + num_samples / 2 * (bits >> 3) * ch, num_samples / 2, bits, ch) + 3.0f;
    ESP_LOGI(TAG, "DEINTERLEAVED: Input: -6dBFS, Output: %.2fdBFS (expected: ~%02f dBFS)",
             output_level, (float)gain + amplitude > 0 ? 0 : (float)gain + amplitude);
    if (gain + amplitude > -70.0f && amplitude > -70.0f) {
        TEST_ASSERT_FLOAT_WITHIN(1.0f, (float)gain + amplitude > 0 ? 0 : (float)gain + amplitude, output_level);
    } else {
        TEST_ASSERT_LESS_THAN(-70.0f, output_level);
    }
}

static void alc_gain_sequence_test(const alc_test_config_t *config, const alc_test_gain_params_t *gain_params)
{
    for (int i = 0; i < gain_params->count; i++) {
        alc_volume_test(config->sample_rate, config->bits_per_sample, config->channel,
                        config->num_samples, gain_params->gains[i], config->amplitude);
    }
}

static void alc_run_test_matrix(const alc_test_gain_params_t *gain_params, const char *test_name)
{
    ESP_LOGI(TAG, "Starting %s", test_name);
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                for (int l = 0; l < AE_TEST_PARAM_NUM(amplitude); l++) {
                    alc_test_config_t config = {
                        .sample_rate = sample_rate[i],
                        .bits_per_sample = bits_per_sample[j],
                        .channel = channel[k],
                        .amplitude = amplitude[l],
                        .num_samples = TEST_DURATION_MS * sample_rate[i] / 1000,
                        .is_inplace = false};
                    alc_test_env_init(&config);
                    alc_gain_sequence_test(&config, gain_params);
                    alc_test_env_deinit(false);
                }
            }
        }
    }
    ESP_LOGI(TAG, "Completed %s", test_name);
}

static void test_alc_consistency(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channel)
{
    ESP_LOGI(TAG, "Testing ALC consistency: %ld Hz, %d bits, %d channels", sample_rate, bits_per_sample, channel);
    esp_ae_alc_cfg_t config = {
        .sample_rate = sample_rate,
        .channel = channel,
        .bits_per_sample = bits_per_sample,
    };
    esp_ae_alc_handle_t hd1 = NULL;
    esp_ae_alc_handle_t hd2 = NULL;
    esp_err_t ret = esp_ae_alc_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_alc_open(&config, &hd2);
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

    ret = esp_ae_alc_set_gain(hd1, 0, -12);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_alc_set_gain(hd2, 0, -12);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_alc_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_alc_deintlv_process(hd2, sample_count, (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ret = esp_ae_intlv_process(channel, bits_per_sample, sample_count,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, sample_count * output_bytes_per_sample, 1);

    esp_ae_alc_close(hd1);
    esp_ae_alc_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("Alc branch test", "AUDIO_EFFECT")
{
    esp_ae_alc_cfg_t config;
    config.channel = 1;
    config.bits_per_sample = 32;
    config.sample_rate = 44100;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_alc_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_open(NULL, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.channel = 0;
    ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.channel = 1;
    config.bits_per_sample = 8;
    ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    config.bits_per_sample = 16;
    config.sample_rate = 0;
    ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    // create alc_handle
    ESP_LOGI(TAG, "create alc_handle");
    config.channel = 1;
    config.bits_per_sample = 16;
    config.sample_rate = 100;
    ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_alc_set_gain");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_set_gain(NULL, 0, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_set_gain(alc_handle, 0, 65);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_alc_get_gain");
    ESP_LOGI(TAG, "test1");
    int8_t gain = 0;
    ret = esp_ae_alc_get_gain(NULL, 0, &gain);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_get_gain(alc_handle, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    esp_ae_alc_set_gain(alc_handle, 0, 10);
    ret = esp_ae_alc_get_gain(alc_handle, 0, &gain);
    ESP_LOGI(TAG, "gain:%d", gain);
    TEST_ASSERT_EQUAL(gain, 10);

    ESP_LOGI(TAG, "esp_ae_alc_process");
    char samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_process(NULL, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_process(alc_handle, sample_num, NULL, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_alc_process(alc_handle, sample_num, samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_alc_process(alc_handle, 0, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_alc_process(alc_handle, 10, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_alc_deintlv_process");
    char samples1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_deintlv_process(NULL, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, NULL, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, (esp_ae_sample_t *)samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_alc_deintlv_process(alc_handle, 0, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, (esp_ae_sample_t *)samples1, (esp_ae_sample_t *)samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_alc_close(alc_handle);
}

TEST_CASE("ALC add volume test", "AUDIO_EFFECT")
{
    alc_test_gain_params_t gain_params = {
        .gains = {3, 6, 12, 30, 63, 0},
        .count = 6};
    alc_run_test_matrix(&gain_params, "ALC add volume test");
}

TEST_CASE("ALC sub volume test", "AUDIO_EFFECT")
{
    alc_test_gain_params_t gain_params = {
        .gains = {-5, -15, -30, -64},
        .count = 4};
    alc_run_test_matrix(&gain_params, "ALC sub volume test");
}

TEST_CASE("ALC boundary test", "AUDIO_EFFECT")
{
    alc_test_gain_params_t gain_params = {
        .gains = {63, -64, 0, -64, 63},
        .count = 5};
    alc_run_test_matrix(&gain_params, "ALC boundary test");
}

TEST_CASE("ALC stay add sub test", "AUDIO_EFFECT")
{
    alc_test_gain_params_t gain_params = {
        .gains = {0, 6, -12},
        .count = 3};
    alc_run_test_matrix(&gain_params, "ALC stay add sub test");
}

TEST_CASE("ALC stay sub add test", "AUDIO_EFFECT")
{
    alc_test_gain_params_t gain_params = {
        .gains = {0, -12, 6},
        .count = 3};
    alc_run_test_matrix(&gain_params, "ALC stay sub add test");
}

TEST_CASE("ALC inplace test", "AUDIO_EFFECT")
{
    alc_test_config_t config = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 1,
        .amplitude = -6.0f,
        .num_samples = TEST_DURATION_MS * 44100 / 1000,
        .is_inplace = true};
    alc_test_gain_params_t gain_params = {
        .gains = {0, -12, 6},
        .count = 3};
    alc_test_env_init(&config);
    alc_gain_sequence_test(&config, &gain_params);
    alc_test_env_deinit(true);
}

TEST_CASE("ALC reset test", "AUDIO_EFFECT")
{
    uint32_t srate = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = TEST_DURATION_MS * srate / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    input_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    output_buffer = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(num_samples * ch, bits >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);
    esp_ae_alc_cfg_t alc_config = {
        .sample_rate = srate,
        .channel = ch,
        .bits_per_sample = bits,
    };
    esp_ae_alc_open(&alc_config, &alc_handle);
    TEST_ASSERT_NOT_NULL(alc_handle);
    ae_test_generate_sweep_signal(input_buffer, TEST_DURATION_MS, srate, -6.0f, bits, ch);
    for (int i = 0; i < ch; i++) {
        ret = esp_ae_alc_set_gain(alc_handle, i, -12);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    }
    ret = esp_ae_alc_process(alc_handle, num_samples / 2, (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)), (esp_ae_sample_t)output_buffer);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_alc_reset(alc_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_alc_process(alc_handle, num_samples / 2, (esp_ae_sample_t)(input_buffer + num_samples / 2 * ch * (bits >> 3)), (esp_ae_sample_t)output_buffer_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, num_samples / 2 * ch * (bits >> 3));
    esp_ae_alc_close(alc_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("ALC interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int sr_idx = 0; sr_idx < AE_TEST_PARAM_NUM(sample_rate); sr_idx++) {
        for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits_per_sample); bit_idx++) {
            for (int ch_idx = 0; ch_idx < AE_TEST_PARAM_NUM(channel); ch_idx++) {
                test_alc_consistency(sample_rate[sr_idx], bits_per_sample[bit_idx], channel[ch_idx]);
            }
        }
    }
}
