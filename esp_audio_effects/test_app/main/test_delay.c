/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
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
#include "esp_ae_delay.h"
#include "esp_ae_data_weaver.h"
#include "ae_common.h"

#define TAG              "TEST_DELAY"
#define TEST_DURATION_MS 500

static uint32_t sample_rate[]    = {16000, 44100};
static uint8_t  bits_per_sample[] = {16, 24, 32};
static uint8_t  channel[]        = {1, 2};

TEST_CASE("Delay branch test", "AUDIO_EFFECT")
{
    esp_ae_delay_cfg_t config = {
        .sample_rate = 44100,
        .channel = 1,
        .bits_per_sample = 16,
        .max_delay_ms = 500,
        .delay_para = {
            .delay_time_ms = 250,
            .feedback = 0.5f,
            .mix = 0.35f,
        },
    };
    esp_ae_delay_handle_t handle = NULL;
    int ret = 0;

    ESP_LOGI(TAG, "esp_ae_delay_open");
    ESP_LOGI(TAG, "test1 - NULL cfg");
    ret = esp_ae_delay_open(NULL, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test2 - NULL handle");
    ret = esp_ae_delay_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test3 - invalid channel");
    config.channel = 0;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test4 - invalid bits_per_sample");
    config.channel = 1;
    config.bits_per_sample = 8;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test5 - invalid sample_rate");
    config.bits_per_sample = 16;
    config.sample_rate = 0;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test6 - invalid max_delay_ms");
    config.sample_rate = 44100;
    config.max_delay_ms = 1001;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "test7 - delay_time > max_delay");
    config.max_delay_ms = 100;
    config.delay_para.delay_time_ms = 200;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "create handle");
    config.max_delay_ms = 500;
    config.delay_para.delay_time_ms = 250;
    ret = esp_ae_delay_open(&config, &handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_NOT_NULL(handle);

    ESP_LOGI(TAG, "esp_ae_delay_set_delay_time");
    ret = esp_ae_delay_set_delay_time(NULL, 100);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_set_delay_time(handle, 100);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_delay_get_delay_time");
    uint16_t delay_time = 0;
    ret = esp_ae_delay_get_delay_time(NULL, &delay_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_delay_time(handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_delay_time(handle, &delay_time);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(100, delay_time);

    ESP_LOGI(TAG, "esp_ae_delay_set_feedback");
    ret = esp_ae_delay_set_feedback(NULL, 0.5f);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_set_feedback(handle, 0.3f);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_delay_get_feedback");
    float feedback = 0;
    ret = esp_ae_delay_get_feedback(NULL, &feedback);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_feedback(handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_feedback(handle, &feedback);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, feedback);

    ESP_LOGI(TAG, "esp_ae_delay_set_mix");
    ret = esp_ae_delay_set_mix(NULL, 0.35f);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_set_mix(handle, 0.5f);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_delay_get_mix");
    float mix = 0;
    ret = esp_ae_delay_get_mix(NULL, &mix);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_mix(handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_get_mix(handle, &mix);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, mix);

    ESP_LOGI(TAG, "esp_ae_delay_process");
    char samples[512];
    int sample_num = 16;
    ret = esp_ae_delay_process(NULL, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_process(handle, sample_num, NULL, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_process(handle, sample_num, samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_process(handle, 0, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_process(handle, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_delay_deintlv_process");
    char deintlv_buf[512] = {0};
    esp_ae_sample_t in_arr[1] = {deintlv_buf};
    esp_ae_sample_t out_arr[1] = {deintlv_buf};
    ret = esp_ae_delay_deintlv_process(NULL, sample_num, in_arr, out_arr);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_deintlv_process(handle, sample_num, NULL, out_arr);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_deintlv_process(handle, sample_num, in_arr, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_deintlv_process(handle, 0, in_arr, out_arr);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_deintlv_process(handle, sample_num, in_arr, out_arr);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_delay_reset");
    ret = esp_ae_delay_reset(NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ret = esp_ae_delay_reset(handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    esp_ae_delay_close(handle);
}

TEST_CASE("Delay basic function test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                uint32_t sr = sample_rate[i];
                uint8_t bits = bits_per_sample[j];
                uint8_t ch = channel[k];
                uint32_t num_samples = TEST_DURATION_MS * sr / 1000;
                int sample_bytes = bits >> 3;
                int frame_size = num_samples * ch * sample_bytes;

                ESP_LOGI(TAG, "Test delay: %ld Hz, %d bits, %d ch", sr, bits, ch);

                uint8_t *input_buf = (uint8_t *)calloc(1, frame_size);
                uint8_t *output_buf = (uint8_t *)calloc(1, frame_size);
                TEST_ASSERT_NOT_NULL(input_buf);
                TEST_ASSERT_NOT_NULL(output_buf);

                ae_test_generate_sine_signal(input_buf, TEST_DURATION_MS, sr, -6.0f, bits, ch, 1000.0f);

                esp_ae_delay_cfg_t cfg = {
                    .sample_rate = sr,
                    .channel = ch,
                    .bits_per_sample = bits,
                    .max_delay_ms = 300,
                    .delay_para = {
                        .delay_time_ms = 100,
                        .feedback = 0.5f,
                        .mix = 0.35f,
                    },
                };
                esp_ae_delay_handle_t handle = NULL;
                esp_ae_err_t ret = esp_ae_delay_open(&cfg, &handle);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                TEST_ASSERT_NOT_NULL(handle);

                uint32_t chunk_size = 256;
                uint32_t processed = 0;
                while (processed < num_samples) {
                    uint32_t to_process = (num_samples - processed) > chunk_size ? chunk_size : (num_samples - processed);
                    int offset = processed * ch * sample_bytes;
                    ret = esp_ae_delay_process(handle, to_process,
                                              (esp_ae_sample_t)(input_buf + offset),
                                              (esp_ae_sample_t)(output_buf + offset));
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    processed += to_process;
                }

                float input_rms = ae_test_calculate_rms_dbfs(input_buf + frame_size / 2,
                                                             num_samples / 2, bits, ch);
                float output_rms = ae_test_calculate_rms_dbfs(output_buf + frame_size / 2,
                                                              num_samples / 2, bits, ch);
                ESP_LOGI(TAG, "Input RMS: %.2f dBFS, Output RMS: %.2f dBFS", input_rms, output_rms);
                TEST_ASSERT_TRUE(output_rms > -96.0f);

                bool output_differs = (memcmp(input_buf, output_buf, frame_size) != 0);
                TEST_ASSERT_TRUE(output_differs);

                esp_ae_delay_close(handle);
                free(input_buf);
                free(output_buf);
            }
        }
    }
}

TEST_CASE("Delay inplace test", "AUDIO_EFFECT")
{
    for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
        uint32_t sr = 44100;
        uint8_t bits = bits_per_sample[j];
        uint8_t ch = 2;
        uint32_t num_samples = TEST_DURATION_MS * sr / 1000;
        int sample_bytes = bits >> 3;
        int frame_size = num_samples * ch * sample_bytes;

        ESP_LOGI(TAG, "Inplace test: %d bits", bits);

        uint8_t *buffer = (uint8_t *)calloc(1, frame_size);
        uint8_t *ref_output = (uint8_t *)calloc(1, frame_size);
        TEST_ASSERT_NOT_NULL(buffer);
        TEST_ASSERT_NOT_NULL(ref_output);

        ae_test_generate_sine_signal(buffer, TEST_DURATION_MS, sr, -6.0f, bits, ch, 1000.0f);

        esp_ae_delay_cfg_t cfg = {
            .sample_rate = sr,
            .channel = ch,
            .bits_per_sample = bits,
            .max_delay_ms = 300,
            .delay_para = {
                .delay_time_ms = 100,
                .feedback = 0.5f,
                .mix = 0.35f,
            },
        };

        esp_ae_delay_handle_t handle1 = NULL;
        esp_ae_delay_handle_t handle2 = NULL;
        esp_ae_err_t ret = esp_ae_delay_open(&cfg, &handle1);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        ret = esp_ae_delay_open(&cfg, &handle2);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

        uint8_t *input_copy = (uint8_t *)calloc(1, frame_size);
        TEST_ASSERT_NOT_NULL(input_copy);
        memcpy(input_copy, buffer, frame_size);

        ret = esp_ae_delay_process(handle1, num_samples,
                                   (esp_ae_sample_t)input_copy, (esp_ae_sample_t)ref_output);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

        ret = esp_ae_delay_process(handle2, num_samples,
                                   (esp_ae_sample_t)buffer, (esp_ae_sample_t)buffer);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

        TEST_ASSERT_EQUAL_MEMORY(ref_output, buffer, frame_size);

        esp_ae_delay_close(handle1);
        esp_ae_delay_close(handle2);
        free(buffer);
        free(ref_output);
        free(input_copy);
    }
}

TEST_CASE("Delay reset test", "AUDIO_EFFECT")
{
    uint32_t sr = 48000;
    uint8_t ch = 2;
    uint8_t bits = 16;
    uint32_t num_samples = TEST_DURATION_MS * sr / 1000;
    int sample_bytes = bits >> 3;
    int frame_size = num_samples * ch * sample_bytes;

    uint8_t *input_buf = (uint8_t *)calloc(1, frame_size);
    uint8_t *output_buf1 = (uint8_t *)calloc(1, frame_size);
    uint8_t *output_buf2 = (uint8_t *)calloc(1, frame_size);
    TEST_ASSERT_NOT_NULL(input_buf);
    TEST_ASSERT_NOT_NULL(output_buf1);
    TEST_ASSERT_NOT_NULL(output_buf2);

    ae_test_generate_sine_signal(input_buf, TEST_DURATION_MS, sr, -6.0f, bits, ch, 1000.0f);

    esp_ae_delay_cfg_t cfg = {
        .sample_rate = sr,
        .channel = ch,
        .bits_per_sample = bits,
        .max_delay_ms = 300,
        .delay_para = {
            .delay_time_ms = 100,
            .feedback = 0.5f,
            .mix = 0.35f,
        },
    };
    esp_ae_delay_handle_t handle = NULL;
    esp_ae_err_t ret = esp_ae_delay_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_process(handle, num_samples, (esp_ae_sample_t)input_buf, (esp_ae_sample_t)output_buf1);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_reset(handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_process(handle, num_samples, (esp_ae_sample_t)input_buf, (esp_ae_sample_t)output_buf2);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    TEST_ASSERT_EQUAL_MEMORY(output_buf1, output_buf2, frame_size);

    esp_ae_delay_close(handle);
    free(input_buf);
    free(output_buf1);
    free(output_buf2);
}

TEST_CASE("Delay interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int i = 0; i < AE_TEST_PARAM_NUM(sample_rate); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(bits_per_sample); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(channel); k++) {
                uint32_t sr = sample_rate[i];
                uint8_t bits = bits_per_sample[j];
                uint8_t ch = channel[k];

                ESP_LOGI(TAG, "Consistency test: %ld Hz, %d bits, %d ch", sr, bits, ch);

                esp_ae_delay_cfg_t cfg = {
                    .sample_rate = sr,
                    .channel = ch,
                    .bits_per_sample = bits,
                    .max_delay_ms = 200,
                    .delay_para = {
                        .delay_time_ms = 50,
                        .feedback = 0.4f,
                        .mix = 0.35f,
                    },
                };

                esp_ae_delay_handle_t hd1 = NULL;
                esp_ae_delay_handle_t hd2 = NULL;
                esp_ae_err_t ret = esp_ae_delay_open(&cfg, &hd1);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_delay_open(&cfg, &hd2);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                const int duration_ms = 100;
                int sample_bytes = bits >> 3;
                uint32_t sample_count = sr * duration_ms / 1000;

                void *interlv_in = calloc(sample_count, sample_bytes * ch);
                void *interlv_out = calloc(sample_count, sample_bytes * ch);
                TEST_ASSERT_NOT_NULL(interlv_in);
                TEST_ASSERT_NOT_NULL(interlv_out);

                void *deinterlv_in[2] = {0};
                void *deinterlv_out[2] = {0};
                for (int c = 0; c < ch; c++) {
                    deinterlv_in[c] = calloc(sample_count, sample_bytes);
                    deinterlv_out[c] = calloc(sample_count, sample_bytes);
                    TEST_ASSERT_NOT_NULL(deinterlv_in[c]);
                    TEST_ASSERT_NOT_NULL(deinterlv_out[c]);
                }
                void *deinterlv_out_cmp = calloc(sample_count, sample_bytes * ch);
                TEST_ASSERT_NOT_NULL(deinterlv_out_cmp);

                ae_test_generate_sweep_signal(interlv_in, duration_ms, sr, 0.0f, bits, ch);

                ret = esp_ae_deintlv_process(ch, bits, sample_count,
                                             (esp_ae_sample_t)interlv_in, (esp_ae_sample_t *)deinterlv_in);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                ret = esp_ae_delay_process(hd1, sample_count, (esp_ae_sample_t)interlv_in, (esp_ae_sample_t)interlv_out);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                ret = esp_ae_delay_deintlv_process(hd2, sample_count,
                                                   (esp_ae_sample_t *)deinterlv_in, (esp_ae_sample_t *)deinterlv_out);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                ret = esp_ae_intlv_process(ch, bits, sample_count,
                                           (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                TEST_ASSERT_EQUAL_MEMORY(interlv_out, deinterlv_out_cmp, sample_count * sample_bytes * ch);

                esp_ae_delay_close(hd1);
                esp_ae_delay_close(hd2);
                free(interlv_in);
                free(interlv_out);
                for (int c = 0; c < ch; c++) {
                    free(deinterlv_in[c]);
                    free(deinterlv_out[c]);
                }
                free(deinterlv_out_cmp);
            }
        }
    }
}

TEST_CASE("Delay parameter change test", "AUDIO_EFFECT")
{
    uint32_t sr = 44100;
    uint8_t bits = 16;
    uint8_t ch = 1;
    uint32_t num_samples = TEST_DURATION_MS * sr / 1000;
    int sample_bytes = bits >> 3;
    int frame_size = num_samples * ch * sample_bytes;

    uint8_t *input_buf = (uint8_t *)calloc(1, frame_size);
    uint8_t *output_buf1 = (uint8_t *)calloc(1, frame_size);
    uint8_t *output_buf2 = (uint8_t *)calloc(1, frame_size);
    TEST_ASSERT_NOT_NULL(input_buf);
    TEST_ASSERT_NOT_NULL(output_buf1);
    TEST_ASSERT_NOT_NULL(output_buf2);

    ae_test_generate_sine_signal(input_buf, TEST_DURATION_MS, sr, -6.0f, bits, ch, 1000.0f);

    esp_ae_delay_cfg_t cfg = {
        .sample_rate = sr,
        .channel = ch,
        .bits_per_sample = bits,
        .max_delay_ms = 500,
        .delay_para = {
            .delay_time_ms = 100,
            .feedback = 0.3f,
            .mix = 0.35f,
        },
    };

    esp_ae_delay_handle_t handle1 = NULL;
    esp_ae_delay_handle_t handle2 = NULL;
    esp_ae_err_t ret = esp_ae_delay_open(&cfg, &handle1);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_delay_open(&cfg, &handle2);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_process(handle1, num_samples, (esp_ae_sample_t)input_buf, (esp_ae_sample_t)output_buf1);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_set_delay_time(handle2, 400);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_delay_set_feedback(handle2, 0.7f);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_delay_process(handle2, num_samples, (esp_ae_sample_t)input_buf, (esp_ae_sample_t)output_buf2);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    bool outputs_differ = (memcmp(output_buf1, output_buf2, frame_size) != 0);
    TEST_ASSERT_TRUE(outputs_differ);

    esp_ae_delay_close(handle1);
    esp_ae_delay_close(handle2);
    free(input_buf);
    free(output_buf1);
    free(output_buf2);
}

TEST_CASE("Delay echo timing test", "AUDIO_EFFECT")
{
    uint32_t sr = 16000;
    uint8_t bits = 16;
    uint8_t ch = 1;
    uint16_t delay_ms = 100;
    uint32_t delay_samples = sr * delay_ms / 1000;
    uint32_t total_samples = sr;
    int sample_bytes = bits >> 3;
    int frame_size = total_samples * ch * sample_bytes;

    int16_t *input_buf = (int16_t *)calloc(total_samples, sizeof(int16_t));
    int16_t *output_buf = (int16_t *)calloc(total_samples, sizeof(int16_t));
    TEST_ASSERT_NOT_NULL(input_buf);
    TEST_ASSERT_NOT_NULL(output_buf);

    input_buf[0] = 16000;

    esp_ae_delay_cfg_t cfg = {
        .sample_rate = sr,
        .channel = ch,
        .bits_per_sample = bits,
        .max_delay_ms = 500,
        .delay_para = {
            .delay_time_ms = delay_ms,
            .feedback = 0.5f,
            .mix = 0.5f,
        },
    };
    esp_ae_delay_handle_t handle = NULL;
    esp_ae_err_t ret = esp_ae_delay_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_delay_process(handle, total_samples, (esp_ae_sample_t)input_buf, (esp_ae_sample_t)output_buf);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    TEST_ASSERT_NOT_EQUAL(0, output_buf[0]);

    bool echo_found = false;
    for (uint32_t s = delay_samples - 2; s <= delay_samples + 2; s++) {
        if (s < total_samples && output_buf[s] != 0) {
            echo_found = true;
            ESP_LOGI(TAG, "Echo found at sample %ld (expected %ld), value: %d", s, delay_samples, output_buf[s]);
            break;
        }
    }
    TEST_ASSERT_TRUE(echo_found);

    esp_ae_delay_close(handle);
    free(input_buf);
    free(output_buf);
}
