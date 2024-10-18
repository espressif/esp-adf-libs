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
#include "test_utils.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "test_common.h"
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_sonic.h"

#define TAG "TEST_SONIC"
#define CMP_MODE

static int sonic_interleave_16_bit_test(int mode, char *mode_name)
{
    // stream open
    float para[6] = {0.5, 0.75, 1.25, 1.5, 1.75, 2.0};
    int chan[5] = {1, 2};
    int sp[5] = {8, 44};
    int samplerate[5] = {8000, 44100};
    char out_name[100] = {0};
    char in_name[100] = {0};
    for (int j = 0; j < 1; j++) {
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 6; i++) {
                sprintf(in_name, "/sdcard/pcm/thetest%d_%d.pcm", sp[j], k + 1);
                FILE *infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/sonic_test/asm/test16_%s%d_%d_%d.pcm", mode_name, sp[j], chan[k], i);
#ifdef CMP_MODE
                FILE *outfile = fopen(out_name, "rb");
#else
                FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int sample_rate = samplerate[j];
                int channel = chan[k];
                esp_ae_sonic_cfg_t config = {0};
                config.sample_rate = sample_rate;
                config.channel = channel;
                config.bits_per_sample = 16;
                void *sonic_handle = NULL;
                int in_num = 512;
                int out_num = 512;
                short *inbuf = calloc(1, in_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                short *outbuf = calloc(1, out_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                short *outbuf_cmp = calloc(1, out_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(outbuf_cmp, NULL);
                short *in;
                esp_ae_sonic_open(&config, &sonic_handle);
                TEST_ASSERT_NOT_EQUAL(sonic_handle, NULL);
                if (mode == 1) {
                    esp_ae_sonic_set_speed(sonic_handle, para[i]);
                } else if (mode == 2) {
                    esp_ae_sonic_set_pitch(sonic_handle, para[i]);
                }
                esp_ae_sonic_in_data_t in_samples = {0};
                esp_ae_sonic_out_data_t out_samples = {0};
                in_samples.samples = inbuf;
                out_samples.samples = outbuf;
                int in_read = 0;
                int sample_num = 0;
                int remain_num = 0;
                int ret = 0;
                while ((in_read = fread(inbuf, 1, in_num * channel * sizeof(short), infile)) > 0) {
                    sample_num = in_read / (channel * sizeof(short));
                    remain_num = sample_num;
                    in_samples.samples = inbuf;
                    in_samples.num = sample_num;
                    out_samples.needed_num = 512;
                    while (remain_num > 0 || out_samples.out_num > 0) {
                        ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
                        TEST_ASSERT_EQUAL(ret, 0);
                        if (out_samples.out_num > 0) {
#ifdef CMP_MODE
                            fread(outbuf_cmp, 1,
                                  out_samples.out_num * sizeof(short) * channel, outfile);
                            TEST_ASSERT_EQUAL(memcmp(out_samples.samples, outbuf_cmp,
                                                     out_samples.out_num * sizeof(short) * channel),
                                              0);
#else
                            fwrite(out_samples.samples, 1,
                                   out_samples.out_num * sizeof(short) * channel, outfile);
#endif /* CMP_MODE */
                        }
                        in = inbuf + in_samples.consume_num * channel;
                        remain_num -= in_samples.consume_num;
                        in_samples.num = remain_num;
                        in_samples.samples = in;
                    }
                }
                esp_ae_sonic_close(sonic_handle);
                fclose(infile);
                infile = NULL;
                fclose(outfile);
                outfile = NULL;
                free(inbuf);
                inbuf = NULL;
                free(outbuf);
                outbuf = NULL;
                free(outbuf_cmp);
                outbuf_cmp = NULL;
            }
        }
    }
    return 1;
}

static int sonic_interleave_24_bit_test(int mode, char *mode_name)
{
    // stream open
    float para[6] = {0.5, 0.75, 1.25, 1.5, 1.75, 2.0};
    int chan[5] = {1, 2};
    int sp[5] = {8, 44};
    int samplerate[5] = {8000, 44100};
    char out_name[100] = {0};
    char in_name[100] = {0};
    for (int j = 0; j < 1; j++) {
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 6; i++) {
                sprintf(in_name, "/sdcard/pcm/thetest%d_%d.pcm", sp[j], k + 1);
                FILE *infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/sonic_test/asm/test24_%s%d_%d_%d.pcm", mode_name, sp[j], chan[k], i);
#ifdef CMP_MODE
                FILE *outfile = fopen(out_name, "rb");
#else
                FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);

                int sample_rate = samplerate[j];
                int channel = chan[k];
                esp_ae_sonic_cfg_t config = {0};
                config.sample_rate = sample_rate;
                config.channel = channel;
                config.bits_per_sample = ESP_AE_BIT24;
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                int in_num = 512;
                int out_num = 512;
                char *inbuf = calloc(sizeof(short), in_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *outbuf = calloc(sizeof(short), out_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                int *in_buf = calloc(sizeof(short), in_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
                int *out_buf = calloc(sizeof(short), out_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
                int *outbuf_cmp = calloc(sizeof(short), out_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(outbuf_cmp, NULL);
                void *sonic_handle = NULL;
                esp_ae_sonic_open(&config, &sonic_handle);
                TEST_ASSERT_NOT_EQUAL(sonic_handle, NULL);
                if (mode == 1) {
                    esp_ae_sonic_set_speed(sonic_handle, para[i]);
                } else if (mode == 2) {
                    esp_ae_sonic_set_pitch(sonic_handle, para[i]);
                }
                esp_ae_sonic_in_data_t in_samples = {0};
                esp_ae_sonic_out_data_t out_samples = {0};
                int *in;
                in_samples.samples = in_buf;
                out_samples.samples = out_buf;
                int in_read = 0;
                int sample_num = 0;
                int remain_num = 0;
                int ret = 0;
                while ((in_read = fread(inbuf, 1, in_num * channel * sizeof(short), infile)) > 0) {
                    sample_num = in_read / (channel * sizeof(short));
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
                    remain_num = sample_num;
                    in_samples.samples = in_buf;
                    in_samples.num = sample_num;
                    out_samples.needed_num = 512;
                    while (remain_num > 0 || out_samples.out_num > 0) {
                        ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
                        TEST_ASSERT_EQUAL(ret, 0);
                        if (out_samples.out_num > 0) {
                            esp_ae_bit_cvt_process(b2_handle, out_samples.out_num,
                                                    out_samples.samples, outbuf);
#ifdef CMP_MODE
                            fread(outbuf_cmp, 1,
                                  out_samples.out_num * sizeof(short) * channel, outfile);
                            TEST_ASSERT_EQUAL(memcmp(outbuf, outbuf_cmp,
                                                     out_samples.out_num * sizeof(short) * channel),
                                              0);
#else
                            fwrite(outbuf, 1, out_samples.out_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                        }
                        in = inbuf + in_samples.consume_num * channel * (config.bits_per_sample >> 3);
                        remain_num -= in_samples.consume_num;
                        in_samples.num = remain_num;
                        in_samples.samples = in;
                    }
                }
                esp_ae_sonic_close(sonic_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(in_buf);
                free(outbuf);
                free(out_buf);
                free(outbuf_cmp);
            }
        }
    }
    return 1;
}

static int sonic_interleave_32_bit_test(int mode, char *mode_name)
{
    // stream open
    float para[6] = {0.5, 0.75, 1.25, 1.5, 1.75, 2.0};
    int chan[5] = {1, 2};
    int sp[5] = {8, 44};
    int samplerate[5] = {8000, 44100};
    char out_name[100] = {0};
    char in_name[100] = {0};
    for (int j = 0; j < 1; j++) {
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 6; i++) {
                sprintf(in_name, "/sdcard/pcm/thetest%d_%d.pcm", sp[j], k + 1);
                FILE *infile = fopen(in_name, "rb");
                TEST_ASSERT_NOT_EQUAL(infile, NULL);
                sprintf(out_name, "/sdcard/sonic_test/asm/test32_%s%d_%d_%d.pcm", mode_name, sp[j], chan[k], i);
#ifdef CMP_MODE
                FILE *outfile = fopen(out_name, "rb");
#else
                FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
                TEST_ASSERT_NOT_EQUAL(outfile, NULL);
                int sample_rate = samplerate[j];
                int channel = chan[k];
                esp_ae_sonic_cfg_t config = {0};
                config.sample_rate = sample_rate;
                config.channel = channel;
                config.bits_per_sample = 32;
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
                int in_num = 512;
                int out_num = 512;
                char *inbuf = calloc(sizeof(short), in_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
                char *outbuf = calloc(sizeof(short), out_num * channel * sizeof(short));
                TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
                int *in_buf = calloc(sizeof(short), in_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
                int *out_buf = calloc(sizeof(short), out_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
                int *outbuf_cmp = calloc(sizeof(short), out_num * channel * sizeof(int));
                TEST_ASSERT_NOT_EQUAL(outbuf_cmp, NULL);
                void *sonic_handle = NULL;
                esp_ae_sonic_open(&config, &sonic_handle);
                TEST_ASSERT_NOT_EQUAL(sonic_handle, NULL);
                if (mode == 1) {
                    esp_ae_sonic_set_speed(sonic_handle, para[i]);
                } else if (mode == 2) {
                    esp_ae_sonic_set_pitch(sonic_handle, para[i]);
                }
                esp_ae_sonic_in_data_t in_samples = {0};
                esp_ae_sonic_out_data_t out_samples = {0};
                int *in;
                in_samples.samples = in_buf;
                out_samples.samples = out_buf;
                int in_read = 0;
                int sample_num = 0;
                int remain_num = 0;
                int ret = 0;
                while ((in_read = fread(inbuf, 1, in_num * channel * sizeof(short), infile)) > 0) {
                    sample_num = in_read / (channel * sizeof(short));
                    esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
                    remain_num = sample_num;
                    in_samples.samples = in_buf;
                    in_samples.num = sample_num;
                    out_samples.needed_num = 512;
                    while (remain_num > 0 || out_samples.out_num > 0) {
                        ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
                        TEST_ASSERT_EQUAL(ret, 0);
                        if (out_samples.out_num > 0) {
                            esp_ae_bit_cvt_process(b2_handle, out_samples.out_num,
                                                    out_samples.samples, outbuf);
#ifdef CMP_MODE
                            fread(outbuf_cmp, 1,
                                  out_samples.out_num * (16 >> 3) * channel, outfile);
                            TEST_ASSERT_EQUAL(memcmp(outbuf, outbuf_cmp,
                                                     out_samples.out_num * sizeof(short) * channel),
                                              0);
#else
                            fwrite(outbuf, 1, out_samples.out_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                        }
                        in = inbuf + in_samples.consume_num * channel * (config.bits_per_sample >> 3);
                        remain_num -= in_samples.consume_num;
                        in_samples.num = remain_num;
                        in_samples.samples = in;
                    }
                }
                esp_ae_sonic_close(sonic_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(inbuf);
                free(in_buf);
                free(outbuf);
                free(out_buf);
                free(outbuf_cmp);
            }
        }
    }
    return 1;
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

TEST_CASE("Sonic speed test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    sonic_interleave_16_bit_test(1, "speed");
    sonic_interleave_24_bit_test(1, "speed");
    sonic_interleave_32_bit_test(1, "speed");
    ae_sdcard_deinit();
}

TEST_CASE("Sonic pitch test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    sonic_interleave_16_bit_test(2, "pitch");
    sonic_interleave_24_bit_test(2, "pitch");
    sonic_interleave_32_bit_test(2, "pitch");
    ae_sdcard_deinit();
}
