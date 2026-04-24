/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
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

#define TAG "TEST_SONIC"

#define WIFI_FS_MOUNT_POINT "/sdcard"
#define SONIC_INPUT_ROOT    "/sdcard/mount/enc_effects_testset/voice"
#define SONIC_OUTPUT_ROOT   "/sdcard/mount/ae_test/sonic_test"

static char file_name[][100] = {
    "manch_48000_1_16_10",
    "manen_48000_1_16_10",
    "manloud_48000_1_16_10",
    "womanch_16000_1_16_6",
    "womanen_48000_1_16_10",
    "womanloud_48000_1_16_10",
};

static uint8_t bits_per_sample[] = {16, 24, 32};

#define WRITE_CACHE_SIZE (512 * 1024)

static int process_audio_data_fs(FILE *infile, FILE *outfile, void *sonic_handle,
                                 ae_audio_info_t *audio_info, int output_bits)
{
    int in_num = 4096;
    int out_num = 4096;
    short *inbuf = calloc(1, in_num * audio_info->channels * sizeof(short));
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    short *outbuf = calloc(1, out_num * audio_info->channels * (output_bits >> 3));
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    uint8_t *cache = malloc(WRITE_CACHE_SIZE);
    TEST_ASSERT_NOT_NULL(cache);
    uint32_t cache_pos = 0;
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
    uint32_t total_written = 0;
    while ((in_read = fread(inbuf, 1, in_num * audio_info->channels * sizeof(short), infile)) > 0) {
        sample_num = in_read / (audio_info->channels * sizeof(short));
        remain_num = sample_num;
        in_samples.samples = inbuf;
        out_samples.samples = outbuf;
        if (output_bits != 16) {
            esp_ae_bit_cvt_process(cvt_in_handle, sample_num, inbuf, inbuf_target);
            in_samples.samples = inbuf_target;
        }
        in_samples.num = sample_num;
        out_samples.needed_num = 4096;

        void *in = in_samples.samples;
        while (remain_num > 0 || out_samples.out_num > 0) {
            ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
            TEST_ASSERT_EQUAL(ret, 0);
            if (out_samples.out_num > 0) {
                size_t write_size = out_samples.out_num * (output_bits >> 3) * audio_info->channels;
                if (cache_pos + write_size >= WRITE_CACHE_SIZE) {
                    size_t written = fwrite(cache, 1, cache_pos, outfile);
                    TEST_ASSERT_EQUAL(cache_pos, written);
                    cache_pos = 0;
                }
                memcpy(cache + cache_pos, out_samples.samples, write_size);
                cache_pos += write_size;
                total_written += write_size;
            }
            int consume_bytes = in_samples.consume_num * audio_info->channels * (output_bits >> 3);
            in = (uint8_t *)in + consume_bytes;
            remain_num -= in_samples.consume_num;
            in_samples.num = remain_num;
            in_samples.samples = in;
        }
    }
    if (cache_pos > 0) {
        size_t written = fwrite(cache, 1, cache_pos, outfile);
        TEST_ASSERT_EQUAL(cache_pos, written);
    }
    TEST_ASSERT_TRUE(ae_test_write_wav_header(outfile, audio_info, total_written));
    if (cvt_in_handle) {
        esp_ae_bit_cvt_close(cvt_in_handle);
    }
    free(inbuf);
    free(outbuf);
    free(cache);
    if (inbuf_target) {
        free(inbuf_target);
    }
    return 1;
}

static int sonic_http_test(const char *filename, float speed, float pitch, int target_bits)
{
    ESP_LOGI(TAG, "Starting wifi_fs test for file: %s, speed: %.2f, pitch: %.2f, bits: %d",
             filename, speed, pitch, target_bits);
    FILE *input_fp = NULL;
    FILE *output_fp = NULL;
    char input_path[256];
    snprintf(input_path, sizeof(input_path), "%s/%s.wav", SONIC_INPUT_ROOT, filename);
    ESP_LOGI(TAG, "Input path: %s", input_path);
    input_fp = fopen(input_path, "rb");
    TEST_ASSERT_NOT_NULL(input_fp);

    ae_audio_info_t audio_info = {0};
    uint32_t data_size = 0;
    TEST_ASSERT_TRUE(ae_test_parse_wav_header(input_fp, &audio_info, &data_size));
    ESP_LOGI(TAG, "Audio format: %d channels, %d Hz, %d bits",
             audio_info.channels, audio_info.sample_rate, audio_info.bits_per_sample);
    ESP_LOGI(TAG, "Input data size: %" PRIu32 " bytes", data_size);

    char output_filename[1024];
    snprintf(output_filename, sizeof(output_filename), "%s_speed_%.2f_pitch_%.2f_bits_%d.wav", filename, speed, pitch, target_bits);
    audio_info.bits_per_sample = target_bits;
    char output_path[1200];
    snprintf(output_path, sizeof(output_path), "%s/%s", SONIC_OUTPUT_ROOT, output_filename);
    output_fp = fopen(output_path, "wb+");
    TEST_ASSERT_NOT_NULL(output_fp);
    TEST_ASSERT_TRUE(ae_test_write_wav_header(output_fp, &audio_info, 0));

    esp_ae_sonic_cfg_t config = {0};
    config.sample_rate = audio_info.sample_rate;
    config.channel = audio_info.channels;
    config.bits_per_sample = audio_info.bits_per_sample;

    void *sonic_handle = NULL;
    esp_ae_err_t ret = esp_ae_sonic_open(&config, &sonic_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    esp_ae_sonic_set_speed(sonic_handle, speed);
    esp_ae_sonic_set_pitch(sonic_handle, pitch);

    int result = process_audio_data_fs(input_fp, output_fp, sonic_handle, &audio_info, target_bits);
    // Cleanup
    esp_ae_sonic_close(sonic_handle);
    fclose(input_fp);
    fclose(output_fp);
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
    ESP_LOGI(TAG, "Starting wifi_fs download and upload test");
    ESP_ERROR_CHECK(ae_test_ensure_wifi_fs_ready(WIFI_FS_MOUNT_POINT));
    // Test different speed and pitch combinations
    float test_combinations[][2] = {{0.5f, 1.0f}, {1.0f, 2.0f}, {1.25f, 0.75f}};
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
    ESP_LOGI(TAG, "wifi_fs download and upload test completed successfully");
    ae_test_wifi_fs_cleanup(WIFI_FS_MOUNT_POINT);
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
