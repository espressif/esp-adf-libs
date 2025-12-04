/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "test_common.h"
#include "esp_lc3_enc.h"
#include "esp_lc3_dec.h"
#include "esp_audio_types.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_simple_dec.h"
#include "esp_board_manager.h"

#define TAG "TEST_LC3"
#define AUD_COMPARE

static char *cmp_buf;

static void init_dsp_state()
{
#ifdef CONFIG_IDF_TARGET_ESP32H4
    int32_t b = 0xc0000000;
    asm volatile("csrrwi x31, 0x7f3, 1");
    asm volatile("csrrw x31, 0x80a, %0" ::"r"(b));
#endif /* CONFIG_IDF_TARGET_ESP32H4 */
}

static esp_err_t lc3_bitrate_valid_check(esp_lc3_enc_config_t *cfg, int bitrate)
{
    if (cfg->frame_dms == 100) {
        if (((bitrate < 16000 || bitrate > 320000) && (cfg->sample_rate != 44100))
            || ((bitrate < 14700 || bitrate > 294000) && (cfg->sample_rate == 44100))) {
            return ESP_FAIL;
        }
    } else {
        if (((bitrate < 21334 || bitrate > 426667) && (cfg->sample_rate != 44100))
            || ((bitrate < 19600 || bitrate > 392000) && (cfg->sample_rate == 44100))) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static int lc3_encoder(char *infile_name, char *expectfile_name, uint8_t enable_vbr, esp_lc3_enc_config_t *enc_cfg)
{
    // stream open
    FILE *infile = fopen(infile_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen(expectfile_name, "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    char *enc_cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(enc_cmp_buf, NULL);
#else
    FILE *expected_fp = fopen(expectfile_name, "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    void *enc_hd = NULL;
    esp_lc3_enc_open(enc_cfg, sizeof(esp_lc3_enc_config_t), &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);

    int inbuf_sz = 0;
    int outbuf_sz = 0;
    esp_lc3_enc_get_frame_size(enc_hd, &inbuf_sz, &outbuf_sz);
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *outbuf = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    esp_audio_enc_in_frame_t in_frame = {0};
    esp_audio_enc_out_frame_t out_frame = {0};
    int inread = 0;
    int bitrate = 96000;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        in_frame.buffer = inbuf;
        in_frame.len = inbuf_sz;
        out_frame.buffer = outbuf;
        out_frame.len = outbuf_sz;
        // Test for differnt bitrates
        if (enable_vbr == true) {
            if (lc3_bitrate_valid_check(enc_cfg, bitrate) == ESP_OK) {
                esp_lc3_enc_set_bitrate(enc_hd, bitrate);
            }
            bitrate -= 50;
        }
        int ret = esp_lc3_enc_process(enc_hd, &in_frame, &out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
#ifdef AUD_COMPARE
        fread(enc_cmp_buf, 1, out_frame.encoded_bytes, expected_fp);
        TEST_ASSERT_EQUAL(memcmp(enc_cmp_buf, outbuf, out_frame.encoded_bytes), 0);
#else
        fwrite(outbuf, 1, out_frame.encoded_bytes, expected_fp);
#endif /* AUD_COMPARE */
    }
    free(inbuf);
    free(outbuf);
    esp_lc3_enc_close(enc_hd);
    fclose(infile);
    fclose(expected_fp);
#ifdef AUD_COMPARE
    free(enc_cmp_buf);
    enc_cmp_buf = NULL;
#endif /* AUD_COMPARE */
    return 0;
}

static int lc3_decoder(char *infile_name, char *expectfile_name, esp_lc3_dec_cfg_t *dec_cfg, bool do_plc)
{
    // stream open
    FILE *infile = fopen(infile_name, "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen(expectfile_name, "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    char *dec_cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(dec_cmp_buf, NULL);
#else
    FILE *expected_fp = fopen(expectfile_name, "w+");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */

    void *dec_hd = NULL;
    esp_lc3_dec_open(dec_cfg, sizeof(esp_lc3_enc_config_t), &dec_hd);
    TEST_ASSERT_NOT_EQUAL(dec_hd, NULL);

    int inbuf_sz = 1024;
    int outbuf_sz = 1024;
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);

    uint8_t *outbuf = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    esp_audio_dec_in_raw_t in_frame = {0};
    esp_audio_dec_out_frame_t out_frame = {0};
    esp_audio_dec_info_t info = {0};
    int inread = 0;
    inread = fread(inbuf, 1, inbuf_sz, infile);
    in_frame.buffer = inbuf;
    in_frame.len = inbuf_sz;
    in_frame.consumed = 0;
    in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
    out_frame.buffer = outbuf;
    out_frame.len = outbuf_sz;
    int cnt = 0;
    while (1) {
        if (do_plc == true) {
            if (cnt % 50 == 0) {
                in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_PLC;
            } else {
                in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
            }
        }
        memset(outbuf, 0, out_frame.len);
        esp_audio_err_t dec_ret = esp_lc3_dec_decode(dec_hd, &in_frame, &out_frame, &info);
        if (dec_ret == ESP_AUDIO_ERR_OK) {
            cnt++;
#ifdef AUD_COMPARE
            fread(dec_cmp_buf, 1, out_frame.decoded_size, expected_fp);
            TEST_ASSERT_EQUAL(memcmp(dec_cmp_buf, outbuf, out_frame.decoded_size), 0);
#else
            fwrite(outbuf, 1, out_frame.decoded_size, expected_fp);
#endif /* AUD_COMPARE */
            in_frame.len -= in_frame.consumed;
            in_frame.buffer += in_frame.consumed;
            in_frame.consumed = 0;
            out_frame.decoded_size = 0;
            if (in_frame.len == 0) {
                inread = fread(inbuf, 1, inbuf_sz, infile);
                if (inread <= 0) {
                    break;
                }
                in_frame.buffer = inbuf;
                in_frame.len = inread;
            }
        } else if (dec_ret == ESP_AUDIO_ERR_DATA_LACK) {
            memmove(inbuf, inbuf + (inbuf_sz - in_frame.len), in_frame.len);
            inread = fread(inbuf + in_frame.len, 1, inbuf_sz - in_frame.len, infile);
            if (inread <= 0) {
                break;
            }
            in_frame.buffer = inbuf;
            in_frame.len += inread;
            in_frame.consumed = 0;
        } else if (dec_ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
            outbuf = realloc(outbuf, out_frame.needed_size);
            out_frame.buffer = outbuf;
            out_frame.len = out_frame.needed_size;
            continue;
        } else {
            ESP_LOGE(TAG, "decode error:%d", dec_ret);
            break;
        }
    }
    fclose(infile);
    fclose(expected_fp);
    free(inbuf);
    free(outbuf);
#ifdef AUD_COMPARE
    free(dec_cmp_buf);
    dec_cmp_buf = NULL;
#endif /* AUD_COMPARE */
    esp_lc3_dec_close(dec_hd);
    return 0;
}

static void lc3_enc_task1(void *pvParamters)
{
    ESP_LOGI(TAG, "task1 cbr");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_lc3_enc_config_t enc_cfg = ESP_LC3_ENC_CONFIG_DEFAULT();
    enc_cfg.channel = 2;
    init_dsp_state();
    lc3_encoder("/sdcard/lc3/thetest48_2.pcm", "/sdcard/lc3/thetest48_2_1.lc3", false, &enc_cfg);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void lc3_enc_task2(void *pvParamters)
{
    ESP_LOGI(TAG, "task2 vbr");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_lc3_enc_config_t enc_cfg = ESP_LC3_ENC_CONFIG_DEFAULT();
    enc_cfg.len_prefixed = true;
    init_dsp_state();
    lc3_encoder("/sdcard/lc3/thetest48_1.pcm", "/sdcard/lc3/thetest48_1_2.lc3", true, &enc_cfg);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void lc3_dec_task1(void *pvParamters)
{
    ESP_LOGI(TAG, "task1");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_lc3_dec_cfg_t dec_cfg = ESP_LC3_DEC_CONFIG_DEFAULT();
    dec_cfg.channel = 2;
    init_dsp_state();
    lc3_decoder("/sdcard/lc3/thetest48_2_1.lc3", "/sdcard/lc3/lc3_thetest48_2_1.pcm", &dec_cfg, false);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void lc3_dec_task2(void *pvParamters)
{
    ESP_LOGI(TAG, "task2");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_lc3_dec_cfg_t dec_cfg = ESP_LC3_DEC_CONFIG_DEFAULT();
    dec_cfg.len_prefixed = true;
    dec_cfg.is_cbr = false;
    init_dsp_state();
    lc3_decoder("/sdcard/lc3/thetest48_1_2.lc3", "/sdcard/lc3/lc3_thetest48_1_2.pcm", &dec_cfg, false);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

TEST_CASE("LC3 ENC test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    init_dsp_state();
    char in_name[100];
    char out_name[100];
    esp_lc3_enc_config_t enc_cfg1 = ESP_LC3_ENC_CONFIG_DEFAULT();
    enc_cfg1.channel = 1;
    enc_cfg1.sample_rate = 16000;
    ESP_LOGI(TAG, "LC3_ENC with mono, no pre_append, cbr");
    enc_cfg1.len_prefixed = false;
    sprintf(in_name, "/sdcard/lc3/man2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_16000_np_cbr.lc3");
    lc3_encoder(in_name, out_name, false, &enc_cfg1);

    ESP_LOGI(TAG, "LC3_ENC with mono, have pre_append, cbr");
    enc_cfg1.len_prefixed = true;
    sprintf(in_name, "/sdcard/lc3/man2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_16000_hp_cbr.lc3");
    lc3_encoder(in_name, out_name, false, &enc_cfg1);

    ESP_LOGI(TAG, "LC3_ENC with mono, no pre_append, vbr");
    enc_cfg1.len_prefixed = false;
    sprintf(in_name, "/sdcard/lc3/man2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_16000_np_vbr.lc3");
    lc3_encoder(in_name, out_name, true, &enc_cfg1);

    ESP_LOGI(TAG, "LC3_ENC with mono, have pre_append, vbr");
    enc_cfg1.len_prefixed = true;
    sprintf(in_name, "/sdcard/lc3/man2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_16000_hp_vbr.lc3");
    lc3_encoder(in_name, out_name, true, &enc_cfg1);

    esp_lc3_enc_config_t enc_cfg2 = ESP_LC3_ENC_CONFIG_DEFAULT();
    enc_cfg2.channel = 2;
    enc_cfg2.sample_rate = 48000;
    ESP_LOGI(TAG, "LC3_ENC with dual, no pre_append, cbr");
    enc_cfg2.len_prefixed = false;
    sprintf(in_name, "/sdcard/lc3/thetest48_2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_48_np_cbr.lc3");
    lc3_encoder(in_name, out_name, false, &enc_cfg2);

    ESP_LOGI(TAG, "LC3_ENC with dual, have pre_append, cbr");
    enc_cfg2.len_prefixed = true;
    sprintf(in_name, "/sdcard/lc3/thetest48_2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_48_hp_cbr.lc3");
    lc3_encoder(in_name, out_name, false, &enc_cfg2);

    ESP_LOGI(TAG, "LC3_ENC with dual, no pre_append, vbr");
    enc_cfg2.len_prefixed = false;
    sprintf(in_name, "/sdcard/lc3/thetest48_2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_48_np_vbr.lc3");
    lc3_encoder(in_name, out_name, true, &enc_cfg2);

    ESP_LOGI(TAG, "LC3_ENC with dual, have pre_append, vbr");
    enc_cfg2.len_prefixed = true;
    sprintf(in_name, "/sdcard/lc3/thetest48_2.pcm");
    sprintf(out_name, "/sdcard/lc3/test_48_hp_vbr.lc3");
    lc3_encoder(in_name, out_name, true, &enc_cfg2);

    esp_board_manager_deinit();
}

TEST_CASE("LC3 DEC test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    init_dsp_state();
    char in_name[100];
    char out_name[100];
    esp_lc3_dec_cfg_t dec_cfg1 = ESP_LC3_DEC_CONFIG_DEFAULT();
    dec_cfg1.sample_rate = 16000;
    ESP_LOGI(TAG, "LC3_DEC with mono, no pre_append, cbr");
    dec_cfg1.len_prefixed = false;
    dec_cfg1.is_cbr = true;
    sprintf(in_name, "/sdcard/lc3/test_16000_np_cbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_16000_np_cbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg1, false);

    ESP_LOGI(TAG, "LC3_DEC with mono, have pre_append, cbr");
    dec_cfg1.len_prefixed = true;
    dec_cfg1.is_cbr = true;
    sprintf(in_name, "/sdcard/lc3/test_16000_hp_cbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_16000_hp_cbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg1, false);

    ESP_LOGI(TAG, "LC3_DEC with mono, have pre_append, vbr");
    dec_cfg1.len_prefixed = true;
    dec_cfg1.is_cbr = false;
    sprintf(in_name, "/sdcard/lc3/test_16000_hp_vbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_16000_hp_vbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg1, false);

    esp_lc3_dec_cfg_t dec_cfg2 = ESP_LC3_DEC_CONFIG_DEFAULT();
    dec_cfg2.channel = 2;
    ESP_LOGI(TAG, "LC3_DEC with dual, no pre_append, cbr");
    dec_cfg2.len_prefixed = false;
    dec_cfg2.is_cbr = true;
    sprintf(in_name, "/sdcard/lc3/test_48_np_cbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_48_np_cbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg2, false);

    ESP_LOGI(TAG, "LC3_DEC with dual, have pre_append, cbr");
    dec_cfg2.len_prefixed = true;
    dec_cfg2.is_cbr = true;
    sprintf(in_name, "/sdcard/lc3/test_48_hp_cbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_48_hp_cbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg2, false);

    ESP_LOGI(TAG, "LC3_DEC with dual, have pre_append, vbr");
    dec_cfg2.len_prefixed = true;
    dec_cfg2.is_cbr = false;
    sprintf(in_name, "/sdcard/lc3/test_48_hp_vbr.lc3");
    sprintf(out_name, "/sdcard/lc3/test_48_hp_vbr.pcm");
    lc3_decoder(in_name, out_name, &dec_cfg2, false);

    esp_board_manager_deinit();
}

TEST_CASE("LC3 ENC with multi-task test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    QueueHandle_t xQueue = NULL;
    xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreatePinnedToCore(lc3_enc_task1, "lc3_enc_task1", 4096, xQueue, 10, NULL, 0);
    xTaskCreatePinnedToCore(lc3_enc_task2, "lc3_enc_task2", 4096, xQueue, 10, NULL, 1);
    uint32_t done = 0;
    int cnt = 0;
    while (1) {
        xQueueReceive(xQueue, (void *)&done, portMAX_DELAY);
        cnt++;
        if (cnt == 2) {
            break;
        }
    }
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    vQueueDelete(xQueue);
    esp_board_manager_deinit();
}

TEST_CASE("LC3 DEC with multi-task test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    QueueHandle_t xQueue = NULL;
    xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreatePinnedToCore(lc3_dec_task1, "lc3_dec_task1", 4096, xQueue, 10, NULL, 0);
    xTaskCreatePinnedToCore(lc3_dec_task2, "lc3_dec_task2", 4096, xQueue, 10, NULL, 1);
    uint32_t done = 0;
    int cnt = 0;
    while (1) {
        xQueueReceive(xQueue, (void *)&done, portMAX_DELAY);
        cnt++;
        if (cnt == 2) {
            break;
        }
    }
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    vQueueDelete(xQueue);
    esp_board_manager_deinit();
}

TEST_CASE("LC3 CODEC with esp_audio_codec interface test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    init_dsp_state();
    esp_lc3_enc_register();
    esp_lc3_dec_register();
    FILE *infile = fopen("/sdcard/lc3/thetest48_1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_loop_test.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_loop_test.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_lc3_enc_config_t lc3_enc_cfg = ESP_LC3_ENC_CONFIG_DEFAULT();
    lc3_enc_cfg.channel = 1;
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_LC3,
        .cfg = &lc3_enc_cfg,
        .cfg_sz = sizeof(esp_lc3_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_lc3_dec_cfg_t lc3_dec_cfg = ESP_LC3_DEC_CONFIG_DEFAULT();
    lc3_dec_cfg.channel = 1;
    lc3_dec_cfg.is_cbr = false;
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_LC3,
        .cfg = &lc3_dec_cfg,
        .cfg_sz = sizeof(esp_lc3_dec_cfg_t),
    };
    esp_audio_dec_handle_t dec_hd = NULL;
    esp_audio_dec_open(&dec_cfg, &dec_hd);
    TEST_ASSERT_NOT_EQUAL(dec_hd, NULL);

    int inbuf_sz = 0;
    int outbuf_sz = 0;
    esp_audio_enc_get_frame_size(enc_hd, &inbuf_sz, &outbuf_sz);
    printf("insize:%d outsize:%d\n", inbuf_sz, outbuf_sz);
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *tmp = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(tmp, NULL);
    uint8_t *outbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    // loop test
    esp_audio_enc_in_frame_t enc_in_frame = {0};
    esp_audio_enc_out_frame_t enc_out_frame = {0};
    esp_audio_dec_in_raw_t dec_in_frame = {0};
    esp_audio_dec_out_frame_t dec_out_frame = {0};
    int inread = 0;
    int bitrate = 96000;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        // do encode
        enc_in_frame.buffer = inbuf;
        enc_in_frame.len = inbuf_sz;
        enc_out_frame.buffer = tmp;
        enc_out_frame.len = outbuf_sz;
        // Test for differnt bitrates
        if (lc3_bitrate_valid_check(&lc3_enc_cfg, bitrate) == ESP_OK) {
            esp_audio_enc_set_bitrate(enc_hd, bitrate);
        }
        bitrate -= 50;
        int ret = esp_audio_enc_process(enc_hd, &enc_in_frame, &enc_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        // do decode
        dec_in_frame.buffer = tmp;
        dec_in_frame.len = enc_out_frame.encoded_bytes;
        dec_in_frame.consumed = 0;
        dec_in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
        dec_out_frame.buffer = outbuf;
        dec_out_frame.len = inbuf_sz;
        ret = esp_audio_dec_process(dec_hd, &dec_in_frame, &dec_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
#ifdef AUD_COMPARE
        fread(cmp_buf, 1, dec_out_frame.decoded_size, expected_fp);
        TEST_ASSERT_EQUAL(memcmp(cmp_buf, outbuf, dec_out_frame.decoded_size), 0);
#else
        fwrite(outbuf, 1, dec_out_frame.decoded_size, expected_fp);
#endif /* AUD_COMPARE */
    }
    fclose(infile);
    fclose(expected_fp);
    free(inbuf);
    free(tmp);
    free(outbuf);
#ifdef AUD_COMPARE
    free(cmp_buf);
    cmp_buf = NULL;
#endif /* AUD_COMPARE */
    esp_audio_enc_close(enc_hd);
    esp_audio_dec_close(dec_hd);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_LC3);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_LC3);
    esp_board_manager_deinit();
}

TEST_CASE("LC3 CODEC PLC_WITH_AUDIO_CODEC_TEST", "AUDIO_CODEC")
{
    esp_board_manager_init();
    init_dsp_state();
    esp_lc3_enc_register();
    esp_lc3_dec_register();
    FILE *infile = fopen("/sdcard/lc3/thetest48_1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_plc_test.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_plc_test.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_lc3_enc_config_t lc3_enc_cfg = ESP_LC3_ENC_CONFIG_DEFAULT();
    lc3_enc_cfg.channel = 1;
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_LC3,
        .cfg = &lc3_enc_cfg,
        .cfg_sz = sizeof(esp_lc3_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_lc3_dec_cfg_t lc3_dec_cfg = ESP_LC3_DEC_CONFIG_DEFAULT();
    lc3_dec_cfg.channel = 1;
    lc3_dec_cfg.is_cbr = false;
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_LC3,
        .cfg = &lc3_dec_cfg,
        .cfg_sz = sizeof(esp_lc3_dec_cfg_t),
    };
    esp_audio_dec_handle_t dec_hd = NULL;
    esp_audio_dec_open(&dec_cfg, &dec_hd);
    TEST_ASSERT_NOT_EQUAL(dec_hd, NULL);

    int inbuf_sz = 0;
    int outbuf_sz = 0;
    esp_audio_enc_get_frame_size(enc_hd, &inbuf_sz, &outbuf_sz);
    printf("insize:%d outsize:%d\n", inbuf_sz, outbuf_sz);
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *tmp = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(tmp, NULL);
    uint8_t *outbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    // loop test
    esp_audio_enc_in_frame_t enc_in_frame = {0};
    esp_audio_enc_out_frame_t enc_out_frame = {0};
    esp_audio_dec_in_raw_t dec_in_frame = {0};
    esp_audio_dec_out_frame_t dec_out_frame = {0};
    int inread = 0;
    int bitrate = 96000;
    int cnt = 0;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        // do encode
        enc_in_frame.buffer = inbuf;
        enc_in_frame.len = inbuf_sz;
        enc_out_frame.buffer = tmp;
        enc_out_frame.len = outbuf_sz;
        // Test for differnt bitrates
        if (lc3_bitrate_valid_check(&lc3_enc_cfg, bitrate) == ESP_OK) {
            esp_audio_enc_set_bitrate(enc_hd, bitrate);
        }
        bitrate -= 50;
        int ret = esp_audio_enc_process(enc_hd, &enc_in_frame, &enc_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        // do decode
        dec_in_frame.buffer = tmp;
        dec_in_frame.len = enc_out_frame.encoded_bytes;
        dec_in_frame.consumed = 0;
        if (cnt % 50 == 0) {
            dec_in_frame.buffer = NULL;
            dec_in_frame.len = 0;
            dec_in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_PLC;
        } else {
            dec_in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
        }
        dec_out_frame.buffer = outbuf;
        dec_out_frame.len = inbuf_sz;
        cnt++;
        ret = esp_audio_dec_process(dec_hd, &dec_in_frame, &dec_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
#ifdef AUD_COMPARE
        fread(cmp_buf, 1, dec_out_frame.decoded_size, expected_fp);
        TEST_ASSERT_EQUAL(memcmp(cmp_buf, outbuf, dec_out_frame.decoded_size), 0);
#else
        fwrite(outbuf, 1, dec_out_frame.decoded_size, expected_fp);
#endif /* AUD_COMPARE */
    }
    fclose(infile);
    fclose(expected_fp);
    free(inbuf);
    free(tmp);
    free(outbuf);
#ifdef AUD_COMPARE
    free(cmp_buf);
    cmp_buf = NULL;
#endif /* AUD_COMPARE */
    esp_audio_enc_close(enc_hd);
    esp_audio_dec_close(dec_hd);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_LC3);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_LC3);
    esp_board_manager_deinit();
}

TEST_CASE("LC3 CODEC PLC_WITH_SIMPLE_DEC_TEST", "AUDIO_CODEC")
{
    esp_board_manager_init();
    init_dsp_state();
    esp_lc3_enc_register();
    esp_lc3_dec_register();
    FILE *infile = fopen("/sdcard/lc3/thetest48_1.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_plc_test.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/lc3/thetest48_1_plc_test.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_lc3_enc_config_t lc3_enc_cfg = ESP_LC3_ENC_CONFIG_DEFAULT();
    lc3_enc_cfg.channel = 1;
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_LC3,
        .cfg = &lc3_enc_cfg,
        .cfg_sz = sizeof(esp_lc3_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_lc3_dec_cfg_t lc3_dec_cfg = ESP_LC3_DEC_CONFIG_DEFAULT();
    lc3_dec_cfg.channel = 1;
    lc3_dec_cfg.is_cbr = false;
    esp_audio_simple_dec_cfg_t dec_cfg = {
        .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_LC3,
        .dec_cfg = &lc3_dec_cfg,
        .cfg_size = sizeof(esp_lc3_dec_cfg_t),
    };
    esp_audio_simple_dec_handle_t dec_hd = NULL;
    esp_audio_simple_dec_open(&dec_cfg, &dec_hd);
    TEST_ASSERT_NOT_EQUAL(dec_hd, NULL);

    int inbuf_sz = 0;
    int outbuf_sz = 0;
    esp_audio_enc_get_frame_size(enc_hd, &inbuf_sz, &outbuf_sz);
    printf("insize:%d outsize:%d\n", inbuf_sz, outbuf_sz);
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *tmp = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(tmp, NULL);
    uint8_t *outbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    // loop test
    esp_audio_enc_in_frame_t enc_in_frame = {0};
    esp_audio_enc_out_frame_t enc_out_frame = {0};
    esp_audio_simple_dec_raw_t dec_in_frame = {0};
    esp_audio_simple_dec_out_t dec_out_frame = {0};
    int inread = 0;
    int bitrate = 96000;
    int cnt = 0;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        // do encode
        enc_in_frame.buffer = inbuf;
        enc_in_frame.len = inbuf_sz;
        enc_out_frame.buffer = tmp;
        enc_out_frame.len = outbuf_sz;
        // Test for differnt bitrates
        if (lc3_bitrate_valid_check(&lc3_enc_cfg, bitrate) == ESP_OK) {
            esp_audio_enc_set_bitrate(enc_hd, bitrate);
        }
        bitrate -= 50;
        int ret = esp_audio_enc_process(enc_hd, &enc_in_frame, &enc_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        // do decode
        dec_in_frame.buffer = tmp;
        dec_in_frame.len = enc_out_frame.encoded_bytes;
        dec_in_frame.consumed = 0;
        if (cnt % 50 == 0) {
            dec_in_frame.buffer = NULL;
            dec_in_frame.len = 0;
            dec_in_frame.frame_recover = ESP_AUDIO_SIMPLE_DEC_RECOVERY_PLC;
        } else {
            dec_in_frame.frame_recover = ESP_AUDIO_SIMPLE_DEC_RECOVERY_NONE;
        }
        dec_out_frame.buffer = outbuf;
        dec_out_frame.len = inbuf_sz;
        cnt++;
        ret = esp_audio_simple_dec_process(dec_hd, &dec_in_frame, &dec_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
#ifdef AUD_COMPARE
        fread(cmp_buf, 1, dec_out_frame.decoded_size, expected_fp);
        TEST_ASSERT_EQUAL(memcmp(cmp_buf, outbuf, dec_out_frame.decoded_size), 0);
#else
        fwrite(outbuf, 1, dec_out_frame.decoded_size, expected_fp);
#endif /* AUD_COMPARE */
    }
    fclose(infile);
    fclose(expected_fp);
    free(inbuf);
    free(tmp);
    free(outbuf);
#ifdef AUD_COMPARE
    free(cmp_buf);
    cmp_buf = NULL;
#endif /* AUD_COMPARE */
    esp_audio_enc_close(enc_hd);
    esp_audio_simple_dec_close(dec_hd);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_LC3);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_LC3);
    esp_board_manager_deinit();
}