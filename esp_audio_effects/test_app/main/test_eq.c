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
#include "esp_ae_eq.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_data_weaver.h"

#define TAG "TEST_EQ"
#define CMP_MODE

static int music_mode[10][10] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},         // default
    {9, 8, 5, 2, 1, 0, -3, -4, -3, 0},      // dance
    {8, 8, 8, 7, 4, 0, -3, -5, -7, -9},     // full buss
    {-9, -8, -7, -6, -3, 1, 5, 8, 10, 12},  // full treble
    {-2, -1, 0, 2, 3, 2, 0, -2, -2, -1},    // pop
    {6, 5, 2, -2, -5, -2, 0, 3, 5, 6},      // rock
    {2, 1, 0, 0, -1, 0, 1, 2, 3, 4},        // soft
    {8, 7, 6, 3, 2, 0, -1, -2, -1, 0},      // large_hall
    {4, 4, 3, 2, 0, 0, 0, 0, 0, 4},         // party
    {0, 0, 0, 1, 2, 3, 3, 2, 1, 0}          // club
};

static esp_ae_eq_filter_type_t ft[5]   = {ESP_AE_EQ_FILTER_HIGH_PASS, ESP_AE_EQ_FILTER_LOW_PASS, ESP_AE_EQ_FILTER_PEAK,
                                       ESP_AE_EQ_FILTER_HIGH_SHELF, ESP_AE_EQ_FILTER_LOW_SHELF};
static float                q[5]    = {0.5, 1.0, 1.5, 2.0, 3.0};
static float                gain[5] = {-15.0, -5.0, 0.0, 5.0, 15.0};

static void filter_para_cfg(int fc, float q, float gain, esp_ae_eq_filter_type_t filter_type,
                            esp_ae_eq_filter_para_t *eq_para)
{
    eq_para->filter_type = filter_type;
    if (filter_type == ESP_AE_EQ_FILTER_HIGH_PASS || filter_type == ESP_AE_EQ_FILTER_LOW_PASS) {
        eq_para->fc = fc;
        eq_para->q = q;
    } else {
        eq_para->fc = fc;
        eq_para->q = q;
        eq_para->gain = gain;
    }
}

static esp_ae_eq_cfg_t *eq_config_single(int sample_rate, int channel, int bit,
                                      int fc, float q, float gain,
                                      esp_ae_eq_filter_type_t filter_type)
{
    esp_ae_eq_cfg_t *eq_config = calloc(1, sizeof(esp_ae_eq_cfg_t));
    if (eq_config == NULL) {
        ESP_LOGI(TAG, "eq config calloc error.");
        return NULL;
    }
    eq_config->bits_per_sample = bit;
    eq_config->sample_rate = sample_rate;
    eq_config->channel = channel;
    eq_config->filter_num = 1;
    eq_config->para = calloc(1, sizeof(esp_ae_eq_filter_para_t) * eq_config->filter_num);
    if (eq_config->para == NULL) {
        ESP_LOGI(TAG, "para calloc error.");
        free(eq_config);
        return NULL;
    }
    esp_ae_eq_filter_para_t *para = eq_config->para;
    filter_para_cfg(fc, q, gain, filter_type, para);
    return eq_config;
}

static esp_ae_eq_cfg_t *eq_config_para(int sample_rate, int channel, int bit)
{
    esp_ae_eq_cfg_t *eq_config = calloc(1, sizeof(esp_ae_eq_cfg_t));
    if (eq_config == NULL) {
        ESP_LOGI(TAG, "eq config calloc error.");
        return NULL;
    }
    eq_config->bits_per_sample = bit;
    eq_config->sample_rate = sample_rate;
    eq_config->channel = channel;
    eq_config->filter_num = 10;
    eq_config->para = calloc(1, sizeof(esp_ae_eq_filter_para_t) * eq_config->filter_num);
    if (eq_config->para == NULL) {
        ESP_LOGI(TAG, "para calloc error.");
        free(eq_config);
        return NULL;
    }
    esp_ae_eq_filter_para_t *para = eq_config->para;
    int fc[15] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    float q[15] = {0.0};
    float gain[15];
    for (int i = 0; i < eq_config->filter_num; i++) {
        q[i] = 2.0;
    }
    for (int i = 0; i < eq_config->filter_num; i++) {
        // gain[i] = (float)music_mode[3][i];
        gain[i] = 5;
    }
    esp_ae_eq_filter_type_t filter_type[15] = {ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK,
                                            ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK,
                                            ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK, ESP_AE_EQ_FILTER_PEAK,
                                            ESP_AE_EQ_FILTER_PEAK};
    for (int i = 0; i < eq_config->filter_num; i++) {
        filter_para_cfg(fc[i], q[i], gain[i], filter_type[i], para);
        para++;
    }
    return eq_config;
}

static int infile_open(FILE **file, char *name)
{
    FILE *infile = fopen(name, "rb");
    if (!infile) {
        ESP_LOGI(TAG, "infile open error");
        return -1;
    }
    *file = infile;
    return 0;
}

static int outfile_open(FILE **file, char *name)
{
#ifdef CMP_MODE
    FILE *infile = fopen(name, "rb");
#else
    FILE *infile = fopen(name, "wb");
#endif /* CMP_MODE */
    if (!infile) {
        ESP_LOGI(TAG, "infile open error");
        return -1;
    }
    *file = infile;
    return 0;
}

TEST_CASE("EQ branch test", "AUDIO_EFFECT")
{
    esp_ae_eq_handle_t eq_hd = NULL;
    int ret;
    esp_ae_eq_cfg_t *eq_config = calloc(1, sizeof(esp_ae_eq_cfg_t));
    if (eq_config == NULL) {
        ESP_LOGI(TAG, "eq config calloc error.");
        return;
    }
    eq_config->bits_per_sample = 16;
    eq_config->sample_rate = 8000;
    eq_config->channel = 1;
    eq_config->filter_num = 2;
    eq_config->para = calloc(1, sizeof(esp_ae_eq_filter_para_t) * eq_config->filter_num);
    if (eq_config->para == NULL) {
        ESP_LOGI(TAG, "filter_para calloc error.");
        free(eq_config);
        return;
    }
    esp_ae_eq_filter_para_t *para = eq_config->para;
    esp_ae_eq_filter_para_t *para1 = para + 1;
    filter_para_cfg(500, 1.0, 5.0, ESP_AE_EQ_FILTER_HIGH_PASS, para);
    filter_para_cfg(1000, 1.0, 5.0, ESP_AE_EQ_FILTER_HIGH_SHELF, para1);

    ESP_LOGI(TAG, "esp_ae_eq_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_open(NULL, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_open(eq_config, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    eq_config->bits_per_sample = 8;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    eq_config->bits_per_sample = 16;
    eq_config->sample_rate = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    eq_config->sample_rate = 8000;
    eq_config->channel = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    eq_config->channel = 1;
    eq_config->filter_num = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test7");
    eq_config->filter_num = 2;
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_INVALID;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test8");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_MAX;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test9");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 5000;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test10");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test11");
    eq_config->para->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    eq_config->para->fc = 2000;
    eq_config->para->q = 0.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test12");
    eq_config->para->fc = 5000;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test13");
    eq_config->para->fc = 0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test14");
    eq_config->para->fc = 2000;
    eq_config->para->q = 0.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test15");
    eq_config->para->q = 1.0;
    para1->gain = 16.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test16");
    para1->gain = -16.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test17");
    para1->gain = -14.0;
    para1->fc = 20;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_eq_close(eq_hd);
    ESP_LOGI(TAG, "test18");
    para1->fc = 1000;
    para1->filter_type = ESP_AE_EQ_FILTER_HIGH_PASS;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    esp_ae_eq_close(eq_hd);
    ESP_LOGI(TAG, "create eq handle");
    eq_config->para->gain = 5.0;
    ret = esp_ae_eq_open(eq_config, &eq_hd);
    char in_samples[100];
    char in_samples_1[2][100] = {0};
    int sample_num = 10;
    ESP_LOGI(TAG, "esp_ae_eq_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_process(NULL, sample_num, in_samples, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_process(eq_hd, sample_num, NULL, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_process(eq_hd, sample_num, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_process(eq_hd, 0, in_samples, in_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_eq_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_deintlv_process(NULL, sample_num, in_samples_1, in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, NULL, in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_deintlv_process(eq_hd, 0, in_samples_1, in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_deintlv_process(eq_hd, sample_num, in_samples_1, in_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_eq_set_filter_para");
    esp_ae_eq_filter_para_t para_1;
    para_1.filter_type = ESP_AE_EQ_FILTER_PEAK;
    para_1.fc = 500;
    para_1.q = 1.0;
    para_1.gain = -5.0;
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_set_filter_para(NULL, 0, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_set_filter_para(eq_hd, 2, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, &para_1);
    ret = esp_ae_eq_set_filter_para(eq_hd, 0, &para_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    ESP_LOGI(TAG, "esp_ae_eq_get_filter_para");
    esp_ae_eq_filter_para_t fi_para = {0};
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_get_filter_para(NULL, 0, &fi_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_get_filter_para(eq_hd, 2, &fi_para);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_eq_get_filter_para(eq_hd, 0, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_eq_get_filter_para(eq_hd, 0, &fi_para);
    TEST_ASSERT_EQUAL(fi_para.filter_type, ESP_AE_EQ_FILTER_PEAK);
    TEST_ASSERT_EQUAL(fi_para.fc, 500);
    TEST_ASSERT_EQUAL(fi_para.q, 1.0);
    TEST_ASSERT_EQUAL(fi_para.gain, -5.0);
    ESP_LOGI(TAG, "%d %d %02f %02f", fi_para.filter_type, (int)fi_para.fc, fi_para.q, fi_para.gain);
    ESP_LOGI(TAG, "esp_ae_eq_enable_filter");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_enable_filter(NULL, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_enable_filter(eq_hd, 2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_eq_disable_filter");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_eq_disable_filter(NULL, 0);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_eq_disable_filter(eq_hd, 2);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    esp_ae_eq_close(eq_hd);
    free(eq_config->para);
    free(eq_config);
}

TEST_CASE("EQ set test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 16;
    int channel = 1;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);

    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);

    short *buffer = calloc(sizeof(short), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    esp_ae_eq_filter_para_t para_test;
    para_test.filter_type = ESP_AE_EQ_FILTER_LOW_PASS;
    para_test.fc = 500;
    para_test.q = 1.0;
    esp_ae_eq_set_filter_para(eq_handle, 0, &para_test);
    esp_ae_eq_filter_para_t para;
    esp_ae_eq_get_filter_para(eq_handle, 0, &para);
    TEST_ASSERT_EQUAL(para.filter_type, ESP_AE_EQ_FILTER_LOW_PASS);
    TEST_ASSERT_EQUAL(para.fc, 500);
    TEST_ASSERT_EQUAL(para.q, 1.0);

    esp_ae_eq_close(eq_handle);
    free(buffer);
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ interleave 16 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    int sample_rate = 44100;
    int bit = 16;
    int channel = 1;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/16_c/test_para_eq1.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);

    short *buffer = calloc(sizeof(short), 1024);
    short *cmp_buffer = calloc(sizeof(short), 1024);
    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = 1200 / (16 >> 3) / channel;
        ret = esp_ae_eq_process(eq_handle, sample_num, buffer, buffer);
        TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(cmp_buffer);
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ deinterleave 16 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 16;
    int channel = 1;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/16_c_inter/test_para_eq2.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);

    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);

    short *buffer = calloc(sizeof(short), 1024);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    short *cmp_buffer = calloc(sizeof(short), 1024);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    short *outbuf[10];
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(short), 512);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }

    while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
        sample_num = 1024 / 2 / channel;
        esp_ae_deintlv_process(channel, 16, sample_num, buffer, outbuf);
        esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
        esp_ae_intlv_process(channel, 16, sample_num, outbuf, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        if (outbuf[i] != NULL) {
            free(outbuf[i]);
        }
    }
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ interleave 24 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 24;
    int channel = 1;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/24_c/test_para_eq3.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);
    char *buffer = calloc(1, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    char *buffer1 = calloc(1, 1024 * 2 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = 1200 / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_eq_process(eq_handle, sample_num, buffer1, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ deinterleave 24 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 24;
    int channel = 1;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/24_c_inter/test_para_eq4.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    short *buffer = calloc(sizeof(short), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    short *buffer1 = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    short *outbuf[10];
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(short), 512 * 2 * 2);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }
    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = 1200 / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(channel, ESP_AE_BIT24, sample_num, buffer1, outbuf);
        esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
        esp_ae_intlv_process(channel, ESP_AE_BIT24, sample_num, outbuf, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        if (outbuf[i] != NULL) {
            free(outbuf[i]);
        }
    }
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ interleave 32 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 16;
    int channel = 1;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/32_c/test_para_eq5.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    char *buffer = calloc(1, 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    char *buffer1 = calloc(1, 1024 * 2 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);

    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = 1200 / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_eq_process(eq_handle, sample_num, buffer1, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(buffer1);
    free(cmp_buffer);
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ deinterleave 32 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int in_read = 0;
    int sample_num = 0;
    int sample_rate = 44100;
    int bit = 16;
    int channel = 1;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    char in_name[100];
    char out_name[100];
    int ret = 0;
    void *eq_handle = NULL;
    // stream open
    sprintf(in_name, "/sdcard/pcm/test_44100.pcm");
    ret = infile_open(&infile, in_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    sprintf(out_name, "/sdcard/eq_test/32_c_inter/test_para_eq6.pcm");
    ret = outfile_open(&outfile, out_name);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    // config
    esp_ae_eq_cfg_t *eq_config = eq_config_para(sample_rate, channel, bit);
    TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
    ret = esp_ae_eq_open(eq_config, &eq_handle);
    TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
    void *b1_handle = NULL;
    esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
    esp_ae_bit_cvt_open(&b1_config, &b1_handle);
    TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
    void *b2_handle = NULL;
    esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
    esp_ae_bit_cvt_open(&b2_config, &b2_handle);
    TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

    short *buffer = calloc(sizeof(short), 1024 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);
    short *buffer1 = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
    short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    short *outbuf[10];
    for (int i = 0; i < channel; i++) {
        outbuf[i] = calloc(sizeof(short), 512 * 2 * 2);
        TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
    }

    while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
        sample_num = 1200 / (16 >> 3) / channel;
        esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
        esp_ae_deintlv_process(channel, 32, sample_num, buffer1, outbuf);
        esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
        esp_ae_intlv_process(channel, 32, sample_num, outbuf, buffer1);
        esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
        TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
        fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
    }
    esp_ae_eq_close(eq_handle);
    esp_ae_bit_cvt_close(b1_handle);
    esp_ae_bit_cvt_close(b2_handle);
    fclose(infile);
    fclose(outfile);
    free(buffer);
    free(cmp_buffer);
    free(buffer1);
    for (int i = 0; i < channel; i++) {
        if (outbuf[i] != NULL) {
            free(outbuf[i]);
        }
    }
    free(eq_config->para);
    free(eq_config);
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter interleave 16 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 16;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/16_c/test_para_eq1_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);

                short *buffer = calloc(sizeof(short), 1024);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
                    sample_num = 1200 / (16 >> 3) / channel;
                    esp_ae_eq_process(eq_handle, sample_num, buffer, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
                    fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter deinterleave 16 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 16;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/16_c_inter/test_para_eq2_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);

                short *buffer = calloc(sizeof(short), 1024);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                short *outbuf[10];
                for (int i = 0; i < channel; i++) {
                    outbuf[i] = calloc(sizeof(short), 512);
                    TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
                }

                while ((in_read = fread(buffer, 1, 1024, infile)) > 0) {
                    sample_num = 1024 / 2 / channel;
                    esp_ae_deintlv_process(channel, 16, sample_num, buffer, outbuf);
                    esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
                    esp_ae_intlv_process(channel, 16, sample_num, outbuf, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, in_read, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, in_read), 0);
#else
                    fwrite(buffer, 1, in_read, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                for (int i = 0; i < channel; i++) {
                    if (outbuf[i] != NULL) {
                        free(outbuf[i]);
                    }
                }
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter interleave 24 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 24;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/24_c/test_para_eq3_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                char *buffer = calloc(1, 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                char *buffer1 = calloc(1, 1024 * 2 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

                while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
                    sample_num = 1200 / (16 >> 3) / channel;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
                    esp_ae_eq_process(eq_handle, sample_num, buffer1, buffer1);
                    esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
                    fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                free(buffer1);
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter deinterleave 24 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 24;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/24_c_inter/test_para_eq4_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT24};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT24, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                short *buffer = calloc(sizeof(short), 1024 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                short *buffer1 = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
                short *outbuf[10];
                for (int i = 0; i < channel; i++) {
                    outbuf[i] = calloc(sizeof(short), 512 * 2 * 2);
                    TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
                }

                while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
                    sample_num = 1200 / (16 >> 3) / channel;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
                    esp_ae_deintlv_process(channel, ESP_AE_BIT24, sample_num, buffer1, outbuf);
                    esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
                    esp_ae_intlv_process(channel, ESP_AE_BIT24, sample_num, outbuf, buffer1);
                    esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
                    fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                free(buffer1);
                for (int i = 0; i < channel; i++) {
                    if (outbuf[i] != NULL) {
                        free(outbuf[i]);
                    }
                }
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter interleave 32 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 32;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/32_c/test_para_eq5_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                char *buffer = calloc(1, 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                char *buffer1 = calloc(1, 1024 * 2 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer1, NULL);

                while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
                    sample_num = 1200 / (16 >> 3) / channel;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
                    esp_ae_eq_process(eq_handle, sample_num, buffer1, buffer1);
                    esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
                    fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                free(buffer1);
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}

TEST_CASE("EQ single filter deinterleave 32 bit test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    FILE *infile = NULL;
    FILE *outfile = NULL;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                int in_read = 0;
                int sample_num = 0;
                int sample_rate = 8000;
                int channel = 2;
                int bit = 32;
                char in_name[100];
                char out_name[100];
                int ret = 0;
                void *eq_handle = NULL;
                // stream open
                sprintf(in_name, "/sdcard/pcm/test_8000_2.pcm");
                ret = infile_open(&infile, in_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                sprintf(out_name, "/sdcard/eq_test/32_c_inter/test_para_eq6_fil%d_gain%d_q%d.pcm", i, j, k);
                ret = outfile_open(&outfile, out_name);
                TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
                // config
                esp_ae_eq_cfg_t *eq_config = eq_config_single(sample_rate, channel, bit, 500, q[k], gain[j], ft[i]);
                TEST_ASSERT_NOT_EQUAL(eq_config, NULL);
                ret = esp_ae_eq_open(eq_config, &eq_handle);
                TEST_ASSERT_NOT_EQUAL(eq_handle, NULL);
                void *b1_handle = NULL;
                esp_ae_bit_cvt_cfg_t b1_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT16, .dest_bits = ESP_AE_BIT32};
                esp_ae_bit_cvt_open(&b1_config, &b1_handle);
                TEST_ASSERT_NOT_EQUAL(b1_handle, NULL);
                void *b2_handle = NULL;
                esp_ae_bit_cvt_cfg_t b2_config = {.sample_rate = sample_rate, .channel = channel, .src_bits = ESP_AE_BIT32, .dest_bits = ESP_AE_BIT16};
                esp_ae_bit_cvt_open(&b2_config, &b2_handle);
                TEST_ASSERT_NOT_EQUAL(b2_handle, NULL);

                short *buffer = calloc(sizeof(short), 1024 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer, NULL);
                short *cmp_buffer = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
                short *buffer1 = calloc(sizeof(short), 1024 * 2 * 2);
                TEST_ASSERT_NOT_EQUAL(buffer1, NULL);
                short *outbuf[10];
                for (int i = 0; i < channel; i++) {
                    outbuf[i] = calloc(sizeof(short), 512 * 2 * 2);
                    TEST_ASSERT_NOT_EQUAL(outbuf[i], NULL);
                }

                while ((in_read = fread(buffer, 1, 1200, infile)) > 0) {
                    sample_num = 1200 / (16 >> 3) / channel;
                    esp_ae_bit_cvt_process(b1_handle, sample_num, buffer, buffer1);
                    esp_ae_deintlv_process(channel, 32, sample_num, buffer1, outbuf);
                    esp_ae_eq_deintlv_process(eq_handle, sample_num, outbuf, outbuf);
                    esp_ae_intlv_process(channel, 32, sample_num, outbuf, buffer1);
                    esp_ae_bit_cvt_process(b2_handle, sample_num, buffer1, buffer);
#ifdef CMP_MODE
                    fread(cmp_buffer, 1, sample_num * (16 >> 3) * channel, outfile);
                    TEST_ASSERT_EQUAL(memcmp(buffer, cmp_buffer, sample_num * channel * (16 >> 3)), 0);
#else
                    fwrite(buffer, 1, sample_num * (16 >> 3) * channel, outfile);
#endif /* CMP_MODE */
                }
                esp_ae_eq_close(eq_handle);
                esp_ae_bit_cvt_close(b1_handle);
                esp_ae_bit_cvt_close(b2_handle);
                fclose(infile);
                fclose(outfile);
                free(buffer);
                free(buffer1);
                for (int i = 0; i < channel; i++) {
                    if (outbuf[i] != NULL) {
                        free(outbuf[i]);
                    }
                }
                free(eq_config->para);
                free(eq_config);
                free(cmp_buffer);
            }
        }
    }
    ae_sdcard_deinit();
}
