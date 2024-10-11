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
#include "esp_ae_bit_cvt.h"
#include "esp_ae_data_weaver.h"

#define TAG "TEST_BIT_CONVERT"
#define CMP_MODE

static int bit16_convert_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    char *out_name4[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/bit16_32_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit16_8_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit16_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
    sprintf(out_name4, "/sdcard/bit_convert/bit16_16_%d.pcm", channel);
    FILE *outfile4 = fopen(out_name4, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile4, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/bit16_32_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit16_8_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit16_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
    sprintf(out_name4, "/sdcard/bit_convert/bit16_16_%d.pcm", channel);
    FILE *outfile4 = fopen(out_name4, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile4, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT8};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    void *b4_handle = NULL;
    esp_ae_bit_cvt_cfg_t b4_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b4_config, &b4_handle);
    TEST_ASSERT_NOT_EQUAL(b4_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *buffer4 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer4, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = in_read / channel / (16 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile1);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (32 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (32 >> 3), outfile1);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (8 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (8 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (8 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b3_handle, sample_num, buffer, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (24 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (24 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (24 >> 3), outfile3);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b4_handle, sample_num, buffer, buffer4);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile4);
        TEST_ASSERT_EQUAL(memcmp(buffer4, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer4, 1, sample_num * channel * (16 >> 3), outfile4);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    esp_ae_bit_cvt_close(b4_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    fclose(outfile4);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(buffer4);
    free(cmp_buffer);
    return 0;
}

static int bit8_convert_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/bit8_16_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit8_24_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit8_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/bit8_16_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit8_24_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit8_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT8, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT8, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT8, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = in_read / channel / (8 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile1);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (16 >> 3), outfile1);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (24 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (24 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (24 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b3_handle, sample_num, buffer, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (32 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (32 >> 3), outfile3);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(cmp_buffer);
    return 0;
}

static int bit24_convert_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/bit24_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit24_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit24_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/bit24_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit24_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit24_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT8};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = in_read / channel / (24 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (8 >> 3), outfile1);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (8 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (8 >> 3), outfile1);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (16 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b3_handle, sample_num, buffer, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (32 >> 3), outfile3);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(cmp_buffer);
    return 0;
}

static int bit32_convert_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    char *out_name4[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/bit32_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit32_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit32_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
    sprintf(out_name4, "/sdcard/bit_convert/bit32_32_%d.pcm", channel);
    FILE *outfile4 = fopen(out_name4, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile4, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/bit32_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/bit32_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/bit32_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
    sprintf(out_name4, "/sdcard/bit_convert/bit32_32_%d.pcm", channel);
    FILE *outfile4 = fopen(out_name4, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile4, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT8};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    void *b4_handle = NULL;
    esp_ae_bit_cvt_cfg_t b4_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b4_config, &b4_handle);
    TEST_ASSERT_NOT_EQUAL(b4_handle, NULL);

    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *buffer4 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer4, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = in_read / channel / (32 >> 3);
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (8 >> 3), outfile1);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (8 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (8 >> 3), outfile1);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (16 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b3_handle, sample_num, buffer, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (24 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (24 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (24 >> 3), outfile3);
#endif /* CMP_MODE */
        esp_ae_bit_cvt_process(b4_handle, sample_num, buffer, buffer4);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile4);
        TEST_ASSERT_EQUAL(memcmp(buffer4, cmp_buffer, sample_num * channel * (32 >> 3)), 0);
#else
        fwrite(buffer4, 1, sample_num * channel * (32 >> 3), outfile4);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    esp_ae_bit_cvt_close(b4_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    fclose(outfile4);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(buffer4);
    free(cmp_buffer);
    return 0;
}

static int bit16_convert_uninterleave_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit16_32_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit16_8_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit16_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit16_32_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit16_8_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit16_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 16, .dest_bits = 32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 16, .dest_bits = 8};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 16, .dest_bits = 24};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);

    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1_1[10];
    char *buffer2_1[10];
    char *buffer3_1[10];
    char *buffer1_2[10];
    char *buffer2_2[10];
    char *buffer3_2[10];
    for (int i = 0; i < channel; i++) {
        buffer1_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer1_1[i], NULL);
        buffer2_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer2_1[i], NULL);
        buffer3_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer3_1[i], NULL);
        buffer1_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer1_2[i], NULL);
        buffer2_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer2_2[i], NULL);
        buffer3_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer3_2[i], NULL);
    }
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = in_read / channel / (16 >> 3);
        esp_ae_deintlv_process(channel, 16, sample_num, buffer, buffer1_1);
        esp_ae_bit_cvt_deintlv_process(b1_handle, sample_num, buffer1_1, buffer1_2);
        esp_ae_intlv_process(channel, 32, sample_num, buffer1_2, buffer1);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile1);
        TEST_ASSERT_EQUAL(memcmp(buffer1, cmp_buffer, sample_num * channel * (32 >> 3)), 0);
#else
        fwrite(buffer1, 1, sample_num * channel * (32 >> 3), outfile1);
#endif /* CMP_MODE */
        esp_ae_deintlv_process(channel, 16, sample_num, buffer, buffer3_1);
        esp_ae_bit_cvt_deintlv_process(b3_handle, sample_num, buffer3_1, buffer3_2);
        esp_ae_intlv_process(channel, 24, sample_num, buffer3_2, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (24 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (24 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (24 >> 3), outfile3);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(buffer1_1[i]);
        free(buffer2_1[i]);
        free(buffer3_1[i]);
        free(buffer1_2[i]);
        free(buffer2_2[i]);
        free(buffer3_2[i]);
    }
    return 0;
}

static int bit24_convert_uninterleave_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit24_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit24_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit24_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit24_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit24_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit24_32_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 24, .dest_bits = 8};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 24, .dest_bits = 16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 24, .dest_bits = 32};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    char *buffer1_1[10];
    char *buffer2_1[10];
    char *buffer3_1[10];
    char *buffer1_2[10];
    char *buffer2_2[10];
    char *buffer3_2[10];
    for (int i = 0; i < channel; i++) {
        buffer1_1[i] = calloc(sizeof(int), 1024);
        buffer2_1[i] = calloc(sizeof(int), 1024);
        buffer3_1[i] = calloc(sizeof(int), 1024);
        buffer1_2[i] = calloc(sizeof(int), 1024);
        buffer2_2[i] = calloc(sizeof(int), 1024);
        buffer3_2[i] = calloc(sizeof(int), 1024);
    }
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = in_read / channel / (24 >> 3);
        esp_ae_deintlv_process(channel, 24, sample_num, buffer, buffer2_1);
        esp_ae_bit_cvt_deintlv_process(b2_handle, sample_num, buffer2_1, buffer2_2);
        esp_ae_intlv_process(channel, 16, sample_num, buffer2_2, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (16 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_deintlv_process(channel, 24, sample_num, buffer, buffer3_1);
        esp_ae_bit_cvt_deintlv_process(b3_handle, sample_num, buffer3_1, buffer3_2);
        esp_ae_intlv_process(channel, 32, sample_num, buffer3_2, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (32 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (32 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (32 >> 3), outfile3);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(buffer1_1[i]);
        free(buffer2_1[i]);
        free(buffer3_1[i]);
        free(buffer1_2[i]);
        free(buffer2_2[i]);
        free(buffer3_2[i]);
    }
    return 0;
}

static int bit32_convert_uninterleave_test(char *in_name, int sample_rate, int channel)
{
    char *out_name1[100];
    char *out_name2[100];
    char *out_name3[100];
    FILE *infile = fopen(in_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit32_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit32_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit32_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "rb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#else
    sprintf(out_name1, "/sdcard/bit_convert/unin_bit32_8_%d.pcm", channel);
    FILE *outfile1 = fopen(out_name1, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    sprintf(out_name2, "/sdcard/bit_convert/unin_bit32_16_%d.pcm", channel);
    FILE *outfile2 = fopen(out_name2, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
    sprintf(out_name3, "/sdcard/bit_convert/unin_bit32_24_%d.pcm", channel);
    FILE *outfile3 = fopen(out_name3, "wb");
    TEST_ASSERT_NOT_EQUAL(outfile3, NULL);
#endif /* CMP_MODE */
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 32, .dest_bits = 8};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 32, .dest_bits = 16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    void *b3_handle = NULL;
    esp_ae_bit_cvt_cfg_t b3_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = 32, .dest_bits = 24};
    esp_ae_bit_cvt_open(&b3_config, &b3_handle);
    TEST_ASSERT_NOT_EQUAL(b3_handle, NULL);
    char *buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    char *buffer2 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer2, NULL);
    char *buffer3 = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer3, NULL);
    char *cmp_buffer = calloc(sizeof(int), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1_1[10];
    char *buffer2_1[10];
    char *buffer3_1[10];
    char *buffer1_2[10];
    char *buffer2_2[10];
    char *buffer3_2[10];
    for (int i = 0; i < channel; i++) {
        buffer1_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer1_1[i], NULL);
        buffer2_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer2_1[i], NULL);
        buffer3_1[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer3_1[i], NULL);
        buffer1_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer1_2[i], NULL);
        buffer2_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer2_2[i], NULL);
        buffer3_2[i] = calloc(sizeof(int), 1024);
        TEST_ASSERT_NOT_EQUAL(buffer3_2[i], NULL);
    }
    int in_read = 0;
    int sample_num = 0;
    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = in_read / channel / (32 >> 3);
        esp_ae_deintlv_process(channel, 32, sample_num, buffer, buffer2_1);
        esp_ae_bit_cvt_deintlv_process(b2_handle, sample_num, buffer2_1, buffer2_2);
        esp_ae_intlv_process(channel, 16, sample_num, buffer2_2, buffer2);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (16 >> 3), outfile2);
        TEST_ASSERT_EQUAL(memcmp(buffer2, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer2, 1, sample_num * channel * (16 >> 3), outfile2);
#endif /* CMP_MODE */
        esp_ae_deintlv_process(channel, 32, sample_num, buffer, buffer3_1);
        esp_ae_bit_cvt_deintlv_process(b3_handle, sample_num, buffer3_1, buffer3_2);
        esp_ae_intlv_process(channel, 24, sample_num, buffer3_2, buffer3);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * channel * (24 >> 3), outfile3);
        TEST_ASSERT_EQUAL(memcmp(buffer3, cmp_buffer, sample_num * channel * (24 >> 3)), 0);
#else
        fwrite(buffer3, 1, sample_num * channel * (24 >> 3), outfile3);
#endif /* CMP_MODE */
    }
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    esp_ae_bit_cvt_close(b3_handle);
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    fclose(outfile3);
    free(buffer);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(buffer1_1[i]);
        free(buffer2_1[i]);
        free(buffer3_1[i]);
        free(buffer1_2[i]);
        free(buffer2_2[i]);
        free(buffer3_2[i]);
    }
    return 0;
}

TEST_CASE("Bit Convert branch test", "AUDIO_EFFECT")
{
    esp_ae_bit_cvt_cfg_t config;
    void *bit_handle = NULL;
    int ret = 0;
    ESP_LOGI(TAG, "esp_ae_bit_cvt_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_open(NULL, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_open(&config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    config.channel = 0;
    config.src_bits = ESP_AE_BIT8;
    config.dest_bits = ESP_AE_BIT16;
    config.sample_rate = 8000;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    config.channel = 1;
    config.src_bits = ESP_AE_BIT8;
    config.dest_bits = 64;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    config.channel = 1;
    config.src_bits = 64;
    config.dest_bits = ESP_AE_BIT8;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create bit_covert handle");
    config.channel = 1;
    config.src_bits = ESP_AE_BIT16;
    config.dest_bits = ESP_AE_BIT8;
    ret = esp_ae_bit_cvt_open(&config, &bit_handle);
    ESP_LOGI(TAG, "esp_ae_bit_cvt_process");
    char in_samples[100];
    char out_samples[100];
    int sample_num = 10;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_process(NULL, sample_num, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_process(bit_handle, sample_num, NULL, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_bit_cvt_process(bit_handle, sample_num, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_bit_cvt_process(bit_handle, 0, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_bit_cvt_deintlv_process");
    char in_samples_1[2][100] = {0};
    char out_samples_1[2][100] = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_bit_cvt_deintlv_process(NULL, sample_num, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, sample_num, NULL, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, sample_num, in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, 0, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_bit_cvt_deintlv_process(bit_handle, 10, in_samples_1, out_samples_1);
    esp_ae_bit_cvt_close(bit_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
}

TEST_CASE("Bit Convert Mono test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char *in_name[100];
    int channel = 1;
    int sample_rate = 8000;
    sprintf(in_name, "/sdcard/pcm/test_8000.pcm");
    bit16_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_8_%d.pcm", channel);
    bit8_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_24_%d.pcm", channel);
    bit24_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_32_%d.pcm", channel);
    bit32_convert_test(in_name, sample_rate, channel);

    sprintf(in_name, "/sdcard/pcm/test_8000.pcm");
    bit16_convert_uninterleave_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/unin_bit16_24_%d.pcm", channel);
    bit24_convert_uninterleave_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/unin_bit16_32_%d.pcm", channel);
    bit32_convert_uninterleave_test(in_name, sample_rate, channel);
    ae_sdcard_deinit();
}

TEST_CASE("Bit Convert Dual test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    char *in_name[100];
    int channel = 2;
    int sample_rate = 8000;
    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    bit16_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_8_%d.pcm", channel);
    bit8_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_24_%d.pcm", channel);
    bit24_convert_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/bit16_32_%d.pcm", channel);
    bit32_convert_test(in_name, sample_rate, channel);

    sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
    bit16_convert_uninterleave_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/unin_bit16_24_%d.pcm", channel);
    bit24_convert_uninterleave_test(in_name, sample_rate, channel);
    sprintf(in_name, "/sdcard/bit_convert/unin_bit16_32_%d.pcm", channel);
    bit32_convert_uninterleave_test(in_name, sample_rate, channel);
    ae_sdcard_deinit();
}