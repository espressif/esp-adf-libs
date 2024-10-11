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
#include "esp_ae_fade.h"

#define TAG "TEST_FADE"
#define CMP_MODE

static int fade_interleave_16_bit_test(char *in_name, char *out_name, int sample_rate,
                                       int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#else
    FILE *outfile = fopen(out_name, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#endif /* CMP_MODE */
    char *buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *out = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(out, NULL);

    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 501;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = 16;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * (16 >> 3));
        cnt++;
        ret = esp_ae_fade_process(handle1, sample_num, buffer, out);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
#ifdef CMP_MODE
        fread(cmp_buffer, 1, 1024, outfile);
        TEST_ASSERT_EQUAL(memcmp(out, cmp_buffer, 1024), 0);
#else
        fwrite(out, 1, 1024, outfile);
#endif /* CMP_MODE */
    }

    esp_ae_fade_close(handle1);
    free(buffer);
    free(out);
    fclose(infile);
    fclose(outfile);
    free(cmp_buffer);
    ESP_LOGI(TAG, "end");
    return 0;
}

static int fade_uninterleave_16_bit_test(char *in_name, char *out_name, int sample_rate,
                                         int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#else
    FILE *outfile = fopen(out_name, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#endif /* CMP_MODE */
    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 501;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = 16;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);
    char *buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    char *in_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        in_buf[i] = calloc(1, 1024);
        TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
    }
    char *out_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        out_buf[i] = calloc(1, 1024);
        TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
    }

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * sizeof(short));
        esp_ae_deintlv_process(info1.channel, 16, sample_num, buffer, in_buf);
        cnt++;
        ret = esp_ae_fade_deintlv_process(handle1, sample_num, in_buf, out_buf);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
        esp_ae_intlv_process(info1.channel, 16, sample_num, out_buf, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, 1024, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, 1024), 0);
#else
        fwrite(buffer, 1, 1024, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_fade_close(handle1);
    free(buffer);
    free(cmp_buffer);
    for (int i = 0; i < info1.channel; i++) {
        free(in_buf[i]);
    }
    for (int i = 0; i < info1.channel; i++) {
        free(out_buf[i]);
    }
    fclose(infile);
    fclose(outfile);
    return 0;
}

static int fade_interleave_24_bit_test(char *in_name, char *out_name, int sample_rate,
                                       int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#else
    FILE *outfile = fopen(out_name, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
#endif /* CMP_MODE */
    char *buffer = calloc(2, 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(2, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(2, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);

    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 10000;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = ESP_AE_BIT24;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * (16 >> 3));
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        cnt++;
        ret = esp_ae_fade_process(handle1, sample_num, buffer1, buffer2);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer2, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, 1024, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, 1024), 0);
#else
        fwrite(buffer, 1, 1024, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_fade_close(handle1);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(cmp_buffer);
    fclose(infile);
    fclose(outfile);
    return 0;
}

static int fade_uninterleave_24_bit_test(char *in_name, char *out_name, int sample_rate,
                                         int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 10000;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = ESP_AE_BIT24;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    char *buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *in_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        in_buf[i] = calloc(2, 1024);
        TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
    }
    char *out_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        out_buf[i] = calloc(2, 1024);
        TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
    }

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * (16 >> 3));
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(info1.channel, ESP_AE_BIT24, sample_num, buffer1, in_buf);
        cnt++;
        ret = esp_ae_fade_deintlv_process(handle1, sample_num, in_buf, out_buf);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
        esp_ae_intlv_process(info1.channel, ESP_AE_BIT24, sample_num, out_buf, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * info1.channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * info1.channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * info1.channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }
    esp_ae_fade_close(handle1);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer1);
    for (int i = 0; i < info1.channel; i++) {
        free(in_buf[i]);
    }
    for (int i = 0; i < info1.channel; i++) {
        free(out_buf[i]);
    }
    fclose(infile);
    fclose(outfile);
    free(cmp_buffer);
    return 0;
}

static int fade_interleave_32_bit_test(char *in_name, char *out_name, int sample_rate,
                                       int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    char *buffer = calloc(2, 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(2, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(2, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);

    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 10000;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = 32;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * (16 >> 3));
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        cnt++;
        ret = esp_ae_fade_process(handle1, sample_num, buffer1, buffer2);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer2, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, 1024, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, 1024), 0);
#else
        fwrite(buffer, 1, 1024, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_fade_close(handle1);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer1);
    free(buffer2);
    fclose(infile);
    fclose(outfile);
    free(cmp_buffer);
    return 0;
}

static int fade_uninterleave_32_bit_test(char *in_name, char *out_name, int sample_rate,
                                         int channel, int cur_type)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    esp_ae_fade_cfg_t info1 = {0};
    info1.mode = ESP_AE_FADE_MODE_FADE_IN;
    info1.curve = cur_type;
    info1.transit_time = 10000;
    info1.sample_rate = sample_rate;
    info1.channel = channel;
    info1.bits_per_sample = 32;
    esp_ae_fade_handle_t handle1 = NULL;
    int ret = esp_ae_fade_open(&info1, &handle1);
    TEST_ASSERT_NOT_EQUAL(handle1, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    char *buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *cmp_buffer = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(2, 1024);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *in_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        in_buf[i] = calloc(2, 1024);
        TEST_ASSERT_NOT_EQUAL(in_buf[i], NULL);
    }
    char *out_buf[10] = {0};
    for (int i = 0; i < info1.channel; i++) {
        out_buf[i] = calloc(2, 1024);
        TEST_ASSERT_NOT_EQUAL(out_buf[i], NULL);
    }

    int in_read = 0;
    int cnt = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        int sample_num = in_read / (info1.channel * (16 >> 3));
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(info1.channel, 32, sample_num, buffer1, in_buf);
        cnt++;
        ret = esp_ae_fade_deintlv_process(handle1, sample_num, in_buf, out_buf);
        if (cnt == 10) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_OUT);
        }
        if (cnt == 100) {
            esp_ae_fade_set_mode(handle1, ESP_AE_FADE_MODE_FADE_IN);
        }
        esp_ae_intlv_process(info1.channel, 32, sample_num, out_buf, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * info1.channel * (16 >> 3), outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * info1.channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * info1.channel * (16 >> 3), outfile);
#endif /* CMP_MODE */
    }
    esp_ae_fade_close(handle1);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    for (int i = 0; i < info1.channel; i++) {
        free(in_buf[i]);
    }
    for (int i = 0; i < info1.channel; i++) {
        free(out_buf[i]);
    }
    fclose(infile);
    fclose(outfile);
    return 0;
}

TEST_CASE("Fade branch test", "AUDIO_EFFECT")
{
    esp_ae_fade_cfg_t config;
    void *fade_handle = NULL;
    config.mode = ESP_AE_FADE_MODE_FADE_OUT;
    config.curve = ESP_AE_FADE_CURVE_QUAD;
    config.transit_time = 3000;
    config.sample_rate = 44100;
    config.channel = 1;
    config.bits_per_sample = 16;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_fade_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_open(NULL, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.channel = 0;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.channel = 1;
    config.bits_per_sample = 8;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    config.bits_per_sample = 16;
    config.transit_time = 0;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    config.transit_time = 3000;
    config.mode = ESP_AE_FADE_MODE_INVALID;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    config.mode = ESP_AE_FADE_MODE_MAX;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    config.mode = ESP_AE_FADE_MODE_FADE_OUT;
    config.curve = ESP_AE_FADE_CURVE_INVALID;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test9");
    config.curve = ESP_AE_FADE_CURVE_MAX;
    ret = esp_ae_fade_open(&config, &fade_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create fade handle");
    config.curve = ESP_AE_FADE_CURVE_LINE;
    ret = esp_ae_fade_open(&config, &fade_handle);
    char samples[100];
    char samples_1[2][100] = {0};
    int sample_num = 10;
    ESP_LOGI(TAG, "esp_ae_fade_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_process(NULL, sample_num, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_process(fade_handle, sample_num, NULL, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_process(fade_handle, sample_num, samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_fade_process(fade_handle, 0, samples, samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_fade_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_fade_deintlv_process(NULL, sample_num, samples_1, samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_fade_deintlv_process(fade_handle, sample_num, NULL, samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_fade_deintlv_process(fade_handle, sample_num, samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_fade_deintlv_process(fade_handle, 0, samples_1, samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_fade_deintlv_process(fade_handle, sample_num, samples_1, samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_fade_close(fade_handle);
}

TEST_CASE("Fade Mono test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000.pcm");

    sprintf(out_name, "/sdcard/fade_test/test_fade1_0.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test_fade2_0.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test_fade3_0.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test_fade4_0.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test_fade5_0.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test_fade6_0.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_LINE);

    sprintf(out_name, "/sdcard/fade_test/test_fade1_1.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test_fade2_1.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test_fade3_1.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test_fade4_1.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test_fade5_1.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test_fade6_1.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_QUAD);

    sprintf(out_name, "/sdcard/fade_test/test_fade1_2.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test_fade2_2.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test_fade3_2.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test_fade4_2.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test_fade5_2.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test_fade6_2.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 1, ESP_AE_FADE_CURVE_SQRT);
    ae_sdcard_deinit();
}

TEST_CASE("Fade Dual test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");

    sprintf(out_name, "/sdcard/fade_test/test2_fade1_0.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test2_fade2_0.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test2_fade3_0.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test2_fade4_0.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test2_fade5_0.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);
    sprintf(out_name, "/sdcard/fade_test/test2_fade6_0.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_LINE);

    sprintf(out_name, "/sdcard/fade_test/test2_fade1_1.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test2_fade2_1.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test2_fade3_1.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test2_fade4_1.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test2_fade5_1.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);
    sprintf(out_name, "/sdcard/fade_test/test2_fade6_1.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_QUAD);

    sprintf(out_name, "/sdcard/fade_test/test2_fade1_2.pcm");
    fade_interleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test2_fade2_2.pcm");
    fade_uninterleave_16_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test2_fade3_2.pcm");
    fade_interleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test2_fade4_2.pcm");
    fade_uninterleave_24_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test2_fade5_2.pcm");
    fade_interleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    sprintf(out_name, "/sdcard/fade_test/test2_fade6_2.pcm");
    fade_uninterleave_32_bit_test(in_name, out_name, 8000, 2, ESP_AE_FADE_CURVE_SQRT);
    ae_sdcard_deinit();
}