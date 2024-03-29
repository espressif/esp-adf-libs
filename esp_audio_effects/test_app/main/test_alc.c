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
#include "esp_ae_alc.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_data_weaver.h"

#define TAG "TEST_ALC"

int count  = 0;
int count1 = 10000000;
int count2 = 10000000;

#define CMP_MODE

static int alc_interleave_32_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = 32;
    void *alc_handle = NULL;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate,
                                       .channel = channel,
                                       .src_bits = ESP_AE_BIT16,
                                       .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / channel / (16 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_alc_process(alc_handle, sample_num, (void *)buffer1, (void *)buffer2);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer2, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(cmp_buffer);
    return 0;
}

static int alc_uninterleave_32_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = 32;
    void *alc_handle;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate,
                                       .channel = channel,
                                       .src_bits = ESP_AE_BIT16,
                                       .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate,
                                       .channel = channel,
                                       .src_bits = ESP_AE_BIT32,
                                       .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    char *buffer = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *outbuf[10] = {0};
    char *outbuf1[10] = {0};
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(int), 1200);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
        outbuf1[i] = calloc(sizeof(int), 1200);
        TEST_ASSERT_NOT_EQUAL(outbuf1[i], NULL);
    }
    int in_read = 0;
    int sample_num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(channel, 32, sample_num, buffer1, outbuf);
        esp_ae_alc_deintlv_process(alc_handle, sample_num, outbuf, outbuf1);
        esp_ae_intlv_process(channel, 32, sample_num, outbuf1, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(outbuf[i]);
        free(outbuf1[i]);
    }
    return 0;
}

static int alc_interleave_24_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = 24;
    void *alc_handle = NULL;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate,
                                       .channel = channel,
                                       .src_bits = ESP_AE_BIT16, 
                                       .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate,
                                       .channel = channel, 
                                       .src_bits = ESP_AE_BIT24, 
                                       .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / channel / (16 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_alc_process(alc_handle, sample_num, (void *)buffer1, (void *)buffer2);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer2, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(cmp_buffer);
    free(buffer1);
    free(buffer2);
    return 0;
}

static int alc_uninterleave_24_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = ESP_AE_BIT24;
    void *alc_handle;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, 
                                       .channel = channel, 
                                       .src_bits = ESP_AE_BIT16, 
                                       .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, 
                                       .channel = channel, 
                                       .src_bits = ESP_AE_BIT24, 
                                       .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    char *buffer = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1200 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

    char *outbuf[10] = {0};
    char *outbuf1[10] = {0};
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(int), 1200);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
        outbuf1[i] = calloc(sizeof(int), 1200);
        TEST_ASSERT_NOT_EQUAL(outbuf1[i], NULL);
    }
    int in_read = 0;
    int sample_num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(channel, ESP_AE_BIT24, sample_num, buffer1, outbuf);
        esp_ae_alc_deintlv_process(alc_handle, sample_num, outbuf, outbuf1);
        esp_ae_intlv_process(channel, ESP_AE_BIT24, sample_num, outbuf1, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(cmp_buffer);
    free(buffer1);
    for (int i = 0; i < channel; i++) {
        free(outbuf[i]);
        free(outbuf1[i]);
    }
    return 0;
}

static int alc_interleave_16_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = 16;
    void *alc_handle = NULL;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);

    char *buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / channel / (16 >> 3);
        esp_ae_alc_process(alc_handle, sample_num, (void *)buffer, (void *)buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    return 0;
}

static int alc_uninterleave_16_bit_test(char *in, char *out, int channel, int sample_rate, int gain)
{
    // stream open
    FILE *infile = fopen(in, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out, "rb");
#else
    FILE *outfile = fopen(out, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_alc_cfg_t config;
    config.sample_rate = sample_rate;
    config.channel = channel;
    config.bits_per_sample = 16;
    void *alc_handle;
    int ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_NOT_EQUAL(alc_handle, NULL);
    short *buffer = calloc(sizeof(short), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    short *outbuf[10] = {0};
    short *outbuf1[10] = {0};
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(cmp_buffer, outbuf[i]);
        outbuf1[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(cmp_buffer, outbuf1[i]);
    }
    int in_read = 0;
    int sample_num = 0;
    for (int i = 0; i < channel; i++) {
        esp_ae_alc_set_gain(alc_handle, i, gain);
    }
    while ((in_read = fread(buffer, 1, 1024 * 2, infile)) > 0) {
        if (count == count1) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, 10);
            }
        }
        if (count == count2) {
            for (int i = 0; i < channel; i++) {
                esp_ae_alc_set_gain(alc_handle, i, -10);
            }
        }
        sample_num = in_read / (16 >> 3) / channel;
        esp_ae_deintlv_process(channel, 16, sample_num, buffer, outbuf);
        esp_ae_alc_deintlv_process(alc_handle, sample_num, outbuf, outbuf1);
        esp_ae_intlv_process(channel, 16, sample_num, outbuf1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, in_read, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, in_read), 0);
#else
        fwrite(buffer, 1, in_read, outfile);
#endif /* CMP_MODE */
        count++;
    }
    esp_ae_alc_close(alc_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    for (int i = 0; i < channel; i++) {
        free(outbuf[i]);
        free(outbuf1[i]);
    }
    free(cmp_buffer);
    return 0;
}

TEST_CASE("Alc branch test", "AUDIO_EFFECT")
{
    esp_ae_alc_cfg_t config;
    config.channel = 1;
    config.bits_per_sample = 32;
    config.sample_rate = 44100;
    void *alc_handle = NULL;
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
    config.sample_rate = 44100;
    ret = esp_ae_alc_open(&config, &alc_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    ESP_LOGI(TAG, "esp_ae_alc_set_gain");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_set_gain(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_set_gain(alc_handle, 0, 65);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_alc_get_gain");
    ESP_LOGI(TAG, "test1");
    int gain = 0;
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

    ESP_LOGI(TAG, "esp_ae_alc_deintlv_process");
    char samples1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_alc_deintlv_process(NULL, sample_num, samples1, samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, NULL, samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_alc_deintlv_process(alc_handle, 0, samples1, samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_alc_deintlv_process(alc_handle, sample_num, samples1, samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_alc_close(alc_handle);
}

TEST_CASE("Alc add test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int gain[5] = {0, 5, 15, 30};
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    for (int i = 0; i < 4; i++) {
        ESP_LOGI(TAG, "volume:%d", gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_0_%d.pcm", gain[i]);
        alc_interleave_16_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_1_%d.pcm", gain[i]);
        alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_2_%d.pcm", gain[i]);
        alc_interleave_24_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_3_%d.pcm", gain[i]);
        alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_4_%d.pcm", gain[i]);
        alc_interleave_32_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_add_5_%d.pcm", gain[i]);
        alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, gain[i]);
    }
    ae_sdcard_deinit();
}

TEST_CASE("Alc sub test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    int gain[5] = {-30, -15, -5};
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "volume:%d", gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_0_%d.pcm", gain[i]);
        alc_interleave_16_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_1_%d.pcm", gain[i]);
        alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_2_%d.pcm", gain[i]);
        alc_interleave_24_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_3_%d.pcm", gain[i]);
        alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_4_%d.pcm", gain[i]);
        alc_interleave_32_bit_test(in_name, out_name, 2, 8000, gain[i]);
        sprintf(out_name, "/sdcard/alc_test/test_alc_sub_5_%d.pcm", gain[i]);
        alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, gain[i]);
    }
    ae_sdcard_deinit();
}

TEST_CASE("Alc boundary test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_0.pcm");
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, 63);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_1.pcm");
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, 63);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_2.pcm");
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, 63);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_3.pcm");
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, 63);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_4.pcm");
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, 63);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_bd_5.pcm");
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, 63);

    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_0.pcm");
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, -64);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_1.pcm");
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, -64);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_2.pcm");
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, -64);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_3.pcm");
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, -64);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_4.pcm");
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, -64);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_bd_5.pcm");
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, -64);
    ae_sdcard_deinit();
}

TEST_CASE("Alc stay add sub", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_0.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_1.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_2.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_3.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_4.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_add_sub_5.pcm");
    count1 = 20;
    count2 = 60;
    count = 0;
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    ae_sdcard_deinit();
}

TEST_CASE("Alc stay sub add", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_0.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_1.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_2.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_3.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_4.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_stay_sub_add_5.pcm");
    count1 = 60;
    count2 = 20;
    count = 0;
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    ae_sdcard_deinit();
}

TEST_CASE("Alc add sub", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_0.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_1.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_2.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_3.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_4.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_add_sub_5.pcm");
    count1 = 0;
    count2 = 60;
    count = 0;
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    ae_sdcard_deinit();
}

TEST_CASE("Alc sub add", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_0.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_interleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_1.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_uninterleave_16_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_2.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_interleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_3.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_uninterleave_24_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_4.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_interleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    sprintf(out_name, "/sdcard/alc_test/test_alc_sub_add_5.pcm");
    count1 = 60;
    count2 = 0;
    count = 0;
    alc_uninterleave_32_bit_test(in_name, out_name, 2, 8000, 0);
    ae_sdcard_deinit();
}
