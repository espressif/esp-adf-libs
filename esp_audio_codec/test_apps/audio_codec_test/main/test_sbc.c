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
#include "esp_sbc_enc.h"
#include "esp_sbc_dec.h"
#include "esp_sbc_def.h"
#include "esp_audio_types.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_simple_dec.h"
#include "esp_board_manager.h"

#define TAG "TEST_SBC"
#define AUD_COMPARE

static char *cmp_buf;

static int sbc_encoder(char *infile_name, char *expectfile_name, esp_sbc_enc_config_t *enc_cfg, bool is_vbr)
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
    esp_sbc_enc_open(enc_cfg, sizeof(esp_sbc_enc_config_t), &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);

    int inbuf_sz = 0;
    int outbuf_sz = 0;
    esp_sbc_enc_get_frame_size(enc_hd, &inbuf_sz, &outbuf_sz);
    printf("insize:%d outsize:%d\n", inbuf_sz, outbuf_sz);
    uint8_t *inbuf = calloc(1, inbuf_sz);
    TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
    uint8_t *outbuf = calloc(1, outbuf_sz);
    TEST_ASSERT_NOT_EQUAL(outbuf, NULL);

    esp_audio_enc_in_frame_t in_frame = {0};
    esp_audio_enc_out_frame_t out_frame = {0};
    int inread = 0;
    int cnt = 0;
    int bitrate = 60000;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        if (is_vbr == true) {
            cnt++;
            if (cnt % 4 == 0) {
                bitrate -= 10;
                esp_sbc_enc_set_bitrate(enc_hd, bitrate);
            }
        }
        in_frame.buffer = inbuf;
        in_frame.len = inbuf_sz;
        out_frame.buffer = outbuf;
        out_frame.len = outbuf_sz;
        int ret = esp_sbc_enc_process(enc_hd, &in_frame, &out_frame);
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
    esp_sbc_enc_close(enc_hd);
    fclose(infile);
    fclose(expected_fp);
#ifdef AUD_COMPARE
    free(enc_cmp_buf);
    enc_cmp_buf = NULL;
#endif /* AUD_COMPARE */
    return 0;
}

static int sbc_decoder(char *infile_name, char *expectfile_name, esp_sbc_dec_cfg_t *dec_cfg, bool do_plc)
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
    esp_sbc_dec_open(dec_cfg, sizeof(esp_sbc_dec_cfg_t), &dec_hd);
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
        esp_audio_err_t dec_ret = esp_sbc_dec_decode(dec_hd, &in_frame, &out_frame, &info);
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
            if (inread <= 0 && in_frame.len == 0) {
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
    esp_sbc_dec_close(dec_hd);
    return 0;
}

static int sbc_calcu_bitpool(esp_sbc_enc_config_t *enc_cfg, int bitrate)
{
    int bitpool = 0;
    int bitnum_per_subband = bitrate * enc_cfg->sub_bands_num / enc_cfg->sample_rate;
    int channel = ((enc_cfg->ch_mode == ESP_SBC_CH_MODE_MONO) ? (1) : (2));
    if ((enc_cfg->ch_mode == ESP_SBC_CH_MODE_STEREO) || (enc_cfg->ch_mode == ESP_SBC_CH_MODE_JOINT_STEREO)) {
        bitpool = bitnum_per_subband - ((32 + (enc_cfg->sub_bands_num * channel << 2) + ((enc_cfg->ch_mode - 2) * enc_cfg->sub_bands_num)) / enc_cfg->block_length);
    } else {
        bitpool = bitnum_per_subband - ((32 + (enc_cfg->sub_bands_num << 2)) / enc_cfg->block_length);
    }
    return bitpool;
}

static void sbc_enc_task1(void *pvParamters)
{
    ESP_LOGI(TAG, "task1");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_sbc_enc_config_t enc_cfg = {
        .allocation_method = ESP_SBC_ALLOC_SNR,
        .bitpool = 42,
        .block_length = 8,
        .sub_bands_num = 4,
        .sample_rate = 48000,
        .bits_per_sample = 16,
        .ch_mode = ESP_SBC_CH_MODE_DUAL,
        .sbc_mode = ESP_SBC_MODE_STD,
    };
    sbc_encoder("/sdcard/sbc/thetest48_2.pcm", "/sdcard/sbc/thetest48_2_1.sbc", &enc_cfg, false);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void sbc_enc_task2(void *pvParamters)
{
    ESP_LOGI(TAG, "task2");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_sbc_enc_config_t enc_cfg = {
        .allocation_method = ESP_SBC_ALLOC_SNR,
        .bitpool = 50,
        .block_length = 16,
        .sub_bands_num = 8,
        .bits_per_sample = 16,
        .sample_rate = 48000,
        .ch_mode = ESP_SBC_CH_MODE_DUAL,
        .sbc_mode = ESP_SBC_MODE_STD,
    };
    sbc_encoder("/sdcard/sbc/thetest48_2.pcm", "/sdcard/sbc/thetest48_2_2.sbc", &enc_cfg, true);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void sbc_dec_task1(void *pvParamters)
{
    ESP_LOGI(TAG, "task1");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_sbc_dec_cfg_t dec_cfg = {
        .sbc_mode = ESP_SBC_MODE_STD,
        .ch_num = 2,
        .enable_plc = false,
    };
    sbc_decoder("/sdcard/sbc/thetest48_2_1.sbc", "/sdcard/sbc/thetest48_2_1.pcm", &dec_cfg, false);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

static void sbc_dec_task2(void *pvParamters)
{
    ESP_LOGI(TAG, "task2");
    QueueHandle_t xQueue = (QueueHandle_t)pvParamters;
    esp_sbc_dec_cfg_t dec_cfg = {
        .sbc_mode = ESP_SBC_MODE_STD,
        .ch_num = 2,
        .enable_plc = false,
    };
    sbc_decoder("/sdcard/sbc/thetest48_2_2.sbc", "/sdcard/sbc/thetest48_2_2.pcm", &dec_cfg, false);
    uint32_t done = 1;
    xQueueSend(xQueue, &done, 0);
    vTaskDelete(NULL);
}

TEST_CASE("SBC ENC test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    ESP_LOGI(TAG, "SBC_ENC std with mono and cbr");
    char in_name[100];
    char out_name[100];
    esp_sbc_enc_config_t enc_cfg = {
        .allocation_method = ESP_SBC_ALLOC_SNR,
        .bitpool = 36,
        .block_length = 8,
        .sub_bands_num = 4,
        .sample_rate = 16000,
        .bits_per_sample = 16,
        .ch_mode = ESP_SBC_CH_MODE_MONO,
        .sbc_mode = ESP_SBC_MODE_STD,
    };
    sprintf(in_name, "/sdcard/sbc/man2.pcm");
    sprintf(out_name, "/sdcard/sbc/test_16000_std.sbc");
    sbc_encoder(in_name, out_name, &enc_cfg, false);

    ESP_LOGI(TAG, "SBC_ENC msbc");
    enc_cfg.sbc_mode = ESP_SBC_MODE_MSBC;
    sprintf(out_name, "/sdcard/sbc/test_16000_msbc.sbc");
    sbc_encoder(in_name, out_name, &enc_cfg, false);

    ESP_LOGI(TAG, "SBC_ENC std with dual and cbr");
    enc_cfg.sbc_mode = ESP_SBC_MODE_STD;
    enc_cfg.allocation_method = ESP_SBC_ALLOC_LOUDNESS;
    enc_cfg.bitpool = 50;
    enc_cfg.block_length = 16;
    enc_cfg.sub_bands_num = 8;
    enc_cfg.bits_per_sample = 16;
    enc_cfg.ch_mode = ESP_SBC_CH_MODE_DUAL;
    sprintf(in_name, "/sdcard/sbc/thetest48_2.pcm");
    sprintf(out_name, "/sdcard/sbc/test_48_2_std.sbc");
    sbc_encoder(in_name, out_name, &enc_cfg, false);

    ESP_LOGI(TAG, "SBC_ENC std with dual and vbr");
    sprintf(out_name, "/sdcard/sbc/test_48_2_std_with_vbr.sbc");
    sbc_encoder(in_name, out_name, &enc_cfg, true);

    esp_board_manager_deinit();
}

TEST_CASE("SBC DEC test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    char in_name[100];
    char out_name[100];
    ESP_LOGI(TAG, "SBC_DEC std with mono cbr");
    esp_sbc_dec_cfg_t dec_cfg = {
        .ch_num = 1,
        .sbc_mode = ESP_SBC_MODE_STD,
    };
    sprintf(in_name, "/sdcard/sbc/test_16000_std.sbc");
    sprintf(out_name, "/sdcard/sbc/test_16000_std.pcm");
    sbc_decoder(in_name, out_name, &dec_cfg, false);

    ESP_LOGI(TAG, "SBC_DEC msbc with no plc");
    dec_cfg.sbc_mode = ESP_SBC_MODE_MSBC;
    sprintf(in_name, "/sdcard/sbc/test_16000_msbc.sbc");
    sprintf(out_name, "/sdcard/sbc/test_16000_msbc.pcm");
    sbc_decoder(in_name, out_name, &dec_cfg, false);

    ESP_LOGI(TAG, "SBC_DEC msbc with have plc");
    sprintf(in_name, "/sdcard/sbc/test_16000_msbc.sbc");
    sprintf(out_name, "/sdcard/sbc/test_16000_msbc_plc.pcm");
    sbc_decoder(in_name, out_name, &dec_cfg, false);

    ESP_LOGI(TAG, "SBC_DEC std with dual cbr");
    dec_cfg.sbc_mode = ESP_SBC_MODE_STD;
    dec_cfg.ch_num = 2,
    sprintf(in_name, "/sdcard/sbc/test_48_2_std.sbc");
    sprintf(out_name, "/sdcard/sbc/test_48_2_std.pcm");
    sbc_decoder(in_name, out_name, &dec_cfg, false);

    ESP_LOGI(TAG, "SBC_DEC std with dual vbr");
    sprintf(in_name, "/sdcard/sbc/test_48_2_std_with_vbr.sbc");
    sprintf(out_name, "/sdcard/sbc/test_48_2_std_with_vbr.pcm");
    sbc_decoder(in_name, out_name, &dec_cfg, false);

    esp_board_manager_deinit();
}

TEST_CASE("SBC ENC with multi-task test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    QueueHandle_t xQueue = NULL;
    xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreatePinnedToCore(sbc_enc_task1, "sbc_enc_task1", 4096, xQueue, 9, NULL, 0);
    xTaskCreatePinnedToCore(sbc_enc_task2, "sbc_enc_task2", 4096, xQueue, 10, NULL, 1);
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

TEST_CASE("SBC DEC with multi-task test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    QueueHandle_t xQueue = NULL;
    xQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreatePinnedToCore(sbc_dec_task1, "sbc_dec_task1", 4096, xQueue, 9, NULL, 0);
    xTaskCreatePinnedToCore(sbc_dec_task2, "sbc_dec_task2", 4096, xQueue, 10, NULL, 1);
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

TEST_CASE("SBC CODEC with esp_audio_codec interface test", "AUDIO_CODEC")
{
    esp_board_manager_init();
    esp_sbc_enc_register();
    esp_sbc_dec_register();
    FILE *infile = fopen("/sdcard/sbc/thetest48_2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/sbc/thetest48_2_loop_test.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/sbc/thetest48_2_loop_test.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_sbc_enc_config_t sbc_enc_cfg = {
        .allocation_method = ESP_SBC_ALLOC_SNR,
        .bitpool = 50,
        .block_length = 16,
        .sub_bands_num = 8,
        .sample_rate = 48000,
        .bits_per_sample = 16,
        .ch_mode = ESP_SBC_CH_MODE_DUAL,
        .sbc_mode = ESP_SBC_MODE_STD,
    };
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_SBC,
        .cfg = &sbc_enc_cfg,
        .cfg_sz = sizeof(esp_sbc_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_sbc_dec_cfg_t sbc_dec_cfg = {
        .sbc_mode = ESP_SBC_MODE_STD,
        .ch_num = 2,
        .enable_plc = false,
    };
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_SBC,
        .cfg = &sbc_dec_cfg,
        .cfg_sz = sizeof(esp_sbc_dec_cfg_t),
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
    int cnt = 0;
    int bitrate = 100000;
    while ((inread = fread(inbuf, 1, inbuf_sz, infile)) > 0) {
        /* code */
        if (inread < inbuf_sz) {
            break;
        }
        // Test for differnt bitrates
        if (cnt % 10 == 0) {
            bitrate -= 10;
            if (sbc_calcu_bitpool(&sbc_enc_cfg, bitrate) >= 2) {
                esp_audio_enc_set_bitrate(enc_hd, bitrate);
            }
        }
        // do encode
        enc_in_frame.buffer = inbuf;
        enc_in_frame.len = inbuf_sz;
        enc_out_frame.buffer = tmp;
        enc_out_frame.len = outbuf_sz;
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
        cnt++;
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
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_SBC);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_SBC);
    esp_board_manager_deinit();
}

TEST_CASE("SBC CODEC_PLC_WITH_AUDIO_CODEC_TEST", "AUDIO_CODEC")
{
    esp_board_manager_init();
    esp_sbc_enc_register();
    esp_sbc_dec_register();
    FILE *infile = fopen("/sdcard/sbc/man2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/sbc/man2_plc.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/sbc/man2_plc.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_sbc_enc_config_t sbc_enc_cfg = ESP_SBC_MSBC_ENC_CONFIG_DEFAULT();
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_SBC,
        .cfg = &sbc_enc_cfg,
        .cfg_sz = sizeof(esp_sbc_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_sbc_dec_cfg_t sbc_dec_cfg = {
        .sbc_mode = ESP_SBC_MODE_MSBC,
        .ch_num = 1,
        .enable_plc = true,
    };
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_SBC,
        .cfg = &sbc_dec_cfg,
        .cfg_sz = sizeof(esp_sbc_dec_cfg_t),
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
        int ret = esp_audio_enc_process(enc_hd, &enc_in_frame, &enc_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        // do decode
        dec_in_frame.buffer = tmp;
        dec_in_frame.len = enc_out_frame.encoded_bytes;
        dec_in_frame.consumed = 0;
        if (cnt % 5 == 0 || cnt % 5 == 1) {
            dec_in_frame.buffer = NULL;
            dec_in_frame.len = 0;
            dec_in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_PLC;
            memset(tmp, 0, enc_out_frame.encoded_bytes);
        } else {
            dec_in_frame.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
        }
        dec_out_frame.buffer = outbuf;
        dec_out_frame.len = inbuf_sz;
        ret = esp_audio_dec_process(dec_hd, &dec_in_frame, &dec_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        cnt++;
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
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_SBC);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_SBC);
    esp_board_manager_deinit();
}

TEST_CASE("SBC CODEC PLC_WITH_SIMPLE_DEC_TEST", "AUDIO_CODEC")
{
    esp_board_manager_init();
    esp_sbc_enc_register();
    esp_sbc_dec_register();
    FILE *infile = fopen("/sdcard/sbc/man2.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(infile, NULL);
#ifdef AUD_COMPARE
    FILE *expected_fp = fopen("/sdcard/sbc/man2_plc.pcm", "rb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
    cmp_buf = calloc(1, 4096);
    TEST_ASSERT_NOT_EQUAL(cmp_buf, NULL);
#else
    FILE *expected_fp = fopen("/sdcard/sbc/man2_plc.pcm", "wb");
    TEST_ASSERT_NOT_EQUAL(expected_fp, NULL);
#endif /* AUD_COMPARE */
    // create encoder handle
    esp_sbc_enc_config_t sbc_enc_cfg = ESP_SBC_MSBC_ENC_CONFIG_DEFAULT();
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_SBC,
        .cfg = &sbc_enc_cfg,
        .cfg_sz = sizeof(esp_sbc_enc_config_t),
    };
    esp_audio_enc_handle_t enc_hd = NULL;
    esp_audio_enc_open(&enc_cfg, &enc_hd);
    TEST_ASSERT_NOT_EQUAL(enc_hd, NULL);
    // create decoder handle
    esp_sbc_dec_cfg_t sbc_dec_cfg = {
        .sbc_mode = ESP_SBC_MODE_MSBC,
        .ch_num = 1,
        .enable_plc = true,
    };
    esp_audio_simple_dec_cfg_t dec_cfg = {
        .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_SBC,
        .dec_cfg = &sbc_dec_cfg,
        .cfg_size = sizeof(esp_sbc_dec_cfg_t),
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
        int ret = esp_audio_enc_process(enc_hd, &enc_in_frame, &enc_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        // do decode
        dec_in_frame.buffer = tmp;
        dec_in_frame.len = enc_out_frame.encoded_bytes;
        dec_in_frame.consumed = 0;
        if (cnt % 5 == 0 || cnt % 5 == 1) {
            dec_in_frame.buffer = NULL;
            dec_in_frame.len = 0;
            dec_in_frame.frame_recover = ESP_AUDIO_SIMPLE_DEC_RECOVERY_PLC;
            memset(tmp, 0, enc_out_frame.encoded_bytes);
        } else {
            dec_in_frame.frame_recover = ESP_AUDIO_SIMPLE_DEC_RECOVERY_NONE;
        }
        dec_out_frame.buffer = outbuf;
        dec_out_frame.len = inbuf_sz;
        ret = esp_audio_simple_dec_process(dec_hd, &dec_in_frame, &dec_out_frame);
        TEST_ASSERT_EQUAL(ret, ESP_OK);
        cnt++;
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
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_SBC);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_SBC);
    esp_board_manager_deinit();
}