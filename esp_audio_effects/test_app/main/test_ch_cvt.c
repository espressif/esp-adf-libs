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
#include "esp_ae_data_weaver.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_ch_cvt.h"

#define TAG "TEST_CHANNEL_CONVERT"
#define CMP_MODE

static int ch_cvt_interleave_16_bit_test(char *in, char *out, int sample_rate,
                                         int src_ch, int dest_ch, float *weight)
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

    esp_ae_ch_cvt_cfg_t config;
    config.sample_rate = sample_rate;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    config.weight = weight;
    config.weight_len = src_ch * dest_ch;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.bits_per_sample = 16;
    void *c_handle = NULL;
    char *buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(c_handle, NULL);

    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        esp_ae_ch_cvt_process(c_handle, sample_num, (void *)buffer, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }
    esp_ae_ch_cvt_close(c_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    return 0;
}

static int ch_cvt_uninterleave_16_bit_test(char *in, char *out, int sample_rate,
                                           int src_ch, int dest_ch, float *weight)
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

    esp_ae_ch_cvt_cfg_t config;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.sample_rate = sample_rate;
    config.bits_per_sample = 16;
    config.weight = weight;
    config.weight_len = src_ch * dest_ch;
    void *c_handle = NULL;
    char *buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *inbuf[10];
    char *outbuf[10];
    for (int i = 0; i < src_ch_num; i++) {
        inbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(inbuf[i], NULL);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        outbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }

    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(c_handle, NULL);

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    while ((in_read = fread(buffer, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        num++;
        esp_ae_deintlv_process(src_ch_num, 16, sample_num, buffer, inbuf);
        esp_ae_ch_cvt_deintlv_process(c_handle, sample_num, inbuf, outbuf);
        esp_ae_intlv_process(dest_ch_num, 16, sample_num, outbuf, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }
    esp_ae_ch_cvt_close(c_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    for (int i = 0; i < src_ch_num; i++) {
        free(inbuf[i]);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        free(outbuf[i]);
    }
    return 0;
}

static int ch_cvt_interleave_24_bit_test(char *in, char *out, int sample_rate,
                                         int src_ch, int dest_ch, float *weight)
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

    esp_ae_ch_cvt_cfg_t config;
    config.sample_rate = sample_rate;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    config.weight = weight;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.bits_per_sample = ESP_AE_BIT24;
    config.weight_len = src_ch * dest_ch;
    void *c_handle = NULL;
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = src_ch_num, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = dest_ch_num, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(ceill, NULL);

    char *inbuf = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    char *in_buf = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
    char *outbuf = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    char *out_buf = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(inbuf, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
        esp_ae_ch_cvt_process(c_handle, sample_num, (void *)in_buf, out_buf);
        esp_ae_bit_cvt_process(b2_handle, sample_num, out_buf, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(outbuf, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }
    esp_ae_ch_cvt_close(c_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(inbuf);
    free(in_buf);
    free(outbuf);
    free(out_buf);
    free(cmp_buffer);
    return 0;
}

static int ch_cvt_uninterleave_24_bit_test(char *in, char *out, int sample_rate,
                                           int src_ch, int dest_ch, float *weight)
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

    esp_ae_ch_cvt_cfg_t config;
    config.sample_rate = sample_rate;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    config.weight = weight;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.bits_per_sample = ESP_AE_BIT24;
    config.weight_len = src_ch * dest_ch;
    void *c_handle = NULL;
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = src_ch_num, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = dest_ch_num, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(c_handle, NULL);

    char *buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer_a = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer_a, NULL);
    char *buffer1 = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer1_a = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1_a, NULL);
    char *inbuf[10];
    char *outbuf[10];
    for (int i = 0; i < src_ch_num; i++) {
        inbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(inbuf[i], NULL);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        outbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    while ((in_read = fread(buffer, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        num++;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer_a);
        esp_ae_deintlv_process(src_ch_num, ESP_AE_BIT24, sample_num, buffer_a, inbuf);
        esp_ae_ch_cvt_deintlv_process(c_handle, sample_num, inbuf, outbuf);
        esp_ae_intlv_process(dest_ch_num, ESP_AE_BIT24, sample_num, outbuf, buffer1_a);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1_a, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }

    esp_ae_ch_cvt_close(c_handle);
    fclose(infile);
    fclose(outfile);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer_a);
    free(buffer1);
    free(buffer1_a);
    free(cmp_buffer);
    for (int i = 0; i < src_ch_num; i++) {
        free(inbuf[i]);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        free(outbuf[i]);
    }
    return 0;
}

static int ch_cvt_interleave_32_bit_test(char *in, char *out, int sample_rate,
                                         int src_ch, int dest_ch, float *weight)
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

    esp_ae_ch_cvt_cfg_t config;
    config.sample_rate = sample_rate;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    config.weight = weight;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.bits_per_sample = 32;
    config.weight_len = src_ch * dest_ch;
    void *c_handle = NULL;
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = src_ch_num, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = dest_ch_num, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(c_handle, NULL);

    char *inbuf = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    char *in_buf = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
    char *outbuf = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    char *out_buf = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(inbuf, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, inbuf, in_buf);
        esp_ae_ch_cvt_process(c_handle, sample_num, (void *)in_buf, out_buf);
        esp_ae_bit_cvt_process(b2_handle, sample_num, out_buf, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(outbuf, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }

    esp_ae_ch_cvt_close(c_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(inbuf);
    free(in_buf);
    free(outbuf);
    free(out_buf);
    free(cmp_buffer);
    return 0;
}

static int ch_cvt_uninterleave_32_bit_test(char *in, char *out, int sample_rate,
                                           int src_ch, int dest_ch, float *weight)
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
    esp_ae_ch_cvt_cfg_t config;
    config.sample_rate = sample_rate;
    config.src_ch = src_ch;
    config.dest_ch = dest_ch;
    config.weight = weight;
    int src_ch_num = config.src_ch;
    int dest_ch_num = config.dest_ch;
    config.weight_len = src_ch * dest_ch;
    config.bits_per_sample = 32;
    void *c_handle = NULL;
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = src_ch_num, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = dest_ch_num, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    int ret = esp_ae_ch_cvt_open(&config, &c_handle);
    TEST_ASSERT_NOT_EQUAL(c_handle, NULL);

    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer_a = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer_a, NULL);
    char *buffer1 = calloc(sizeof(int), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer1_a = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1_a, NULL);
    char *inbuf[10];
    char *outbuf[10];
    for (int i = 0; i < src_ch_num; i++) {
        inbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(inbuf[i], NULL);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        outbuf[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }

    int in_read = 0;
    int sample_num = 0;
    int num = 0;
    while ((in_read = fread(buffer, 1, 128 * src_ch_num * 2, infile)) > 0) {
        sample_num = in_read / src_ch_num / (16 >> 3);
        num++;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer_a);
        esp_ae_deintlv_process(src_ch_num, 32, sample_num, buffer_a, inbuf);
        esp_ae_ch_cvt_deintlv_process(c_handle, sample_num, inbuf, outbuf);
        esp_ae_intlv_process(dest_ch_num, 32, sample_num, outbuf, buffer1_a);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1_a, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * dest_ch_num * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * dest_ch_num * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }

    esp_ae_ch_cvt_close(c_handle);
    fclose(infile);
    fclose(outfile);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer_a);
    free(buffer1);
    free(buffer1_a);
    free(cmp_buffer);
    for (int i = 0; i < src_ch_num; i++) {
        free(inbuf[i]);
    }
    for (int i = 0; i < dest_ch_num; i++) {
        free(outbuf[i]);
    }
    return 0;
}

TEST_CASE("Channel Convert branch test", "AUDIO_EFFECT")
{
    esp_ae_ch_cvt_cfg_t config = {0};
    void *channel_handle = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_ch_cvt_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_open(NULL, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.sample_rate = 8000;
    config.bits_per_sample = 16;
    config.dest_ch = 0;
    config.src_ch = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.dest_ch = 2;
    config.src_ch = 0;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    config.bits_per_sample = 8;
    config.dest_ch = 2;
    config.src_ch = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    config.bits_per_sample = 16;
    config.weight = 123;
    config.weight_len = 1;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    float wei[2] = {1, 1.1};
    config.bits_per_sample = 16;
    config.weight = wei;
    config.weight_len = 2;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_ch_cvt_close(channel_handle);
    ESP_LOGI(TAG, "create channel_covert handle");
    config.bits_per_sample = 16;
    config.dest_ch = 3;
    config.src_ch = 8;
    config.weight = NULL;
    ret = esp_ae_ch_cvt_open(&config, &channel_handle);

    ESP_LOGI(TAG, "esp_ae_ch_cvt_process");
    char in_samples[100];
    char out_samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_process(NULL, sample_num, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_process(channel_handle, sample_num, NULL, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_ch_cvt_process(channel_handle, sample_num, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_ch_cvt_process(channel_handle, 0, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_ch_cvt_deintlv_process");
    char in_samples_1[2][100] = {0};
    char out_samples_1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_ch_cvt_deintlv_process(NULL, sample_num, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, sample_num, NULL, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, sample_num, in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, 0, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_ch_cvt_deintlv_process(channel_handle, 10, in_samples_1, out_samples_1);
    esp_ae_ch_cvt_close(channel_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Channel Convert 7.1 to 5.1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_7_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_8_6.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_8_6.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_8_6.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_8_6.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_8_6.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_8_6.pcm");
    float w_data[48] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 8, 6, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 8, 6, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 8, 6, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 8, 6, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 8, 6, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 8, 6, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 7.1 to 2.1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_7_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_8_3.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_8_3.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_8_3.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_8_3.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_8_3.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_8_3.pcm");
    float w_data[24] = {0.3, 0.0, 0.1, 0.3, 0.0, 0.3, 0.0, 0.0,
                        0.0, 0.3, 0.1, 0.0, 0.3, 0.0, 0.3, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 8, 3, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 8, 3, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 8, 3, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 8, 3, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 8, 3, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 8, 3, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 7.1 to 2 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_7_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_8_2.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_8_2.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_8_2.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_8_2.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_8_2.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_8_2.pcm");
    float w_data[16] = {0.3, 0.0, 0.1, 0.3, 0.0, 0.3, 0.0, 0.0,
                        0.0, 0.3, 0.1, 0.0, 0.3, 0.0, 0.3, 0.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 8, 2, NULL);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 8, 2, NULL);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 8, 2, NULL);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 8, 2, NULL);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 8, 2, NULL);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 8, 2, NULL);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 7.1 to 1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_7_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_8_1.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_8_1.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_8_1.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_8_1.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_8_1.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_8_1.pcm");
    float w_data[8] = {0.2, 0.1, 0.1, 0.2, 0.1, 0.2, 0.1, 0.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 8, 1, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 8, 1, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 8, 1, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 8, 1, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 8, 1, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 8, 1, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 5.1 to 5.1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_5_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_6_6.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_6_6.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_6_6.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_6_6.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_6_6.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_6_6.pcm");
    float w_data[18] = {0.4, 0.0, 0.2, 0.4, 0.0, 0.0,
                        0.0, 0.4, 0.2, 0.0, 0.4, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 6, 6, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 6, 6, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 6, 6, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 6, 6, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 6, 6, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 6, 6, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 5.1 to 2.1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_5_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_6_3.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_6_3.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_6_3.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_6_3.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_6_3.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_6_3.pcm");
    float w_data[18] = {0.4, 0.0, 0.2, 0.4, 0.0, 0.0,
                        0.0, 0.4, 0.2, 0.0, 0.4, 0.0,
                        0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 6, 3, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 6, 3, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 6, 3, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 6, 3, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 6, 3, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 6, 3, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 5.1 to 2 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_5_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_6_2.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_6_2.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_6_2.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_6_2.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_6_2.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_6_2.pcm");
    float w_data[12] = {0.4, 0.0, 0.2, 0.4, 0.0, 0.0,
                        0.0, 0.4, 0.2, 0.0, 0.4, 0.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 6, 2, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 6, 2, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 6, 2, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 6, 2, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 6, 2, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 6, 2, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 5.1 to 1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_5_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_6_1.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_6_1.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_6_1.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_6_1.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_6_1.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_6_1.pcm");
    float w_data[6] = {0.2, 0.2, 0.2, 0.2, 0.2, 0.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 6, 1, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 6, 1, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 6, 1, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 6, 1, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 6, 1, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 6, 1, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 2.1 to 2 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_2_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_3_2.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_3_2.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_3_2.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_3_2.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_3_2.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_3_2.pcm");
    float w_data[6] = {0.5, 0.0, 0.5,
                       0.0, 0.5, 0.5};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 3, 2, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 3, 2, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 3, 2, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 3, 2, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 3, 2, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 3, 2, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 2.1 to 1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_2_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_3_1.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_3_1.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_3_1.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_3_1.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_3_1.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_3_1.pcm");
    float w_data[3] = {0.4, 0.4, 0.2};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 3, 1, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 3, 1, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 3, 1, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 3, 1, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 3, 1, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 3, 1, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 2 to 1 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_2.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_2_1.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_2_1.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_2_1.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_2_1.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_2_1.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_2_1.pcm");
    float w_data[2] = {0.5, 0.5};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 2, 1, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 2, 1, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 2, 1, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 2, 1, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 2, 1, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 2, 1, w_data);
    ae_sdcard_deinit();
}

TEST_CASE("Channel Convert 1 to 2 test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name1[100];
    char out_name2[100];
    char out_name3[100];
    char out_name4[100];
    char out_name5[100];
    char out_name6[100];
    sprintf(in_name, "/sdcard/pcm/44100_1.pcm");
    sprintf(out_name1, "/sdcard/channel_convert/test_channel_convert_16_a_1_2.pcm");
    sprintf(out_name2, "/sdcard/channel_convert/test_channel_convert_16_b_1_2.pcm");
    sprintf(out_name3, "/sdcard/channel_convert/test_channel_convert_24_a_1_2.pcm");
    sprintf(out_name4, "/sdcard/channel_convert/test_channel_convert_24_b_1_2.pcm");
    sprintf(out_name5, "/sdcard/channel_convert/test_channel_convert_32_a_1_2.pcm");
    sprintf(out_name6, "/sdcard/channel_convert/test_channel_convert_32_b_1_2.pcm");
    float w_data[2] = {1.0, 1.0};
    ch_cvt_interleave_16_bit_test(in_name, out_name1, 44100, 1, 2, w_data);
    ch_cvt_uninterleave_16_bit_test(in_name, out_name2, 44100, 1, 2, w_data);
    ch_cvt_interleave_24_bit_test(in_name, out_name3, 44100, 1, 2, w_data);
    ch_cvt_uninterleave_24_bit_test(in_name, out_name4, 44100, 1, 2, w_data);
    ch_cvt_interleave_32_bit_test(in_name, out_name5, 44100, 1, 2, w_data);
    ch_cvt_uninterleave_32_bit_test(in_name, out_name6, 44100, 1, 2, w_data);
    ae_sdcard_deinit();
}
