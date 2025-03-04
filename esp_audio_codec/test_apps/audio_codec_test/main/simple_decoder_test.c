/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "audio_codec_test.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_simple_dec.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "test_common.h"

#define TAG "SIMP_DEC_TEST"
#define AUD_COMPARE
typedef union {
    esp_m4a_dec_cfg_t m4a_cfg;
    esp_ts_dec_cfg_t  ts_cfg;
    esp_aac_dec_cfg_t aac_cfg;
} simp_dec_all_t;

typedef struct {
    uint8_t *data;
    int      read_size;
    int      size;
} read_ctx_t;

typedef struct {
    uint8_t *data;
    int      write_size;
    int      read_size;
    int      size;
    int      decode_size;
} write_ctx_t;

static write_ctx_t write_ctx;
static read_ctx_t  read_ctx;
static FILE       *simp_dec_fp;
static FILE       *simp_dec_wr_fp;
static char       *cmp_buf;

static esp_audio_simple_dec_type_t get_simple_decoder_type(char *file)
{
    char *ext = strrchr(file, '.');
    if (ext == NULL) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_NONE;
    }
    ext++;
    if (strcasecmp(ext, "aac") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AAC;
    }
    if (strcasecmp(ext, "mp3") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_MP3;
    }
    if (strcasecmp(ext, "amrnb") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB;
    }
    if (strcasecmp(ext, "amrwb") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB;
    }
    if (strcasecmp(ext, "flac") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC;
    }
    if (strcasecmp(ext, "wav") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_WAV;
    }
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "m4a") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_M4A;
    }
    if (strcasecmp(ext, "ts") == 0) {
        return ESP_AUDIO_SIMPLE_DEC_TYPE_TS;
    }
    return ESP_AUDIO_SIMPLE_DEC_TYPE_NONE;
}

static void get_simple_decoder_config(esp_audio_simple_dec_cfg_t *cfg)
{
    simp_dec_all_t *all_cfg = (simp_dec_all_t *)cfg->dec_cfg;
    switch (cfg->dec_type) {
        case ESP_AUDIO_SIMPLE_DEC_TYPE_AAC: {
            esp_aac_dec_cfg_t *aac_cfg = &all_cfg->aac_cfg;
            aac_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_aac_dec_cfg_t);
            break;
        }
        case ESP_AUDIO_SIMPLE_DEC_TYPE_M4A: {
            esp_m4a_dec_cfg_t *m4a_cfg = &all_cfg->m4a_cfg;
            m4a_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_m4a_dec_cfg_t);
            break;
        }
        case ESP_AUDIO_SIMPLE_DEC_TYPE_TS: {
            esp_ts_dec_cfg_t *ts_cfg = &all_cfg->ts_cfg;
            ts_cfg->aac_plus_enable = true;
            cfg->cfg_size = sizeof(esp_ts_dec_cfg_t);
            break;
        }
        default:
            break;
    }
}

static int read_raw_from_file(uint8_t *data, int size)
{
    return fread(data, 1, size, simp_dec_fp);
}

static int write_pcm_to_file(uint8_t *data, int size)
{
#ifdef AUD_COMPARE
    int a = size / 4096;
    int b = size % 4096;
    uint8_t *data_tmp = data;
    int ret = 0;
    for (int i = 0; i < a; i++) {
        ret = fread(cmp_buf, 1, 4096, simp_dec_wr_fp);
        if (ret <= 0) {
            return ESP_OK;
        }
        if (memcmp(cmp_buf, data_tmp, 4096)) {
            ESP_LOGE(TAG, "PCM data not match!(%d)", __LINE__);
            return ESP_FAIL;
        }
        data_tmp += 4096;
    }
    fread(cmp_buf, 1, b, simp_dec_wr_fp);
    if (ret <= 0) {
        return ESP_OK;
    }
    if (memcmp(cmp_buf, data_tmp, b)) {
        ESP_LOGE(TAG, "PCM data not match!(%d %d %d)", __LINE__, size, b);
        return ESP_FAIL;
    }
    return ESP_OK;
#else
    return fwrite(data, 1, size, simp_dec_wr_fp);
#endif
}

static int encoder_read_pcm(uint8_t *data, int size)
{
    if (read_ctx.read_size + size <= read_ctx.size) {
        memcpy(data, read_ctx.data + read_ctx.read_size, size);
        read_ctx.read_size += size;
        return size;
    }
    return 0;
}

static int encoder_write_frame(uint8_t *data, int size)
{
    if (write_ctx.write_size + size < write_ctx.size) {
        memcpy(write_ctx.data + write_ctx.write_size, data, size);
        write_ctx.write_size += size;
        return size;
    }
    return 0;
}

static int get_encode_data(audio_info_t *aud_info)
{
    read_ctx.size = audio_codec_gen_pcm(aud_info, read_ctx.data, read_ctx.size);
    read_ctx.read_size = 0;

    // Encode AAC, encoded data will be sent out through callback `decode_one_frame`
    audio_codec_test_cfg_t enc_cfg = {
        .read = encoder_read_pcm,
        .write = encoder_write_frame,
    };
    audio_encoder_test(ESP_AUDIO_TYPE_AAC, &enc_cfg, aud_info);
    return 0;
}

static int simple_decoder_read_data(uint8_t *data, int size)
{
    int left = write_ctx.write_size - write_ctx.read_size;
    if (left > size) {
        left = size;
    }
    if (left) {
        memcpy(data, write_ctx.data + write_ctx.read_size, left);
        write_ctx.read_size += left;
    }
    return left;
}

static int simple_decoder_write_pcm(uint8_t *data, int size)
{
    write_ctx.decode_size += size;
    return size;
}

int audio_simple_decoder_test(esp_audio_simple_dec_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info)
{
    esp_audio_dec_register_default();
    int ret = esp_audio_simple_dec_register_default();
    int max_out_size = 4096;
    int read_size = 512;
    uint8_t *in_buf = malloc(read_size);
    uint8_t *out_buf = malloc(max_out_size);
    esp_audio_simple_dec_handle_t decoder = NULL;
    do {
        if (in_buf == NULL || out_buf == NULL) {
            ESP_LOGI(TAG, "No memory for decoder");
            ret = ESP_AUDIO_ERR_MEM_LACK;
            break;
        }
        simp_dec_all_t all_cfg = {};
        esp_audio_simple_dec_cfg_t dec_cfg = {
            .dec_type = type,
            .dec_cfg = &all_cfg,
        };
        get_simple_decoder_config(&dec_cfg);
        ret = esp_audio_simple_dec_open(&dec_cfg, &decoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGI(TAG, "Fail to open simple decoder ret %d", ret);
            break;
        }
        int total_decoded = 0;
        uint64_t decode_time = 0;
        esp_audio_simple_dec_raw_t raw = {
            .buffer = in_buf,
        };
        uint64_t read_start = esp_timer_get_time();
        while (ret == ESP_AUDIO_ERR_OK) {
            ret = cfg->read(in_buf, read_size);
            if (ret <= 0) {
                break;
            }
            raw.buffer = in_buf;
            raw.len = ret;
            raw.eos = (ret < read_size);
            esp_audio_simple_dec_out_t out_frame = {
                .buffer = out_buf,
                .len = max_out_size,
            };
            // ATTENTION: when input raw data unconsumed (`raw.len > 0`) do not overwrite its content
            // Or-else unexpected error may happen for data corrupt.
            while (raw.len) {
                uint64_t start = esp_timer_get_time();
                if (start > read_start + 30000000) {
                    raw.eos = true;
                    break;
                }
                ret = esp_audio_simple_dec_process(decoder, &raw, &out_frame);
                decode_time += esp_timer_get_time() - start;
                if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                    // Handle output buffer not enough case
                    uint8_t *new_buf = realloc(out_buf, out_frame.needed_size);
                    if (new_buf == NULL) {
                        break;
                    }
                    out_buf = new_buf;
                    out_frame.buffer = new_buf;
                    max_out_size = out_frame.needed_size;
                    out_frame.len = max_out_size;
                    continue;
                }
                if (ret != ESP_AUDIO_ERR_OK) {
                    ESP_LOGE(TAG, "Fail to decode data ret %d", ret);
                    break;
                }
                if (out_frame.decoded_size) {
                    if (total_decoded == 0) {
                        // Update audio information
                        esp_audio_simple_dec_info_t dec_info = {};
                        esp_audio_simple_dec_get_info(decoder, &dec_info);
                        info->sample_rate = dec_info.sample_rate;
                        info->bits_per_sample = dec_info.bits_per_sample;
                        info->channel = dec_info.channel;
                        ESP_LOGI(TAG, "Audio info: sample_rate %d bits_per_sample %d channel %d", info->sample_rate,
                                 info->bits_per_sample, info->channel);
                    }
                    total_decoded += out_frame.decoded_size;
                    if (cfg->write) {
                        cfg->write(out_frame.buffer, out_frame.decoded_size);
                    }
                }
                // In case that input data contain multiple frames
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
            if (raw.eos) {
                break;
            }
        }
        if (total_decoded) {
            int sample_size = info->channel * info->bits_per_sample >> 3;
            float cpu_usage = (float)decode_time * sample_size * info->sample_rate / total_decoded / 10000;
            ESP_LOGI(TAG, "Decode for %d total_decoded:%d cpu: %.2f%%", type, total_decoded, cpu_usage);
        }
    } while (0);
    esp_audio_simple_dec_close(decoder);
    esp_audio_simple_dec_unregister_default();
    esp_audio_dec_unregister_default();
    if (in_buf) {
        free(in_buf);
    }
    if (out_buf) {
        free(out_buf);
    }
    return ret;
}

int audio_simple_decoder_test_file(char *in_file, char *out_file, audio_info_t *info)
{
    esp_audio_simple_dec_type_t type = get_simple_decoder_type(in_file);
    if (type == ESP_AUDIO_SIMPLE_DEC_TYPE_NONE) {
        ESP_LOGE(TAG, "Not supported file format %s", in_file);
        return -1;
    }
    simp_dec_fp = fopen(in_file, "rb");
    if (simp_dec_fp == NULL) {
        ESP_LOGE(TAG, "File %s not found", in_file);
        return -1;
    }
#ifdef AUD_COMPARE
    simp_dec_wr_fp = fopen(out_file, "rb");
    if (simp_dec_wr_fp == NULL) {
        ESP_LOGE(TAG, "File %s not found", out_file);
        return -1;
    }
    cmp_buf = calloc(1, 4096);
#else
    simp_dec_wr_fp = fopen(out_file, "wb");
    if (simp_dec_wr_fp == NULL) {
        ESP_LOGE(TAG, "File %s not found", out_file);
        return -1;
    }
#endif
    audio_codec_test_cfg_t dec_cfg = {
        .read = read_raw_from_file,
        .write = write_pcm_to_file,
    };
    int ret = audio_simple_decoder_test(type, &dec_cfg, info);
    if (ret < 0) {
        ESP_LOGE(TAG, "Fail to do simple decoder process, ret:%d", ret);
    }
    fclose(simp_dec_fp);
    simp_dec_fp = NULL;
    fclose(simp_dec_wr_fp);
    simp_dec_wr_fp = NULL;
#ifdef AUD_COMPARE
    free(cmp_buf);
    cmp_buf = NULL;
#endif
    return ret;
}

char inname[40][100] = {
    //aac
    "/sdcard/audio_files/aac/0_44100_2_16_223000_44.aac",
    "/sdcard/audio_files/aac/1_24000_1_16_46000_143.aac",
    "/sdcard/audio_files/aac/2_22050_1_16_61000_256.aac",
    "/sdcard/audio_files/aac/3_22050_2_16_92000_256.aac",
    "/sdcard/audio_files/aac/4_22050_1_16_57000_272.aac",
    "/sdcard/audio_files/aac/5_44100_2_16_32000_9.aac",
    "/sdcard/audio_files/aac/6_8000_1_16_0_1.aac",
    //amrnb
    "/sdcard/audio_files/amr/0_8000_1_12800_56.amrnb",
    //amrwb
    "/sdcard/audio_files/amr/1_16000_1_24000_56.amrwb",
    //flac
    "/sdcard/audio_files/flac/0_44100_2_940662_170.flac",
    //m4a
    "/sdcard/audio_files/m4a/0_44100_2_127000_1659.m4a",
    "/sdcard/audio_files/m4a/2_44100_2_283167_205.m4a",
    "/sdcard/audio_files/m4a/3_44100_1_192000_3.m4a",
    //mp3
    "/sdcard/audio_files/mp3/0_32000_2_16_88000_45.mp3",
    "/sdcard/audio_files/mp3/1_16000_1_16_17000_1.mp3",
    "/sdcard/audio_files/mp3/2_44100_2_128007_282.mp3",
    "/sdcard/audio_files/mp3/3_48000_1_64000_10.mp3",
    "/sdcard/audio_files/mp3/4_48000_2_224000_20.mp3",
    "/sdcard/audio_files/mp3/5_32000_2_128000_212.mp3",
    "/sdcard/audio_files/mp3/6_44100_2_128000_598.mp3",
    //ts
    "/sdcard/audio_files/ts/0_44100_2_63000_7.ts",
    //wav
    "/sdcard/audio_files/wav/0_44100_2_1411862_77.wav",
    "/sdcard/audio_files/wav/1_48000_1_192000_17.wav",
    "/sdcard/audio_files/wav/2_44100_2_705000_77.wav",
    "/sdcard/audio_files/wav/3_44100_2_705000_77.wav",
};

char outname[40][100] = {
    //aac
    "/sdcard/audio_files/aac/0_44100_2_16_223000_44.pcm",
    "/sdcard/audio_files/aac/1_24000_1_16_46000_143.pcm",
    "/sdcard/audio_files/aac/2_22050_1_16_61000_256.pcm",
    "/sdcard/audio_files/aac/3_22050_2_16_92000_256.pcm",
    "/sdcard/audio_files/aac/4_22050_1_16_57000_272.pcm",
    "/sdcard/audio_files/aac/5_44100_2_16_32000_9.pcm",
    "/sdcard/audio_files/aac/6_8000_1_16_0_1.pcm",
    //amrnb
    "/sdcard/audio_files/amr/0_8000_1_12800_56.pcm",
    //amrwb
    "/sdcard/audio_files/amr/1_16000_1_24000_56.pcm",
    //flac
    "/sdcard/audio_files/flac/0_44100_2_940662_170.pcm",
    //m4a
    "/sdcard/audio_files/m4a/0_44100_2_127000_1659.pcm",
    "/sdcard/audio_files/m4a/2_44100_2_283167_205.pcm",
    "/sdcard/audio_files/m4a/3_44100_1_192000_3.pcm",
    //mp3
    "/sdcard/audio_files/mp3/0_32000_2_16_88000_45.pcm",
    "/sdcard/audio_files/mp3/1_16000_1_16_17000_1.pcm",
    "/sdcard/audio_files/mp3/2_44100_2_128007_282.pcm",
    "/sdcard/audio_files/mp3/3_48000_1_64000_10.pcm",
    "/sdcard/audio_files/mp3/4_48000_2_224000_20.pcm",
    "/sdcard/audio_files/mp3/5_32000_2_128000_212.pcm",
    "/sdcard/audio_files/mp3/6_44100_2_128000_598.pcm",
    //ts
    "/sdcard/audio_files/ts/0_44100_2_63000_7.pcm",
    //wav
    "/sdcard/audio_files/wav/0_44100_2_1411862_77.pcm",
    "/sdcard/audio_files/wav/1_48000_1_192000_17.pcm",
    "/sdcard/audio_files/wav/2_44100_2_705000_77.pcm",
    "/sdcard/audio_files/wav/3_44100_2_705000_77.pcm",
};

TEST_CASE("Specific music file test", CODEC_TEST_MODULE_NAME)
{
    audio_codec_sdcard_init();
    audio_info_t aud_info = {0};
    for (int i = 0; i < 25; i++) {
        printf("%s\n", inname[i]);
        audio_simple_decoder_test_file(inname[i], outname[i], &aud_info);
    }
    audio_codec_sdcard_deinit();
}

TEST_CASE("AAC simple decoder test", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    // Prepare buffer to hold pcm data and encoded data
    memset(&read_ctx, 0, sizeof(read_ctx));

    int size = 40 * 1024;
    read_ctx.data = malloc(size);
    TEST_ASSERT_NOT_NULL(read_ctx.data);
    read_ctx.size = size;

    memset(&write_ctx, 0, sizeof(write_ctx));
    size = 20 * 1024;
    write_ctx.data = malloc(size);
    TEST_ASSERT_NOT_NULL(write_ctx.data);
    write_ctx.size = size;
    audio_info_t aud_info = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 2,
    };
    // Get encoded data and save into `write_ctx.data`
    TEST_ESP_OK(get_encode_data(&aud_info));

    // Do simple decoder test to read encoded data, gather the decoded pcm data size to `write_ctx.decode_size`
    audio_info_t dec_info = { 0 };
    audio_codec_test_cfg_t dec_cfg = {
        .read = simple_decoder_read_data,
        .write = simple_decoder_write_pcm,
    };
    TEST_ESP_OK(audio_simple_decoder_test(ESP_AUDIO_SIMPLE_DEC_TYPE_AAC, &dec_cfg, &dec_info));

    // Verify the decoder results
    TEST_ASSERT_EQUAL_INT(write_ctx.decode_size, read_ctx.read_size);
    bool info_same = (memcmp(&dec_info, &aud_info, sizeof(audio_info_t)) == 0);
    TEST_ASSERT_TRUE(info_same);
    // Clear up resources
    free(write_ctx.data);
    free(read_ctx.data);
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("Simple Decoder query type", CODEC_TEST_MODULE_NAME)
{
    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB), ESP_AUDIO_ERR_NOT_SUPPORT);
    esp_audio_dec_register_default();
    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB), ESP_AUDIO_ERR_OK);
    esp_audio_dec_unregister_default();

    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_M4A), ESP_AUDIO_ERR_NOT_SUPPORT);
    esp_audio_simple_dec_register_default();
    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_M4A), ESP_AUDIO_ERR_OK);
    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_WAV), ESP_AUDIO_ERR_OK);
    TEST_ASSERT_EQUAL(esp_audio_simple_check_audio_type(ESP_AUDIO_SIMPLE_DEC_TYPE_TS), ESP_AUDIO_ERR_OK);
    esp_audio_simple_dec_unregister_default();
}