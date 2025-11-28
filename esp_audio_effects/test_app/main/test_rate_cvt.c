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
#include "ae_common.h"
#include "esp_ae_rate_cvt.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_ch_cvt.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#define TAG                      "TEST_RATE_CVT"
#define HTTP_SERVER_URL_DOWNLOAD "http://10.18.20.184:8080/audio_files/audio_test_dataset/sine"
#define HTTP_SERVER_URL_UPLOAD   "http://10.18.20.184:8080/upload?folder=ae_test/rate_cvt_test"

static uint32_t sample_rate_in[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
static uint32_t sample_rate[]    = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
static uint8_t  bits[]           = {16, 24, 32};

static bool process_audio_data_http(ae_http_context_t *infile, ae_http_context_t *outfile,
                                    ae_audio_info_t *audio_info, uint32_t dest_rate, uint32_t data_size)
{
    esp_ae_rate_cvt_cfg_t config = {
        .src_rate = audio_info->sample_rate,
        .dest_rate = dest_rate,
        .channel = audio_info->channels,
        .bits_per_sample = audio_info->bits_per_sample,
        .complexity = 3,
        .perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED};

    void *rate_cvt_handle = NULL;
    esp_ae_err_t ret = esp_ae_rate_cvt_open(&config, &rate_cvt_handle);

    uint32_t in_num = 256;
    uint32_t out_num = 0;
    ret = esp_ae_rate_cvt_get_max_out_sample_num(rate_cvt_handle, in_num, &out_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    size_t total_samples = 0;
    uint8_t *inbuf = calloc(1, in_num * audio_info->channels * (audio_info->bits_per_sample >> 3));
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *outbuf = calloc(1, out_num * audio_info->channels * (audio_info->bits_per_sample >> 3));
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    int in_read = 0;
    int sample_num = 0;
    uint32_t total_read = 0;
    bool do_retry = false;
    while (1) {
        in_read = ae_http_read(inbuf, 1, in_num * audio_info->channels * (audio_info->bits_per_sample >> 3), infile);
        if (in_read <= 0) {
            break;
        }
        total_read += in_read;
        if (total_read > data_size) {
            break;
        }
        sample_num = in_read / (audio_info->channels * (audio_info->bits_per_sample >> 3));
        uint32_t out_samples = out_num;
        ret = esp_ae_rate_cvt_process(rate_cvt_handle, inbuf, sample_num, outbuf, &out_samples);
        TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
        if (out_samples > 0) {
            int written = ae_http_write(outbuf, 1, out_samples * (audio_info->bits_per_sample >> 3) * audio_info->channels, false, outfile);
            if (written < 0) {
                ESP_LOGE(TAG, "Failed to write data to HTTP stream do retry");
                do_retry = true;
                break;
            }
            total_samples += out_samples;
        }
    }
    ae_http_write(NULL, 0, 0, true, outfile);
    free(inbuf);
    free(outbuf);
    esp_ae_rate_cvt_close(rate_cvt_handle);
    return do_retry;
}

static void rate_cvt_http_test(uint32_t src_rate, uint32_t dest_rate, uint8_t bits)
{
_RATE_CVT_HTTP_TEST_RETRY:
    ae_http_context_t input_ctx = {0};
    ae_http_context_t output_ctx = {0};
    char download_filename[128];
    sprintf(download_filename, "sine1kHz0dB_%ld_1_%d_10", src_rate, bits);
    int ret = ae_http_download_init(&input_ctx, download_filename, HTTP_SERVER_URL_DOWNLOAD);
    TEST_ASSERT_EQUAL(ret, ESP_OK);

    long data_offset = 0;
    ae_audio_info_t audio_info = {0};
    ae_audio_info_t dest_audio_info = {0};
    uint32_t data_size = 0;
    ae_http_parse_wav_header(&input_ctx, &audio_info.sample_rate, &audio_info.channels,
                             &audio_info.bits_per_sample, &data_offset, &data_size);
    ESP_LOGI(TAG, "Starting HTTP test for rate conversion: %ld -> %ld, bits: %d, channels: %d",
             src_rate, dest_rate, audio_info.bits_per_sample, audio_info.channels);
    memcpy(&dest_audio_info, &audio_info, sizeof(ae_audio_info_t));
    dest_audio_info.sample_rate = dest_rate;
    // Initialize output context for upload
    char output_filename[128];
    snprintf(output_filename, sizeof(output_filename), "sine1kHz0dB_%ld_to_%ld_1_%d_10.wav",
             src_rate, dest_rate, audio_info.bits_per_sample);
    char upload_url[128];
    snprintf(upload_url, sizeof(upload_url), "%s/%d", HTTP_SERVER_URL_UPLOAD, audio_info.bits_per_sample);
    ret = ae_http_upload_init(&output_ctx, output_filename, &dest_audio_info, upload_url);
    TEST_ASSERT_EQUAL(ret, ESP_OK);

    bool do_retry = process_audio_data_http(&input_ctx, &output_ctx, &audio_info, dest_rate, data_size);
    ae_http_deinit(&input_ctx);
    ae_http_deinit(&output_ctx);
    if (do_retry) {
        ESP_LOGI(TAG, "Retry rate conversion");
        goto _RATE_CVT_HTTP_TEST_RETRY;
    }
}

static void test_rate_cvt_consistency(uint32_t src_rate, uint32_t dest_rate, uint8_t bits_per_sample)
{
    ESP_LOGI(TAG, "Testing consistency: %ld->%ld, %d bits", src_rate, dest_rate, bits_per_sample);
    esp_ae_rate_cvt_cfg_t config = {
        .src_rate = src_rate,
        .dest_rate = dest_rate,
        .channel = 2,
        .bits_per_sample = bits_per_sample,
        .complexity = 3,
        .perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED};
    void *hd1 = NULL;
    void *hd2 = NULL;
    esp_err_t ret = esp_ae_rate_cvt_open(&config, &hd1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ret = esp_ae_rate_cvt_open(&config, &hd2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    const int duration_ms = 100;
    int input_bytes_per_sample = (bits_per_sample >> 3) * config.channel;
    int output_bytes_per_sample = (bits_per_sample >> 3) * config.channel;

    uint32_t sample_count = (src_rate * duration_ms) / 1000;
    uint32_t out_sample_count = 0;
    ret = esp_ae_rate_cvt_get_max_out_sample_num(hd1, sample_count, &out_sample_count);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    void *interlv_in = calloc(sample_count, input_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_in, NULL);
    void *interlv_out = calloc(out_sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(interlv_out, NULL);
    void *deinterlv_in[2] = {calloc(sample_count, bits_per_sample >> 3), calloc(sample_count, bits_per_sample >> 3)};
    TEST_ASSERT_NOT_EQUAL(deinterlv_in[0], NULL);
    TEST_ASSERT_NOT_EQUAL(deinterlv_in[1], NULL);
    void *deinterlv_out[2] = {calloc(out_sample_count, bits_per_sample >> 3), calloc(out_sample_count, bits_per_sample >> 3)};
    TEST_ASSERT_NOT_EQUAL(deinterlv_out[0], NULL);
    TEST_ASSERT_NOT_EQUAL(deinterlv_out[1], NULL);
    void *deinterlv_out_cmp = calloc(out_sample_count, output_bytes_per_sample);
    TEST_ASSERT_NOT_EQUAL(deinterlv_out_cmp, NULL);

    ae_test_generate_sweep_signal(interlv_in, duration_ms, src_rate,
                                  0.0f, bits_per_sample, config.channel);

    ret = esp_ae_deintlv_process(config.channel, bits_per_sample, sample_count,
                                 (esp_ae_sample_t)interlv_in, (esp_ae_sample_t *)deinterlv_in);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    uint32_t interleaved_out_samples = out_sample_count;
    ret = esp_ae_rate_cvt_process(hd1, (esp_ae_sample_t)interlv_in,
                                  sample_count, (esp_ae_sample_t)interlv_out, &interleaved_out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    uint32_t deinterleaved_out_samples = out_sample_count;
    ret = esp_ae_rate_cvt_deintlv_process(hd2, (esp_ae_sample_t *)deinterlv_in,
                                          sample_count, (esp_ae_sample_t *)deinterlv_out,
                                          &deinterleaved_out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL(interleaved_out_samples, deinterleaved_out_samples);

    ret = esp_ae_intlv_process(config.channel, bits_per_sample, deinterleaved_out_samples,
                               (esp_ae_sample_t *)deinterlv_out, (esp_ae_sample_t)deinterlv_out_cmp);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(deinterlv_out_cmp, interlv_out, deinterleaved_out_samples * output_bytes_per_sample, 1);

    esp_ae_rate_cvt_close(hd1);
    esp_ae_rate_cvt_close(hd2);
    free(interlv_in);
    free(interlv_out);
    for (int i = 0; i < config.channel; i++) {
        free(deinterlv_in[i]);
        free(deinterlv_out[i]);
    }
    free(deinterlv_out_cmp);
}

TEST_CASE("Rate Convert branch test", "AUDIO_EFFECT")
{
    esp_ae_rate_cvt_cfg_t config;
    void *rsp_handle = NULL;
    config.src_rate = 12000;
    config.dest_rate = 24000;
    config.channel = 2;
    config.bits_per_sample = 16;
    config.complexity = 2;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_fade_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_rate_cvt_open(NULL, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.complexity = 6;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.complexity = 2;
    config.src_rate = 12500;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    config.src_rate = 12000;
    config.dest_rate = 24500;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    config.dest_rate = 24000;
    config.channel = 0;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    config.channel = 2;
    config.bits_per_sample = 8;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create resample handle");
    config.bits_per_sample = 16;
    ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_rate_cvt_get_max_out_sample_num");
    uint32_t out_num;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_rate_cvt_get_max_out_sample_num(NULL, 1024, &out_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, 0, &out_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, 1024, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, 1024, &out_num);
    ESP_LOGI(TAG, "out sample num:%d", (int)out_num);
    char in_samples[100];
    char out_samples[100];
    char in_samples1[2][100] = {0};
    char out_samples1[2][100] = {0};
    uint32_t sample_num = 0;
    ESP_LOGI(TAG, "esp_ae_rate_cvt_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_rate_cvt_process(NULL, (esp_ae_sample_t)in_samples, 1024, (esp_ae_sample_t)out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_process(rsp_handle, NULL, 1024, (esp_ae_sample_t)out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_rate_cvt_process(rsp_handle, (esp_ae_sample_t)in_samples, 0, (esp_ae_sample_t)out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_rate_cvt_process(rsp_handle, (esp_ae_sample_t)in_samples, 1024, NULL, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_rate_cvt_process(rsp_handle, (esp_ae_sample_t)in_samples, 1024, (esp_ae_sample_t)out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    sample_num = 50;
    ret = esp_ae_rate_cvt_process(rsp_handle, (esp_ae_sample_t)in_samples, 1024, (esp_ae_sample_t)out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_rate_cvt_deintlv_process");
    sample_num = 0;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_rate_cvt_deintlv_process(NULL, (esp_ae_sample_t *)in_samples1, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, NULL, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1, 0,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1,
                                          1024, (esp_ae_sample_t *)NULL, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    sample_num = 50;
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    sample_num = 2048;
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, (esp_ae_sample_t *)in_samples1, 1024,
                                          (esp_ae_sample_t *)out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_rate_cvt_close(rsp_handle);
}

TEST_CASE("Rate Convert reset test", "AUDIO_EFFECT")
{
    uint32_t srate_in = 44100;
    uint32_t srate_out = 48000;
    uint8_t ch_num = 2;
    uint8_t bits_per_sample = 16;
    uint32_t duration_ms = 1000;
    uint32_t num_samples = duration_ms * srate_in / 1000;
    esp_ae_err_t ret = ESP_AE_ERR_OK;

    esp_ae_rate_cvt_cfg_t config = {
        .src_rate = srate_in,
        .dest_rate = srate_out,
        .channel = ch_num,
        .bits_per_sample = bits_per_sample,
        .complexity = 2,
        .perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED};

    void *rate_cvt_handle = NULL;
    ret = esp_ae_rate_cvt_open(&config, &rate_cvt_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    TEST_ASSERT_NOT_NULL(rate_cvt_handle);
    uint32_t out_size = 0;
    uint8_t *input_buffer = (uint8_t *)calloc(num_samples * ch_num, bits_per_sample >> 3);
    esp_ae_rate_cvt_get_max_out_sample_num(rate_cvt_handle, num_samples, &out_size);
    uint8_t *output_buffer = (uint8_t *)calloc(out_size * ch_num, bits_per_sample >> 3);
    uint8_t *output_buffer_reset = (uint8_t *)calloc(out_size * ch_num, bits_per_sample >> 3);
    TEST_ASSERT_NOT_NULL(input_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer);
    TEST_ASSERT_NOT_NULL(output_buffer_reset);

    ae_test_generate_sweep_signal(input_buffer, duration_ms, srate_in, -6.0f, bits_per_sample, ch_num);
    uint32_t out_samples = out_size;
    uint32_t half_samples = num_samples / 2;
    ret = esp_ae_rate_cvt_process(rate_cvt_handle, (esp_ae_sample_t)input_buffer + half_samples * ch_num * (bits_per_sample >> 3),
                                  half_samples, (esp_ae_sample_t)output_buffer, &out_samples);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    ret = esp_ae_rate_cvt_reset(rate_cvt_handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    uint32_t out_samples_reset = out_size;
    ret = esp_ae_rate_cvt_process(rate_cvt_handle, (esp_ae_sample_t)input_buffer + half_samples * ch_num * (bits_per_sample >> 3),
                                  half_samples, (esp_ae_sample_t)output_buffer_reset, &out_samples_reset);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    TEST_ASSERT_EQUAL(out_samples, out_samples_reset);
    TEST_ASSERT_EQUAL_MEMORY(output_buffer, output_buffer_reset, out_samples * ch_num * (bits_per_sample >> 3));

    esp_ae_rate_cvt_close(rate_cvt_handle);
    free(input_buffer);
    free(output_buffer);
    free(output_buffer_reset);
}

TEST_CASE("Rate Convert HTTP download and upload test", "AUDIO_EFFECT")
{
    ESP_LOGI(TAG, "Starting HTTP download and upload test");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    // Test different destination sample rates
    uint32_t rates[] = {8000, 16000, 32000, 44100, 48000, 96000};

    for (int i = 0; i < AE_TEST_PARAM_NUM(rates); i++) {
        for (int j = 0; j < AE_TEST_PARAM_NUM(rates); j++) {
            for (int k = 0; k < AE_TEST_PARAM_NUM(bits); k++) {
                ESP_LOGI(TAG, "Testing rate conversion: %ld -> %ld, bits: %d", rates[i], rates[j], bits[k]);
                rate_cvt_http_test(rates[i], rates[j], bits[k]);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
        }
    }
    example_disconnect();
    ESP_LOGI(TAG, "HTTP download and upload test completed successfully");
}

TEST_CASE("Rate Convert interleave vs deinterleave consistency test", "AUDIO_EFFECT")
{
    for (int src_rate_idx = 0; src_rate_idx < AE_TEST_PARAM_NUM(sample_rate_in); src_rate_idx++) {
        for (int dest_rate_idx = 0; dest_rate_idx < AE_TEST_PARAM_NUM(sample_rate); dest_rate_idx++) {
            for (int bit_idx = 0; bit_idx < AE_TEST_PARAM_NUM(bits); bit_idx++) {
                test_rate_cvt_consistency(sample_rate_in[src_rate_idx], sample_rate[dest_rate_idx], bits[bit_idx]);
            }
        }
    }
}
