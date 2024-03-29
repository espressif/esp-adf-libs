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
#include "esp_timer.h"
#include "test_common.h"
#include "esp_ae_data_weaver.h"

#define TAG "TEST_CROSS_DATA"
#define CMP_MODE

static int cross_data_test_16(char *in_name, char *out_name, int channel)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    char *in = calloc(sizeof(int), 1024 * 2);
    char *buf[5];
    for (int i = 0; i < channel; i++) {
        buf[i] = calloc(sizeof(int), 1024 * 2);
    }
    char *out = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(out, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    uint64_t st1 = 0;
    uint64_t end1 = 0;
    uint64_t diff1 = 0;
    uint64_t st2 = 0;
    uint64_t end2 = 0;
    uint64_t diff2 = 0;
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(in, 1, 1024, infile)) > 0) {
        sample_num = in_read / (16 >> 3) / channel;
        st1 = (uint64_t)esp_timer_get_time();
        esp_ae_deintlv_process(channel, 16, sample_num, in, buf);
        end1 = (uint64_t)esp_timer_get_time();
        diff1 += end1 - st1;
        st2 = (uint64_t)esp_timer_get_time();
        esp_ae_intlv_process(channel, 16, sample_num, buf, out);
        end2 = (uint64_t)esp_timer_get_time();
        diff2 += end2 - st2;
#ifdef CMP_MODE
        fread(cmp_buffer, 1, in_read, outfile);
        TEST_ASSERT_EQUAL(memcmp(out, cmp_buffer, in_read), 0);
#else
        fwrite(out, 1, in_read, outfile);
#endif /* CMP_MODE */
    }
    ESP_LOGI(TAG, "%02f %02f", (double)diff1 / 100000.0, (double)diff2 / 100000.0);
    fclose(infile);
    fclose(outfile);
    free(in);
    for (int i = 0; i < channel; i++) {
        free(buf[i]);
    }
    free(out);
    free(cmp_buffer);
    return 0;
}

static int cross_data_test_24(char *in_name, char *out_name, int channel)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);

    char *in = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(in, NULL);
    char *buf[5];
    for (int i = 0; i < channel; i++) {
        buf[i] = calloc(sizeof(int), 1024 * 2);
        TEST_ASSERT_NOT_EQUAL(buf[i], NULL);
    }
    char *out = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(out, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    uint64_t st1 = 0;
    uint64_t end1 = 0;
    uint64_t diff1 = 0;
    uint64_t st2 = 0;
    uint64_t end2 = 0;
    uint64_t diff2 = 0;
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(in, 1, 2400, infile)) > 0) {
        sample_num = in_read / (24 >> 3) / channel;
        st1 = (uint64_t)esp_timer_get_time();
        esp_ae_deintlv_process(channel, 24, sample_num, in, buf);
        end1 = (uint64_t)esp_timer_get_time();
        diff1 += end1 - st1;
        st2 = (uint64_t)esp_timer_get_time();
        esp_ae_intlv_process(channel, 24, sample_num, buf, out);
        end2 = (uint64_t)esp_timer_get_time();
        diff2 += end2 - st2;
#ifdef CMP_MODE
        fread(cmp_buffer, 1, in_read, outfile);
        TEST_ASSERT_EQUAL(memcmp(out, cmp_buffer, in_read), 0);
#else
        fwrite(out, 1, in_read, outfile);
#endif /* CMP_MODE */
    }
    ESP_LOGI(TAG, "%02f %02f", (double)diff1 / 100000.0, (double)diff2 / 100000.0);

    fclose(infile);
    fclose(outfile);
    free(in);
    for (int i = 0; i < channel; i++) {
        free(buf[i]);
    }
    free(out);
    free(cmp_buffer);
    return 0;
}

static int cross_data_test_32(char *in_name, char *out_name, int channel)
{
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile = fopen(out_name, "rb");
#else
    FILE *outfile = fopen(out_name, "wb");
#endif /* CMP_MODE */
    TEST_ASSERT_NOT_EQUAL(outfile, NULL);
    char *in = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(in, NULL);
    char *buf[5];
    for (int i = 0; i < channel; i++) {
        buf[i] = calloc(sizeof(int), 1024 * 2);
        TEST_ASSERT_NOT_EQUAL(buf[i], NULL);
    }
    char *out = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(out, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    uint64_t st1 = 0;
    uint64_t end1 = 0;
    uint64_t diff1 = 0;
    uint64_t st2 = 0;
    uint64_t end2 = 0;
    uint64_t diff2 = 0;
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(in, 1, 1024, infile)) > 0) {
        sample_num = in_read / (32 >> 3) / channel;
        st1 = (uint64_t)esp_timer_get_time();
        esp_ae_deintlv_process(channel, 32, sample_num, in, buf);
        end1 = (uint64_t)esp_timer_get_time();
        diff1 += end1 - st1;
        st2 = (uint64_t)esp_timer_get_time();
        esp_ae_intlv_process(channel, 32, sample_num, buf, out);
        end2 = (uint64_t)esp_timer_get_time();
        diff2 += end2 - st2;
#ifdef CMP_MODE
        fread(cmp_buffer, 1, in_read, outfile);
        TEST_ASSERT_EQUAL(memcmp(out, cmp_buffer, in_read), 0);
#else
        fwrite(out, 1, in_read, outfile);
#endif /* CMP_MODE */
    }
    ESP_LOGI(TAG, "%02f %02f", (double)diff1 / 100000.0, (double)diff2 / 100000.0);
    fclose(infile);
    fclose(outfile);
    free(in);
    for (int i = 0; i < channel; i++) {
        free(buf[i]);
    }
    free(out);
    free(cmp_buffer);
    return 0;
}

TEST_CASE("Crossdata branch test", "AUDIO_EFFECT")
{
    char in_samples[100];
    char out_samples[100];
    char *in_samples1[2][100] = {0};
    char *out_samples1[2][100] = {0};
    int sample_num = 10;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_deintlv_process(2, 16, 10, NULL, out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_deintlv_process(2, 16, 0, in_samples, out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_deintlv_process(0, 16, 10, in_samples, out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_deintlv_process(2, 8, 10, in_samples, out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    ret = esp_ae_deintlv_process(2, 16, 10, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    ret = esp_ae_deintlv_process(2, 16, 10, in_samples, out_samples1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);

    ESP_LOGI(TAG, "esp_ae_intlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_intlv_process(2, 16, 10, NULL, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_intlv_process(2, 16, 0, in_samples1, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_intlv_process(0, 16, 10, in_samples1, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_intlv_process(2, 8, 10, in_samples1, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    ret = esp_ae_intlv_process(2, 16, 10, in_samples1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    ret = esp_ae_intlv_process(2, 16, 10, in_samples1, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Crossdata Mono test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/thetest8_1.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_16_1.pcm");
    cross_data_test_16(in_name, out_name, 1);
    sprintf(in_name, "/sdcard/bit_convert/bit16_24_1.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_24_1.pcm");
    cross_data_test_24(in_name, out_name, 1);
    sprintf(in_name, "/sdcard/bit_convert/bit16_32_1.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_32_1.pcm");
    cross_data_test_32(in_name, out_name, 1);
    ae_sdcard_deinit();
}

TEST_CASE("Crossdata Dual test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char in_name[100];
    char out_name[100];
    sprintf(in_name, "/sdcard/pcm/thetest8_2.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_16_2.pcm");
    cross_data_test_16(in_name, out_name, 2);
    sprintf(in_name, "/sdcard/bit_convert/bit16_24_2.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_24_2.pcm");
    cross_data_test_24(in_name, out_name, 2);
    sprintf(in_name, "/sdcard/bit_convert/bit16_32_2.pcm");
    sprintf(out_name, "/sdcard/cross_data/crossdata_32_2.pcm");
    cross_data_test_32(in_name, out_name, 2);
    ae_sdcard_deinit();
}