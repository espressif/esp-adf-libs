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
#include "esp_ae_mixer.h"

#define TAG "TEST_MIXER"
#define CMP_MODE

TEST_CASE("Mixer branch test", "AUDIO_EFFECT")
{
    esp_ae_mixer_info_t source_info[3] = {0};
    int channel = 2;
    esp_ae_mixer_info_t info1 = {
        .weight1 = 0.5,
        .weight2 = 1.0,
        .transit_time = 3000,
    };
    source_info[0] = info1;
    esp_ae_mixer_info_t info2 = {
        .weight1 = 0.0,
        .weight2 = 0.5,
        .transit_time = 6000,
    };
    source_info[1] = info2;
    esp_ae_mixer_cfg_t downmix_cfg;
    downmix_cfg.sample_rate = 44100;
    downmix_cfg.channel = channel;
    downmix_cfg.bits_per_sample = 16;
    downmix_cfg.src_info = source_info;
    downmix_cfg.src_num = 2;
    void *downmix_handle = NULL;
    int ret;
    ESP_LOGI(TAG, "esp_ae_mixer_open");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_open(NULL, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_open(&downmix_cfg, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    downmix_cfg.channel = 0;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    downmix_cfg.channel = channel;
    downmix_cfg.bits_per_sample = 8;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    downmix_cfg.bits_per_sample = 16;
    downmix_cfg.src_num = 0;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    downmix_cfg.src_num = 2;
    info2.transit_time = 0;
    source_info[1] = info2;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "create mixer handle");
    info2.transit_time = 6000;
    source_info[1] = info2;
    ret = esp_ae_mixer_open(&downmix_cfg, &downmix_handle);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
    char in_samples[2][100] = {0};
    char out_samples[100] = {0};
    char *in_samples_1[2][2] = {0};
    char *out_samples_1[2] = {0};
    int sample_num = 10;
    ESP_LOGI(TAG, "esp_ae_mixer_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_process(NULL, sample_num, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_process(downmix_handle, sample_num, NULL, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_process(downmix_handle, sample_num, in_samples, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mixer_process(downmix_handle, 0, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_mixer_process(downmix_handle, 10, in_samples, out_samples);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_ae_mixer_deintlv_process");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_deintlv_process(NULL, sample_num, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, sample_num, NULL, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, sample_num, in_samples_1, NULL);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test4");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 0, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test5");
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 10, in_samples_1, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test6");
    unsigned char **inbuf[3];
    void *in1[10] = {0};
    in1[0] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
    in1[1] = NULL;
    inbuf[0] = in1;
    inbuf[1] = NULL;
    ret = esp_ae_mixer_deintlv_process(downmix_handle, 10, inbuf, out_samples_1);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "esp_mixer_get_transit_time");
    ESP_LOGI(TAG, "test1");
    ret = esp_ae_mixer_set_mode(NULL, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test2");
    ret = esp_ae_mixer_set_mode(downmix_handle, 2, ESP_AE_MIXER_MODE_FADE_UPWARD);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    ESP_LOGI(TAG, "test3");
    ret = esp_ae_mixer_set_mode(downmix_handle, 0, ESP_AE_MIXER_MODE_MAX);
    TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_INVALID_PARAMETER);
    free(in1[0]);
    esp_ae_mixer_close(downmix_handle);
}

TEST_CASE("Mixer 16bit one mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 8000;
    int channel = 2;
    int bit = 16;
    esp_ae_mixer_info_t info = {
        .weight1 = 0.5,
        .weight2 = 1.0,
        .transit_time = 1000,
    };
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = &info;
    downmix_info.src_num = 1;
    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);

    FILE *infile = fopen("/sdcard/pcm/test_8000_2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc1.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc2.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    unsigned char *inbuf = heap_caps_aligned_calloc(16, sizeof(short),
                                                    1024, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(short),
                                                     1024, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(short),
                                                         1024, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf;
    void *inbuf_de[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de;
    int in_read1 = 0;
    int cnt = 0;
    // start in OFF stable(4)
    esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    while (1) {
        in_read1 = fread(inbuf, sizeof(short), 1024, infile);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = 2048 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf, inbuf_de);
        // start in on transit(1)
        if (cnt == 10) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 18) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on transit(1)
        if (cnt == 28) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 38) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 48) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 58) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off stable(4)
        if (cnt == 68) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    heap_caps_aligned_free(inbuf);
    heap_caps_aligned_free(outbuf);
    heap_caps_aligned_free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        heap_caps_aligned_free(inbuf_de[i]);
        heap_caps_aligned_free(outbuf_de[i]);
    }
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}

TEST_CASE("Mixer 16bit two mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 44100;
    int channel = 2;
    int bit = 16;
    esp_ae_mixer_info_t source_info[2] = {0};
    esp_ae_mixer_info_t info1 = {
        .weight1 = 1.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[0] = info1;
    esp_ae_mixer_info_t info2 = {
        .weight1 = 0.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[1] = info2;
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = source_info;
    downmix_info.src_num = 2;

    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);
    FILE *infile1 = fopen("/sdcard/pcm/mix_test1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile1, NULL);
    FILE *infile2 = fopen("/sdcard/pcm/mix_test2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile2, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix1.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix2.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    // inter
    unsigned char *inbuf1 = calloc(sizeof(short), 1024);
    unsigned char *inbuf2 = calloc(sizeof(short), 1024);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(short), 1024,
                                                         MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf1;
    in[1] = inbuf2;
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(short), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    // deinter
    void *inbuf_de1[10] = {0};
    void *inbuf_de2[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de1[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(inbuf_de1[i], NULL);
        inbuf_de2[i] = calloc(sizeof(short), 1024);
        TEST_ASSERT_NOT_EQUAL(inbuf_de2[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(short), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de1;
    in_de[1] = inbuf_de2;
    int in_read1 = 0;
    int in_read2 = 0;
    int cnt = 0;
    while (1) {
        in_read1 = fread(inbuf1, sizeof(short), 1024, infile1);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = 2048 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf1, inbuf_de1);
        in_read2 = fread(inbuf2, sizeof(short), 1024, infile2);
        if (in_read2 <= 0) {
            break;
        }
        sample_num = 2048 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf2, inbuf_de2);
        // start in on transit(1)
        if (cnt == 1000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in off transit(2)
        if (cnt == 1800) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in on transit(1)
        if (cnt == 6000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 15000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }

        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    free(inbuf1);
    free(inbuf2);
    free(outbuf);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(inbuf_de1[i]);
        free(inbuf_de2[i]);
        free(outbuf_de[i]);
    }
    fclose(infile1);
    fclose(infile2);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}

TEST_CASE("Mixer 24bit one mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 8000;
    int channel = 2;
    int bit = 24;
    esp_ae_mixer_info_t info = {
        .weight1 = 0.5,
        .weight2 = 1.0,
        .transit_time = 1000,
    };
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = &info;
    downmix_info.src_num = 1;
    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);
    FILE *infile = fopen("/sdcard/pcm/test_8000_2_24.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc3.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc4.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc3.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc4.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    unsigned char *inbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                    MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                         MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf;
    void *inbuf_de[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                               MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de;
    int in_read1 = 0;
    int cnt = 0;
    // start in OFF stable(4)
    esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    while (1) {
        in_read1 = fread(inbuf, 3, 1024, infile);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = 1024 / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf, inbuf_de);
        // start in on transit(1)
        if (cnt == 10) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 18) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on transit(1)
        if (cnt == 28) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 38) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 48) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 58) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off stable(4)
        if (cnt == 68) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    free(inbuf);
    free(outbuf);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(inbuf_de[i]);
        free(outbuf_de[i]);
    }
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}

TEST_CASE("Mixer 24bit two mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 44100;
    int channel = 2;
    int bit = 24;
    esp_ae_mixer_info_t source_info[2] = {0};
    esp_ae_mixer_info_t info1 = {
        .weight1 = 1.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[0] = info1;
    esp_ae_mixer_info_t info2 = {
        .weight1 = 0.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[1] = info2;
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = source_info;
    downmix_info.src_num = 2;

    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);
    FILE *infile1 = fopen("/sdcard/pcm/mix_test1_24.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile1, NULL);
    FILE *infile2 = fopen("/sdcard/pcm/mix_test2_24.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile2, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix3.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix4.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix3.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix4.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    // inter
    unsigned char *inbuf1 = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf1, NULL);
    unsigned char *inbuf2 = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf2, NULL);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                         MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf1;
    in[1] = inbuf2;
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    // deinter
    void *inbuf_de1[10] = {0};
    void *inbuf_de2[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de1[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de1[i], NULL);
        inbuf_de2[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de2[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de1;
    in_de[1] = inbuf_de2;

    int in_read1 = 0;
    int in_read2 = 0;
    int cnt = 0;

    while (1) {
        in_read1 = fread(inbuf1, 1, 2400, infile1);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = in_read1 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf1, inbuf_de1);
        in_read2 = fread(inbuf2, 1, 2400, infile2);
        if (in_read2 <= 0) {
            break;
        }
        sample_num = in_read2 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf2, inbuf_de2);
        // start in on transit(1)
        if (cnt == 1000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in off transit(2)
        if (cnt == 1800) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in on transit(1)
        if (cnt == 6000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 15000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    free(inbuf1);
    free(inbuf2);
    free(outbuf);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(inbuf_de1[i]);
        free(inbuf_de2[i]);
        free(outbuf_de[i]);
    }
    fclose(infile1);
    fclose(infile2);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}

TEST_CASE("Mixer 32bit one mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 8000;
    int channel = 2;
    int bit = 32;
    esp_ae_mixer_info_t info = {
        .weight1 = 0.5,
        .weight2 = 1.0,
        .transit_time = 1000,
    };
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = &info;
    downmix_info.src_num = 1;
    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);
    FILE *infile = fopen("/sdcard/pcm/test_8000_2_32.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc5.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc6.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmixc5.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmixc6.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    unsigned char *inbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                    MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                         MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf;
    void *inbuf_de[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                               MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de;
    int in_read1 = 0;
    int cnt = 0;
    // start in OFF stable(4)
    esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    while (1) {
        in_read1 = fread(inbuf, sizeof(int), 1024, infile);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = 1024 / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf, inbuf_de);
        // start in on transit(1)
        if (cnt == 10) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 18) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on transit(1)
        if (cnt == 28) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 38) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 48) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off transit(2)
        if (cnt == 58) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in off stable(4)
        if (cnt == 68) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    free(inbuf);
    free(outbuf);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(inbuf_de[i]);
        free(outbuf_de[i]);
    }
    fclose(infile);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}

TEST_CASE("Mixer 32bit two mix test", "AUDIO_EFFECT")
{
    ae_sdcard_init();
    int sample_rate = 44100;
    int channel = 2;
    int bit = 32;
    esp_ae_mixer_info_t source_info[2] = {0};
    esp_ae_mixer_info_t info1 = {
        .weight1 = 1.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[0] = info1;
    esp_ae_mixer_info_t info2 = {
        .weight1 = 0.0,
        .weight2 = 0.5,
        .transit_time = 1000,
    };
    source_info[1] = info2;
    esp_ae_mixer_cfg_t downmix_info;
    downmix_info.sample_rate = sample_rate,
    downmix_info.channel = channel,
    downmix_info.bits_per_sample = bit,
    downmix_info.src_info = source_info;
    downmix_info.src_num = 2;

    void *downmix_handle1 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle1);
    TEST_ASSERT_NOT_EQUAL(downmix_handle1, NULL);
    void *downmix_handle2 = NULL;
    esp_ae_mixer_open(&downmix_info, &downmix_handle2);
    TEST_ASSERT_NOT_EQUAL(downmix_handle2, NULL);
    FILE *infile1 = fopen("/sdcard/pcm/mix_test1_32.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile1, NULL);
    FILE *infile2 = fopen("/sdcard/pcm/mix_test2_32.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile2, NULL);
#ifdef CMP_MODE
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix5.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix6.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#else
    FILE *outfile1 = fopen("/sdcard/mixer/test_downmix5.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile1, NULL);
    FILE *outfile2 = fopen("/sdcard/mixer/test_downmix6.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(outfile2, NULL);
#endif /* CMP_MODE */
    // inter
    unsigned char *inbuf1 = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf1, NULL);
    unsigned char *inbuf2 = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(inbuf2, NULL);
    unsigned char *cmp_buffer = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                         MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(cmp_buffer, NULL);
    unsigned char *in[3];
    in[0] = inbuf1;
    in[1] = inbuf2;
    unsigned char *outbuf = heap_caps_aligned_calloc(16, sizeof(int), 1024,
                                                     MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
    // deinter
    void *inbuf_de1[10] = {0};
    void *inbuf_de2[10] = {0};
    void *outbuf_de[10] = {0};
    for (int i = 0; i < channel; i++) {
        inbuf_de1[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de1[i], NULL);
        inbuf_de2[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(inbuf_de2[i], NULL);
        outbuf_de[i] = heap_caps_aligned_calloc(16, sizeof(int), 1024, MALLOC_CAP_SPIRAM);
        TEST_ASSERT_NOT_EQUAL(outbuf_de[i], NULL);
    }
    unsigned char *in_de[3];
    in_de[0] = inbuf_de1;
    in_de[1] = inbuf_de2;
    int in_read1 = 0;
    int in_read2 = 0;
    int cnt = 0;
    while (1) {
        in_read1 = fread(inbuf1, sizeof(int), 1024, infile1);
        if (in_read1 <= 0) {
            break;
        }
        int sample_num = 4096 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf1, inbuf_de1);
        in_read2 = fread(inbuf2, sizeof(int), 1024, infile2);
        if (in_read2 <= 0) {
            break;
        }
        sample_num = 4096 / (bit >> 3) / channel;
        esp_ae_deintlv_process(channel, bit, sample_num, inbuf2, inbuf_de2);
        // start in on transit(1)
        if (cnt == 1000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in off transit(2)
        if (cnt == 1800) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        // start in on transit(1)
        if (cnt == 6000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
        }
        // start in on stable(3)
        if (cnt == 15000) {
            esp_ae_mixer_set_mode(downmix_handle1, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 0, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle1, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
            esp_ae_mixer_set_mode(downmix_handle2, 1, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
        }
        esp_ae_mixer_process(downmix_handle1, sample_num, (void **)in, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile1);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile1);
#endif /* CMP_MODE */
        esp_ae_mixer_deintlv_process(downmix_handle2, sample_num, (void ***)in_de, outbuf_de);
        esp_ae_intlv_process(channel, bit, sample_num, outbuf_de, outbuf);
#ifdef CMP_MODE
        fread(cmp_buffer, 1, sample_num * (bit >> 3) * channel, outfile2);
        TEST_ASSERT_EQUAL(memcmp(outbuf, cmp_buffer, sample_num * (bit >> 3) * channel), 0);
#else
        fwrite(outbuf, 1, sample_num * (bit >> 3) * channel, outfile2);
#endif /* CMP_MODE */
        cnt++;
    }
    esp_ae_mixer_close(downmix_handle1);
    esp_ae_mixer_close(downmix_handle2);
    free(inbuf1);
    free(inbuf2);
    free(outbuf);
    free(cmp_buffer);
    for (int i = 0; i < channel; i++) {
        free(inbuf_de1[i]);
        free(inbuf_de2[i]);
        free(outbuf_de[i]);
    }
    fclose(infile1);
    fclose(infile2);
    fclose(outfile1);
    fclose(outfile2);
    ae_sdcard_deinit();
}
