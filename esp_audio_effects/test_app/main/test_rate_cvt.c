/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
#include "esp_ae_rate_cvt.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"

#define TAG "TEST_RSP"
#define CMP_MODE

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
    ret = esp_ae_rate_cvt_process(NULL, in_samples, 1024, out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_process(rsp_handle, NULL, 1024, out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_rate_cvt_process(rsp_handle, in_samples, 0, out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_rate_cvt_process(rsp_handle, in_samples, 1024, NULL, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_rate_cvt_process(rsp_handle, in_samples, 1024, out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    sample_num = 50;
    ret = esp_ae_rate_cvt_process(rsp_handle, in_samples, 1024, out_samples, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_rate_cvt_deintlv_process");
    sample_num = 0;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_rate_cvt_deintlv_process(NULL, in_samples1, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, NULL, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1, 0,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1,
                                            1024, NULL, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    sample_num = 50;
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    sample_num = 2048;
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_samples1, 1024,
                                            out_samples1, &sample_num);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_rate_cvt_close(rsp_handle);
}

TEST_CASE("Rate Convert interleave 16bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = NULL;
                infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/16/test_rsp_rate1_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
#ifdef CMP_MODE
                FILE *outfile = fopen(out_name, "rb");
#else
                FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = 16;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config;
                config.bits_per_sample = bit;
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);
                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 1024 / (16 >> 3) / channel;
                char *inbuf = calloc(1, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *outbuf = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                int in_offset = 0;
                int in_read = 0;
                uint32_t out_samples = 0;
                while (1) {
                    memset(inbuf, 0, 1024);
                    in_read = fread(inbuf, 1, 1024, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 1024 / (16 >> 3) / channel;
                    out_samples = out_samples_num;
                    ret = esp_ae_rate_cvt_process(rsp_handle, inbuf, sample_num, outbuf, &out_samples);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, out_samples * channel * 2, outfile);
                    TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_samples * channel * 2), 0);
#else
                    fwrite(outbuf, 2, out_samples * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_rate_cvt_close(rsp_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(outbuf);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("Rate Convert uninterleave 16bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/16_inter/test_rsp_rate2_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
#ifdef CMP_MODE
                FILE *outfile = fopen(out_name, "rb");
#else
                FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);
                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = 16;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config = {0};
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.bits_per_sample = bit;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);

                int in_offset = 0;
                int in_read = 0;
                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 1024 / (16 >> 3) / channel;
                char *inbuf = calloc(1, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *in_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    in_buf[i] = calloc(1, 1024);
                    TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
                }
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *outbuf = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *out_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    out_buf[i] = calloc(1, out_samples_num * bit >> 3);
                    TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
                }
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

                uint32_t out_sample;
                while (1) {
                    memset(inbuf, 0, 1024);
                    in_read = fread(inbuf, 1, 1024, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 1024 / (16 >> 3) / channel;
                    esp_ae_deintlv_process(channel, 16, sample_num, inbuf, in_buf);
                    out_sample = out_samples_num;
                    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_buf, sample_num, out_buf, &out_sample);
                    esp_ae_intlv_process(channel, 16, out_sample, out_buf, outbuf);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, out_sample * channel * 2, outfile);
                    TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_sample * channel * 2), 0);
#else
                    fwrite(outbuf, 2, out_sample * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_rate_cvt_close(rsp_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(outbuf);
                free(cmp_buffer);
                for (int i = 0; i < channel; i++) {
                    free(in_buf[i]);
                    free(out_buf[i]);
                }
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("Rate Convert interleave 24bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = NULL;
                infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/24/test_rsp_rate3_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
                FILE *outfile = NULL;
#ifdef CMP_MODE
                outfile = fopen(out_name, "rb");
#else
                outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = ESP_AE_BIT24;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config;
                config.bits_per_sample = bit;
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = src_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = dest_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 2048 / (16 >> 3) / channel;
                char *inbuf = calloc(1, 2048);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *in_buf = calloc(1, 2048 * 2);
                TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *out_buf = calloc(1, out_samples_num * (ESP_AE_BIT24 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
                char *outbuf = calloc(1, out_samples_num * (16 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

                int in_offset = 0;
                int in_read = 0;
                uint32_t out_samples = 0;
                while (1) {
                    memset(inbuf, 0, 2048);
                    in_read = fread(inbuf, 1, 2048, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 2048 / (16 >> 3) / channel;
                    out_samples = out_samples_num;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
                    ret = esp_ae_rate_cvt_process(rsp_handle, in_buf, sample_num, out_buf, &out_samples);
                    if (out_samples != 0) {
                        esp_ae_bit_cvt_process(b2_handle, out_samples, out_buf, outbuf);
#ifdef CMP_MODE
                        fread(cmp_buffer, 1, out_samples * channel * (16 >> 3), outfile);
                        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_samples * channel * (16 >> 3)), 0);
#else
                        fwrite(outbuf, 1, out_samples * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
                    }
                }
                esp_ae_rate_cvt_close(rsp_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(in_buf);
                free(outbuf);
                free(out_buf);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("Rate Convert uninterleave 24bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = NULL;
                infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/24_inter/test_rsp_rate4_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
                FILE *outfile = NULL;
#ifdef CMP_MODE
                outfile = fopen(out_name, "rb");
#else
                outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = ESP_AE_BIT24;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config = {0};
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.bits_per_sample = bit;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);

                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 2048 / (16 >> 3) / channel;
                char *inbuf = calloc(2, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *inbuf1 = calloc(4, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf1, NULL);
                char *in_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    in_buf[i] = calloc(4, 1024 / channel);
                    TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
                }
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *outbuf = calloc(1, out_samples_num * (16 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                char *outbuf1 = calloc(1, out_samples_num * (ESP_AE_BIT24 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf1, NULL);
                char *out_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    out_buf[i] = calloc(1, out_samples_num * (ESP_AE_BIT24 >> 3));
                    TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
                }
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = src_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = dest_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
                int in_offset = 0;
                int in_read = 0;
                uint32_t out_sample;
                while (1) {
                    memset(inbuf, 0, 2048);
                    in_read = fread(inbuf, 1, 2048, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 2048 / (16 >> 3) / channel;
                    out_sample = out_samples_num;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, inbuf1);
                    esp_ae_deintlv_process(channel, ESP_AE_BIT24, sample_num, inbuf1, in_buf);
                    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_buf, sample_num, out_buf, &out_sample);
                    if (out_sample != 0) {
                        esp_ae_intlv_process(channel, ESP_AE_BIT24, out_sample, out_buf, outbuf1);
                        esp_ae_bit_cvt_process(b2_handle, out_sample, outbuf1, outbuf);
#ifdef CMP_MODE
                        fread(cmp_buffer, 1, out_sample * channel * (16 >> 3), outfile);
                        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_sample * channel * (16 >> 3)), 0);
#else
                        fwrite(outbuf, 1, out_sample * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
                    }
                }
                esp_ae_rate_cvt_close(rsp_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(outbuf);
                free(inbuf1);
                free(outbuf1);
                free(cmp_buffer);
                for (int i = 0; i < channel; i++) {
                    free(in_buf[i]);
                    free(out_buf[i]);
                }
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("Rate Convert interleave 32bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = NULL;
                infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/32/test_rsp_rate5_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
                FILE *outfile = NULL;
#ifdef CMP_MODE
                outfile = fopen(out_name, "rb");
#else
                outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = 32;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config;
                config.bits_per_sample = bit;
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = src_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = dest_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 2048 / (16 >> 3) / channel;
                char *inbuf = calloc(1, 2048);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *in_buf = calloc(1, 2048 * 2);
                TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *out_buf = calloc(1, out_samples_num * (32 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
                char *outbuf = calloc(1, out_samples_num * (16 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                int in_offset = 0;
                int in_read = 0;
                uint32_t out_samples = 0;
                while (1) {
                    memset(inbuf, 0, 2048);
                    in_read = fread(inbuf, 1, 2048, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 2048 / (16 >> 3) / channel;
                    out_samples = out_samples_num;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
                    ret = esp_ae_rate_cvt_process(rsp_handle, in_buf, sample_num, out_buf, &out_samples);
                    if (out_samples != 0) {
                        esp_ae_bit_cvt_process(b2_handle, out_samples, out_buf, outbuf);
#ifdef CMP_MODE
                        fread(cmp_buffer, 1, out_samples * channel * (16 >> 3), outfile);
                        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_samples * channel * (16 >> 3)), 0);
#else
                        fwrite(outbuf, 1, out_samples * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
                    }
                }
                esp_ae_rate_cvt_close(rsp_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(in_buf);
                free(outbuf);
                free(out_buf);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("Rate Convert uninterleave 32bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int sr[20] = {8, 11, 12, 16, 22, 24, 32, 44, 48, 64, 88, 96};
    int sample_rate_in[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    int sample_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            for (int k = 1; k <= 3; k++) {
                sprintf(in_name, "/sdcard/pcm/test_%d_2.pcm", sample_rate_in[i]);
                FILE *infile = NULL;
                infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/rsp_test/32_inter/test_rsp_rate6_%d_%d_%d.pcm", sr[i], sample_rate[j], k);
                FILE *outfile = NULL;
#ifdef CMP_MODE
                outfile = fopen(out_name, "rb");
#else
                outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int src_rate = sample_rate_in[i];
                int dest_rate = sample_rate[j];
                int channel = 2;
                int bit = 32;
                int complexity = k;
                esp_ae_rate_cvt_cfg_t config = {0};
                config.src_rate = src_rate;
                config.dest_rate = dest_rate;
                config.channel = channel;
                config.bits_per_sample = bit;
                config.complexity = complexity;
                config.perf_type = ESP_AE_RATE_CVT_PERF_TYPE_SPEED;
                void *rsp_handle = NULL;
                int ret = esp_ae_rate_cvt_open(&config, &rsp_handle);
                TEST_ASSERT_NOT_EQUAL(rsp_handle, NULL);
                uint32_t sample_num = 0;
                uint32_t out_samples_num = 0;
                sample_num = 2048 / (16 >> 3) / channel;
                char *inbuf = calloc(2, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *inbuf1 = calloc(4, 1024);
                TEST_ASSERT_NOT_EQUAL(inbuf1, NULL);
                char *in_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    in_buf[i] = calloc(4, 1024 / channel);
                    TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
                }
                esp_ae_rate_cvt_get_max_out_sample_num(rsp_handle, sample_num, &out_samples_num);
                char *outbuf = calloc(1, out_samples_num * (16 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                char *cmp_buffer = calloc(1, out_samples_num * channel * bit >> 3);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                char *outbuf1 = calloc(1, out_samples_num * (32 >> 3) * channel);
                TEST_ASSERT_NOT_EQUAL(outbuf1, NULL);
                char *out_buf[10] = {0};
                for (int i = 0; i < channel; i++) {
                    out_buf[i] = calloc(1, out_samples_num * (32 >> 3));
                    TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
                }
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = src_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = dest_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
                int in_offset = 0;
                int in_read = 0;
                uint32_t out_sample;
                while (1) {
                    memset(inbuf, 0, 2048);
                    in_read = fread(inbuf, 1, 2048, infile);
                    if (in_read <= 0) {
                        break;
                    }
                    sample_num = 2048 / (16 >> 3) / channel;
                    out_sample = out_samples_num;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, inbuf1);
                    esp_ae_deintlv_process(channel, 32, sample_num, inbuf1, in_buf);
                    ret = esp_ae_rate_cvt_deintlv_process(rsp_handle, in_buf, sample_num, out_buf, &out_sample);
                    if (out_sample != 0) {
                        esp_ae_intlv_process(channel, 32, out_sample, out_buf, outbuf1);
                        esp_ae_bit_cvt_process(b2_handle, out_sample, outbuf1, outbuf);
#ifdef CMP_MODE
                        fread(cmp_buffer, 1, out_sample * channel * (16 >> 3), outfile);
                        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, out_sample * channel * (16 >> 3)), 0);
#else
                        fwrite(outbuf, 1, out_sample * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
                    }
                }
                esp_ae_rate_cvt_close(rsp_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(outbuf);
                free(inbuf1);
                free(outbuf1);
                free(cmp_buffer);
                for (int i = 0; i < channel; i++) {
                    free(in_buf[i]);
                    free(out_buf[i]);
                }
            }
        }
    }
    ae_sdcard_deinit();
}