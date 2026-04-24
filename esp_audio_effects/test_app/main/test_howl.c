/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "test_common.h"
#include "esp_ae_howl.h"
#include "ae_common.h"

#define TAG                     "TEST_HOWL"
#define HOWL_INPLACE_NUM_FRAMES 20
#define HOWL_INPLACE_CHANNELS   3

static void howl_default_cfg(esp_ae_howl_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->sample_rate = 16000;
    cfg->channel = 1;
    cfg->bits_per_sample = 16;
    cfg->papr_th = 10.0f;
    cfg->phpr_th = 45.0f;
    cfg->pnpr_th = 45.0f;
    cfg->imsd_th = 10.0f;
    cfg->enable_imsd = true;
}

static void fill_buf_interleaved(uint8_t *buf, uint32_t frame_size, int f, int bits)
{
    if (bits == 16) {
        uint32_t n = frame_size / sizeof(int16_t);
        int16_t *s16 = (int16_t *)buf;
        for (uint32_t i = 0; i < n; i++) {
            s16[i] = (int16_t)((f * 1000 + (int)i) & 0x7fff);
        }
    } else if (bits == 24) {
        uint32_t n = frame_size / 3;
        for (uint32_t i = 0; i < n; i++) {
            uint32_t v = (f * 1000 + (int)i) & 0x7FFFFF;
            buf[i * 3 + 0] = (uint8_t)(v & 0xFF);
            buf[i * 3 + 1] = (uint8_t)((v >> 8) & 0xFF);
            buf[i * 3 + 2] = (uint8_t)((v >> 16) & 0xFF);
        }
    } else if (bits == 32) {
        uint32_t n = frame_size / sizeof(int32_t);
        int32_t *s32 = (int32_t *)buf;
        for (uint32_t i = 0; i < n; i++) {
            s32[i] = (int32_t)((f * 1000 + (int)i) & 0x7FFFFFFF);
        }
    }
}

static void fill_buf_deint_ch(uint8_t *buf, uint32_t bytes_per_ch, int f, int ch, int bits)
{
    if (bits == 16) {
        uint32_t n = bytes_per_ch / sizeof(int16_t);
        int16_t *s16 = (int16_t *)buf;
        for (uint32_t i = 0; i < n; i++) {
            s16[i] = (int16_t)((f * 1000 + ch * 100 + (int)i) & 0x7fff);
        }
    } else if (bits == 24) {
        uint32_t n = bytes_per_ch / 3;
        for (uint32_t i = 0; i < n; i++) {
            uint32_t v = (f * 1000 + ch * 100 + (int)i) & 0x7FFFFF;
            buf[i * 3 + 0] = (uint8_t)(v & 0xFF);
            buf[i * 3 + 1] = (uint8_t)((v >> 8) & 0xFF);
            buf[i * 3 + 2] = (uint8_t)((v >> 16) & 0xFF);
        }
    } else if (bits == 32) {
        uint32_t n = bytes_per_ch / sizeof(int32_t);
        int32_t *s32 = (int32_t *)buf;
        for (uint32_t i = 0; i < n; i++) {
            s32[i] = (int32_t)((f * 1000 + ch * 100 + (int)i) & 0x7FFFFFFF);
        }
    }
}

static void split_interleaved_to_deint(const uint8_t *in_interleaved,
                                       void **out_deint,
                                       uint32_t samples_per_ch,
                                       uint8_t channel,
                                       uint32_t bytes_per_sample)
{
    for (uint32_t i = 0; i < samples_per_ch; i++) {
        for (uint32_t ch = 0; ch < channel; ch++) {
            const uint8_t *src = in_interleaved + ((i * channel + ch) * bytes_per_sample);
            uint8_t *dst = (uint8_t *)out_deint[ch] + i * bytes_per_sample;
            memcpy(dst, src, bytes_per_sample);
        }
    }
}

static void merge_deint_to_interleaved(void **in_deint,
                                       uint8_t *out_interleaved,
                                       uint32_t samples_per_ch,
                                       uint8_t channel,
                                       uint32_t bytes_per_sample)
{
    for (uint32_t i = 0; i < samples_per_ch; i++) {
        for (uint32_t ch = 0; ch < channel; ch++) {
            const uint8_t *src = (const uint8_t *)in_deint[ch] + i * bytes_per_sample;
            uint8_t *dst = out_interleaved + ((i * channel + ch) * bytes_per_sample);
            memcpy(dst, src, bytes_per_sample);
        }
    }
}

TEST_CASE("Howl branch test", "[howl][branch]")
{
    esp_ae_howl_cfg_t cfg;
    esp_ae_howl_handle_t handle = NULL;
    uint32_t frame_size = 0;
    uint32_t samples_per_ch;
    uint8_t buf[2048];
    uint8_t *in_buf = NULL, *out_buf = NULL;
    void *deint_in[2], *deint_out[2];
    esp_ae_err_t ret;
    howl_default_cfg(&cfg);
    /* esp_ae_howl_open: NULL parameters */
    ret = esp_ae_howl_open(NULL, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    ret = esp_ae_howl_open(&cfg, NULL);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);

    /* esp_ae_howl_open: invalid sample_rate */
    cfg.sample_rate = 12345;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid channel */
    cfg.channel = 0;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid bits_per_sample */
    cfg.bits_per_sample = 12;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid papr_th */
    cfg.papr_th = -11.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    cfg.papr_th = 21.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid phpr_th */
    cfg.phpr_th = -1.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    cfg.phpr_th = 101.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid pnpr_th */
    cfg.pnpr_th = -1.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    cfg.pnpr_th = 101.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);

    /* esp_ae_howl_open: invalid imsd_th when enable_imsd */
    cfg.enable_imsd = true;
    cfg.imsd_th = -0.1f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    cfg.imsd_th = 21.0f;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    howl_default_cfg(&cfg);
    cfg.enable_imsd = false;

    /* get_frame_size: NULL handle / NULL frame_size pointer (need valid handle first) */
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_howl_get_frame_size(NULL, &frame_size);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    ret = esp_ae_howl_get_frame_size(handle, NULL);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    ret = esp_ae_howl_get_frame_size(handle, &frame_size);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    /* reset: NULL handle */
    ret = esp_ae_howl_reset(NULL);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);

    /* process: NULL handle / NULL in_data / NULL out_data */
    ret = esp_ae_howl_process(NULL, buf, buf);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    in_buf = (uint8_t *)heap_caps_calloc(1, frame_size, MALLOC_CAP_INTERNAL);
    out_buf = (uint8_t *)heap_caps_calloc(1, frame_size, MALLOC_CAP_INTERNAL);
    TEST_ASSERT_NOT_NULL(in_buf);
    TEST_ASSERT_NOT_NULL(out_buf);
    ret = esp_ae_howl_process(handle, NULL, out_buf);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    ret = esp_ae_howl_process(handle, in_buf, NULL);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    free(in_buf);
    free(out_buf);

    /* deintlv_process: NULL channel pointer (2-ch handle) */
    esp_ae_howl_close(handle);
    handle = NULL;
    cfg.channel = 2;
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_howl_get_frame_size(handle, &frame_size);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    samples_per_ch = frame_size / (cfg.channel * (cfg.bits_per_sample / 8));
    deint_in[0] = heap_caps_calloc(1, samples_per_ch * (cfg.bits_per_sample / 8), MALLOC_CAP_INTERNAL);
    deint_in[1] = NULL;
    deint_out[0] = heap_caps_calloc(1, samples_per_ch * (cfg.bits_per_sample / 8), MALLOC_CAP_INTERNAL);
    deint_out[1] = heap_caps_calloc(1, samples_per_ch * (cfg.bits_per_sample / 8), MALLOC_CAP_INTERNAL);
    TEST_ASSERT_NOT_NULL(deint_in[0]);
    TEST_ASSERT_NOT_NULL(deint_out[0]);
    TEST_ASSERT_NOT_NULL(deint_out[1]);
    ret = esp_ae_howl_deintlv_process(handle, deint_in, deint_out);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_INVALID_PARAMETER, ret);
    free(deint_in[0]);
    free(deint_out[0]);
    free(deint_out[1]);

    /* close: NULL handle safe; use after close */
    esp_ae_howl_close(handle);
}

TEST_CASE("Howl process in-place 16/24/32bit", "[howl][process]")
{
    const int bits_list[] = {16, 24, 32};
    const int num_bits = sizeof(bits_list) / sizeof(bits_list[0]);
    const uint32_t sample_rates[] = {8000, 16000, 32000, 44100, 48000};
    const int num_sample_rates = sizeof(sample_rates) / sizeof(sample_rates[0]);
    const uint8_t channel_list[] = {1, HOWL_INPLACE_CHANNELS};
    const int num_channels = sizeof(channel_list) / sizeof(channel_list[0]);
    int b, s, c;
    for (b = 0; b < num_bits; b++) {
        int bits = bits_list[b];
        uint32_t bytes_per_sample = (bits == 16) ? 2U : (bits == 24) ? 3U : 4U;
        for (s = 0; s < num_sample_rates; s++) {
            uint32_t sr = sample_rates[s];
            uint32_t expect_block_len = (sr < 32000U) ? 512U : 1024U;
            for (c = 0; c < num_channels; c++) {
                uint8_t ch = channel_list[c];
                esp_ae_howl_cfg_t cfg1, cfg2;
                esp_ae_howl_handle_t h_inplace = NULL;
                esp_ae_howl_handle_t h_separate = NULL;
                uint32_t frame_size = 0;
                uint32_t expect_frame_size = expect_block_len * (uint32_t)ch * bytes_per_sample;
                uint8_t *buf_inplace = NULL;
                uint8_t *buf_in_sep = NULL;
                uint8_t *buf_out_sep = NULL;
                esp_ae_err_t ret;
                int f;
                howl_default_cfg(&cfg1);
                howl_default_cfg(&cfg2);
                cfg1.sample_rate = sr;
                cfg2.sample_rate = sr;
                cfg1.channel = ch;
                cfg2.channel = ch;
                cfg1.bits_per_sample = (uint8_t)bits;
                cfg2.bits_per_sample = (uint8_t)bits;
                ret = esp_ae_howl_open(&cfg1, &h_inplace);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_open(&cfg2, &h_separate);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_get_frame_size(h_inplace, &frame_size);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                TEST_ASSERT_EQUAL_UINT32(expect_frame_size, frame_size);
                buf_inplace = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                buf_in_sep = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                buf_out_sep = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                TEST_ASSERT_NOT_NULL(buf_inplace);
                TEST_ASSERT_NOT_NULL(buf_in_sep);
                TEST_ASSERT_NOT_NULL(buf_out_sep);
                for (f = 0; f < HOWL_INPLACE_NUM_FRAMES; f++) {
                    fill_buf_interleaved(buf_inplace, frame_size, f, bits);
                    fill_buf_interleaved(buf_in_sep, frame_size, f, bits);
                    ret = esp_ae_howl_process(h_inplace, buf_inplace, buf_inplace);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    ret = esp_ae_howl_process(h_separate, buf_in_sep, buf_out_sep);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    TEST_ASSERT_EQUAL_MEMORY(buf_inplace, buf_out_sep, frame_size);
                }
                free(buf_inplace);
                free(buf_in_sep);
                free(buf_out_sep);
                esp_ae_howl_close(h_inplace);
                esp_ae_howl_close(h_separate);
            }
        }
    }
}

TEST_CASE("Howl deintlv_process in-place 16/24/32bit", "[howl][deintlv]")
{
    const int bits_list[] = {16, 24, 32};
    const int num_bits = sizeof(bits_list) / sizeof(bits_list[0]);
    const uint32_t sample_rates[] = {8000, 16000, 32000, 44100, 48000};
    const int num_sample_rates = sizeof(sample_rates) / sizeof(sample_rates[0]);
    const uint8_t channel_list[] = {1, HOWL_INPLACE_CHANNELS};
    const int num_channels = sizeof(channel_list) / sizeof(channel_list[0]);
    int b, s, c;
    for (b = 0; b < num_bits; b++) {
        int bits = bits_list[b];
        uint32_t bytes_per_sample = (bits == 16) ? 2U : (bits == 24) ? 3U : 4U;
        for (s = 0; s < num_sample_rates; s++) {
            uint32_t sr = sample_rates[s];
            uint32_t expect_block_len = (sr < 32000U) ? 512U : 1024U;
            for (c = 0; c < num_channels; c++) {
                uint8_t channel = channel_list[c];
                esp_ae_howl_cfg_t cfg;
                esp_ae_howl_handle_t h_inplace = NULL;
                esp_ae_howl_handle_t h_separate = NULL;
                uint32_t frame_size = 0;
                uint32_t samples_per_ch;
                uint32_t bytes_per_ch;
                uint32_t expect_frame_size = expect_block_len * (uint32_t)channel * bytes_per_sample;
                void *inplace_buf[HOWL_INPLACE_CHANNELS];
                void *in_sep[HOWL_INPLACE_CHANNELS];
                void *out_sep[HOWL_INPLACE_CHANNELS];
                void *ref_deint[HOWL_INPLACE_CHANNELS];
                esp_ae_err_t ret;
                int f, ch;
                howl_default_cfg(&cfg);
                cfg.sample_rate = sr;
                cfg.channel = channel;
                cfg.bits_per_sample = (uint8_t)bits;
                ret = esp_ae_howl_open(&cfg, &h_inplace);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_open(&cfg, &h_separate);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_get_frame_size(h_inplace, &frame_size);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                TEST_ASSERT_EQUAL_UINT32(expect_frame_size, frame_size);
                samples_per_ch = frame_size / ((uint32_t)channel * bytes_per_sample);
                bytes_per_ch = samples_per_ch * bytes_per_sample;
                for (ch = 0; ch < channel; ch++) {
                    inplace_buf[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    in_sep[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    out_sep[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    ref_deint[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    TEST_ASSERT_NOT_NULL(inplace_buf[ch]);
                    TEST_ASSERT_NOT_NULL(in_sep[ch]);
                    TEST_ASSERT_NOT_NULL(out_sep[ch]);
                    TEST_ASSERT_NOT_NULL(ref_deint[ch]);
                }
                for (f = 0; f < HOWL_INPLACE_NUM_FRAMES; f++) {
                    for (ch = 0; ch < channel; ch++) {
                        fill_buf_deint_ch((uint8_t *)ref_deint[ch], bytes_per_ch, f, ch, bits);
                        memcpy(inplace_buf[ch], ref_deint[ch], bytes_per_ch);
                        memcpy(in_sep[ch], ref_deint[ch], bytes_per_ch);
                    }
                    ret = esp_ae_howl_deintlv_process(h_inplace, inplace_buf, inplace_buf);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    ret = esp_ae_howl_deintlv_process(h_separate, in_sep, out_sep);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    for (ch = 0; ch < channel; ch++) {
                        TEST_ASSERT_EQUAL_MEMORY(inplace_buf[ch], out_sep[ch], bytes_per_ch);
                    }
                }
                for (ch = 0; ch < channel; ch++) {
                    free(inplace_buf[ch]);
                    free(in_sep[ch]);
                    free(out_sep[ch]);
                    free(ref_deint[ch]);
                }
                esp_ae_howl_close(h_inplace);
                esp_ae_howl_close(h_separate);
            }
        }
    }
}

TEST_CASE("Howl interleave/deinterleave output consistency", "[howl][process][deintlv][consistency]")
{
    const int bits_list[] = {16, 24, 32};
    const int num_bits = sizeof(bits_list) / sizeof(bits_list[0]);
    const uint32_t sample_rates[] = {8000, 16000, 32000, 44100, 48000};
    const int num_sample_rates = sizeof(sample_rates) / sizeof(sample_rates[0]);
    const uint8_t channel_list[] = {1, HOWL_INPLACE_CHANNELS};
    const int num_channels = sizeof(channel_list) / sizeof(channel_list[0]);
    int b, s, c;

    for (b = 0; b < num_bits; b++) {
        int bits = bits_list[b];
        uint32_t bytes_per_sample = (bits == 16) ? 2U : (bits == 24) ? 3U : 4U;
        for (s = 0; s < num_sample_rates; s++) {
            uint32_t sr = sample_rates[s];
            for (c = 0; c < num_channels; c++) {
                ESP_LOGI(TAG, "Testing interleave/deinterleave output consistency: sr: %ld, ch: %d, bits: %d", sr, channel_list[c], bits);
                uint8_t channel = channel_list[c];
                esp_ae_howl_cfg_t cfg_interleaved, cfg_deintlv;
                esp_ae_howl_handle_t h_interleaved = NULL;
                esp_ae_howl_handle_t h_deintlv = NULL;
                uint32_t frame_size = 0;
                uint32_t samples_per_ch;
                uint32_t bytes_per_ch;
                uint8_t *in_interleaved = NULL;
                uint8_t *out_interleaved = NULL;
                uint8_t *deintlv_as_interleaved = NULL;
                void *in_deint[HOWL_INPLACE_CHANNELS] = {0};
                void *out_deint[HOWL_INPLACE_CHANNELS] = {0};
                esp_ae_err_t ret;
                int f, ch;

                howl_default_cfg(&cfg_interleaved);
                howl_default_cfg(&cfg_deintlv);
                cfg_interleaved.sample_rate = sr;
                cfg_deintlv.sample_rate = sr;
                cfg_interleaved.channel = channel;
                cfg_deintlv.channel = channel;
                cfg_interleaved.bits_per_sample = (uint8_t)bits;
                cfg_deintlv.bits_per_sample = (uint8_t)bits;

                ret = esp_ae_howl_open(&cfg_interleaved, &h_interleaved);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_open(&cfg_deintlv, &h_deintlv);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                ret = esp_ae_howl_get_frame_size(h_interleaved, &frame_size);
                TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                samples_per_ch = frame_size / ((uint32_t)channel * bytes_per_sample);
                bytes_per_ch = samples_per_ch * bytes_per_sample;
                in_interleaved = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                out_interleaved = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                deintlv_as_interleaved = (uint8_t *)heap_caps_aligned_calloc(16, 1, frame_size, MALLOC_CAP_INTERNAL);
                TEST_ASSERT_NOT_NULL(in_interleaved);
                TEST_ASSERT_NOT_NULL(out_interleaved);
                TEST_ASSERT_NOT_NULL(deintlv_as_interleaved);
                for (ch = 0; ch < channel; ch++) {
                    in_deint[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    out_deint[ch] = heap_caps_aligned_calloc(16, 1, bytes_per_ch, MALLOC_CAP_INTERNAL);
                    TEST_ASSERT_NOT_NULL(in_deint[ch]);
                    TEST_ASSERT_NOT_NULL(out_deint[ch]);
                }

                for (f = 0; f < HOWL_INPLACE_NUM_FRAMES; f++) {
                    fill_buf_interleaved(in_interleaved, frame_size, f, bits);
                    split_interleaved_to_deint(in_interleaved, in_deint, samples_per_ch, channel, bytes_per_sample);

                    ret = esp_ae_howl_process(h_interleaved, in_interleaved, out_interleaved);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
                    ret = esp_ae_howl_deintlv_process(h_deintlv, in_deint, out_deint);
                    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

                    merge_deint_to_interleaved(out_deint, deintlv_as_interleaved, samples_per_ch, channel, bytes_per_sample);
                    TEST_ASSERT_EQUAL_MEMORY(out_interleaved, deintlv_as_interleaved, frame_size);
                }

                free(in_interleaved);
                free(out_interleaved);
                free(deintlv_as_interleaved);
                for (ch = 0; ch < channel; ch++) {
                    free(in_deint[ch]);
                    free(out_deint[ch]);
                }
                esp_ae_howl_close(h_interleaved);
                esp_ae_howl_close(h_deintlv);
            }
        }
    }
}

TEST_CASE("Howl reset same input same output", "[howl][reset]")
{
    esp_ae_howl_cfg_t cfg;
    esp_ae_howl_handle_t handle = NULL;
    uint32_t frame_size = 0;
    uint8_t *in_buf = NULL;
    uint8_t *out_buf = NULL;
    uint8_t *ref_out = NULL;
    esp_ae_err_t ret;
    int f;
    const int num_frames = 10;

    howl_default_cfg(&cfg);
    ret = esp_ae_howl_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
    ret = esp_ae_howl_get_frame_size(handle, &frame_size);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    in_buf = (uint8_t *)heap_caps_calloc(1, frame_size, MALLOC_CAP_SPIRAM);
    out_buf = (uint8_t *)heap_caps_calloc(1, frame_size, MALLOC_CAP_SPIRAM);
    ref_out = (uint8_t *)heap_caps_calloc((size_t)num_frames, frame_size, MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_NULL(in_buf);
    TEST_ASSERT_NOT_NULL(out_buf);
    TEST_ASSERT_NOT_NULL(ref_out);

    /* First 10 frames: save each output */
    for (f = 0; f < num_frames; f++) {
        fill_buf_interleaved(in_buf, frame_size, f, 16);
        ret = esp_ae_howl_process(handle, in_buf, out_buf);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        memcpy(ref_out + (size_t)f * frame_size, out_buf, frame_size);
    }

    ret = esp_ae_howl_reset(handle);
    TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);

    /* Second 10 frames: same inputs, outputs must match first pass */
    for (f = 0; f < num_frames; f++) {
        fill_buf_interleaved(in_buf, frame_size, f, 16);
        ret = esp_ae_howl_process(handle, in_buf, out_buf);
        TEST_ASSERT_EQUAL(ESP_AE_ERR_OK, ret);
        TEST_ASSERT_EQUAL_MEMORY(out_buf, ref_out + (size_t)f * frame_size, frame_size);
    }

    free(in_buf);
    free(out_buf);
    free(ref_out);
    esp_ae_howl_close(handle);
}
