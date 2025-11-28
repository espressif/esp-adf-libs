/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "test_utils.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ae_common.h"
#include "test_common.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_sonic.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#define TAG "TEST_SONIC"

#define HTTP_SERVER_URL_DOWNLOAD "http://10.18.20.184:8080/audio_files/audio_test_dataset/voice"
#define HTTP_SERVER_URL_UPLOAD   "http://10.18.20.184:8080/upload?folder=ae_test/sonic_test"

static char file_name[][100] = {
    "manch_48000_1_16_10",
    "manen_48000_1_16_10",
    "manloud_48000_1_16_10",
    "womanch_16000_1_16_6",
    "womanen_48000_1_16_10",
    "womanloud_48000_1_16_10",
};

static uint8_t bits_per_sample[] = {16, 24, 32};

static int process_audio_data_http(ae_http_context_t *infile, ae_http_context_t *outfile, void *sonic_handle,
                                   ae_audio_info_t *audio_info, int output_bits)
{
    int in_num = 512;
    int out_num = 512;
    size_t total_samples = 0;
    short *inbuf = calloc(1, in_num * audio_info->channels * sizeof(short));
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    short *outbuf = calloc(1, out_num * audio_info->channels * (output_bits >> 3));
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    void *inbuf_target = NULL;
    esp_ae_bit_cvt_handle_t cvt_in_handle = NULL;
    if (output_bits != 16) {
        inbuf_target = calloc(1, in_num * audio_info->channels * (output_bits >> 3));
        TEST_ASSERT_NOT_EQUAL(inbuf_target, NULL);
        esp_ae_bit_cvt_cfg_t cvt_in_cfg = {audio_info->sample_rate, audio_info->channels, 16, output_bits};
        esp_ae_bit_cvt_open(&cvt_in_cfg, &cvt_in_handle);
    }
    esp_ae_sonic_in_data_t in_samples = {0};
    esp_ae_sonic_out_data_t out_samples = {0};
    int in_read = 0;
    int sample_num = 0;
    int remain_num = 0;
    int ret = 0;
    while ((in_read = ae_http_read(inbuf, 1, in_num * audio_info->channels * sizeof(short), infile)) > 0) {
        sample_num = in_read / (audio_info->channels * sizeof(short));
        remain_num = sample_num;
        in_samples.samples = inbuf;
        out_samples.samples = outbuf;
        if (output_bits != 16) {
            esp_ae_bit_cvt_process(cvt_in_handle, sample_num, inbuf, inbuf_target);
            in_samples.samples = inbuf_target;
        }
        in_samples.num = sample_num;
        out_samples.needed_num = 512;

        void *in = in_samples.samples;
        while (remain_num > 0 || out_samples.out_num > 0) {
            ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
            TEST_ASSERT_EQUAL(ret, 0);
            if (out_samples.out_num > 0) {
                ae_http_write(out_samples.samples, 1, out_samples.out_num * (output_bits >> 3) * audio_info->channels, false, outfile);
                total_samples += out_samples.out_num;
            }
            int consume_bytes = in_samples.consume_num * audio_info->channels * (output_bits >> 3);
            in = (uint8_t *)in + consume_bytes;
            remain_num -= in_samples.consume_num;
            in_samples.num = remain_num;
            in_samples.samples = in;
        }
    }
    ae_http_write(NULL, 0, 0, true, outfile);
    if (cvt_in_handle) {
        esp_ae_bit_cvt_close(cvt_in_handle);
    }
    free(inbuf);
    free(outbuf);
    if (inbuf_target) {
        free(inbuf_target);
    }
    return 1;
}

static int sonic_http_test(const char *filename, float speed, float pitch, int target_bits)
{
    ESP_LOGI(TAG, "Starting HTTP test for file: %s, speed: %.2f, pitch: %.2f, bits: %d",
             filename, speed, pitch, target_bits);
    ae_http_context_t input_ctx = {0};
    ae_http_context_t output_ctx = {0};

    int ret = ae_http_download_init(&input_ctx, filename, HTTP_SERVER_URL_DOWNLOAD);
    TEST_ASSERT_EQUAL(ret, ESP_OK);

    long data_offset = 0;
    ae_audio_info_t audio_info = {0};
    uint32_t data_size = 0;
    ae_http_parse_wav_header(&input_ctx, &audio_info.sample_rate, &audio_info.channels,
                             &audio_info.bits_per_sample, &data_offset, &data_size);
    ESP_LOGI(TAG, "Audio format: %d channels, %d Hz, %d bits",
             audio_info.channels, audio_info.sample_rate, audio_info.bits_per_sample);

    // Initialize output context for upload
    char output_filename[1024];
    snprintf(output_filename, sizeof(output_filename), "%s_speed_%.2f_pitch_%.2f_bits_%d.wav", filename, speed, pitch, target_bits);
    audio_info.bits_per_sample = target_bits;
    ret = ae_http_upload_init(&output_ctx, output_filename, &audio_info, HTTP_SERVER_URL_UPLOAD);
    TEST_ASSERT_EQUAL(ret, ESP_OK);

    esp_ae_sonic_cfg_t config = {0};
    config.sample_rate = audio_info.sample_rate;
    config.channel = audio_info.channels;
    config.bits_per_sample = audio_info.bits_per_sample;

    void *sonic_handle = NULL;
    ret = esp_ae_sonic_open(&config, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    esp_ae_sonic_set_speed(sonic_handle, speed);
    esp_ae_sonic_set_pitch(sonic_handle, pitch);

    int result = process_audio_data_http(&input_ctx, &output_ctx, sonic_handle, &audio_info, target_bits);
    // Cleanup
    esp_ae_sonic_close(sonic_handle);
    ae_http_deinit(&input_ctx);
    ae_http_deinit(&output_ctx);
    return result;
}

TEST_CASE("Sonic branch test", "AUDIO_EFFECT")
{
    esp_ae_sonic_cfg_t config;
    void *sonic_handle = NULL;
    config.sample_rate = 44100;
    config.channel = 2;
    config.bits_per_sample = 16;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_sonic_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_sonic_open(NULL, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_sonic_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.bits_per_sample = 8;
    ret = esp_ae_sonic_open(&config, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.bits_per_sample = 16;
    config.channel = 0;
    ret = esp_ae_sonic_open(&config, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create sonic handle");
    config.channel = 2;
    ret = esp_ae_sonic_open(&config, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_sonic_set_speed");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_sonic_set_speed(NULL, 1.0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_sonic_set_speed(sonic_handle, -1.0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_sonic_get_speed");
    ESP_LOGI(TAG, "test1");
    float speed;
    ret = esp_ae_sonic_get_speed(NULL, &speed);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_sonic_get_speed(sonic_handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_sonic_set_pitch");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_sonic_set_pitch(NULL, 1.0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_sonic_set_pitch(sonic_handle, -1.0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_sonic_get_speed");
    ESP_LOGI(TAG, "test1");
    float pitch;
    ret = esp_ae_sonic_get_pitch(NULL, &pitch);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_sonic_get_pitch(sonic_handle, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_sonic_in_data_t in_samples = {0};
    esp_ae_sonic_out_data_t out_samples = {0};
    char inbuf[100];
    char outbuf[100];
    in_samples.samples = inbuf;
    in_samples.num = 500;
    out_samples.samples = outbuf;
    out_samples.needed_num = 300;
    ESP_LOGI(TAG, "esp_ae_sonic_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_sonic_process(NULL, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_sonic_process(sonic_handle, NULL, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    in_samples.samples = NULL;
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    in_samples.samples = inbuf;
    in_samples.num = 0;
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "test6");
    in_samples.num = 500;
    out_samples.samples = NULL;
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    out_samples.samples = outbuf;
    out_samples.needed_num = 0;
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_sonic_close(sonic_handle);
}

TEST_CASE("Sonic HTTP download and upload test", "AUDIO_EFFECT")
{
    ESP_LOGI(TAG, "Starting HTTP download and upload test");
    // Initialize network
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    // Test different speed and pitch combinations
    float test_combinations[][2] = {{0.5f, 1.0f}, {1.0f, 0.5f}, {1.0f, 2.0f}, {2.0f, 1.0f}, {0.75f, 1.25f}, {1.25f, 0.75f}};
    for (int i = 0; i < AE_TEST_PARAM_NUM(file_name); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(test_combinations); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits_per_sample); k++) {
                float speed = test_combinations[j][0];
                float pitch = test_combinations[j][1];
                ESP_LOGI(TAG, "Testing combination %d: speed=%.2f, pitch=%.2f, file=%s, bits=%d",
                         j + 1, speed, pitch, file_name[i], bits_per_sample[k]);
                sonic_http_test(file_name[i], speed, pitch, bits_per_sample[k]);
                ESP_LOGI(TAG, "Completed test combination %d", j + 1);
            }
        }
    }
    example_disconnect();
    ESP_LOGI(TAG, "HTTP download and upload test completed successfully");
}

TEST_CASE("Sonic reset test", "AUDIO_EFFECT")
{
    esp_ae_err_t ret;
    esp_ae_sonic_cfg_t cfg = {
        .sample_rate = 44100,
        .channel = 2,
        .bits_per_sample = 16,
    };

    void *sonic_handle = NULL;
    ret = esp_ae_sonic_open(&cfg, &sonic_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(sonic_handle);

    ret = esp_ae_sonic_set_speed(sonic_handle, 1.2f);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_sonic_set_pitch(sonic_handle, 1.0f);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    uint32_t num_samples = 1000 * cfg.sample_rate / 1000;
    uint32_t out_samples_num = 1000 * cfg.sample_rate / 1000;
    uint8_t *input_buffer = (uint8_t *)calloc(num_samples * cfg.channel, cfg.bits_per_sample >> 3);
    uint8_t *output_buffer = (uint8_t *)calloc(out_samples_num * cfg.channel, cfg.bits_per_sample >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(out_samples_num * cfg.channel, cfg.bits_per_sample >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);

    ae_test_generate_sweep_signal(input_buffer, 1000, cfg.sample_rate, -6.0f, cfg.bits_per_sample, cfg.channel);

    esp_ae_sonic_in_data_t in_samples = {
        .samples = input_buffer,
        .num = num_samples};
    esp_ae_sonic_out_data_t out_samples = {
        .samples = output_buffer,
        .needed_num = out_samples_num};

    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    uint32_t out_num_1 = out_samples.needed_num;

    ret = esp_ae_sonic_reset(sonic_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    esp_ae_sonic_out_data_t out_samples_reset = {
        .samples = output_buffer_reset,
        .needed_num = out_samples_num};
    ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    uint32_t out_num_2 = out_samples_reset.needed_num;

    TEST_ASSERT_EQUAL(out_num_1, out_num_2);
    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, out_num_1 * cfg.channel * (cfg.bits_per_sample >> 3));

    esp_ae_sonic_close(sonic_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}
