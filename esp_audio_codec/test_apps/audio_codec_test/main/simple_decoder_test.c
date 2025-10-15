/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
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
#include "esp_audio_enc.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_enc_default.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "test_common.h"
#include "esp_board_manager.h"

extern const char test_mp3_start[] asm("_binary_test_mp3_start");
extern const char test_mp3_end[] asm("_binary_test_mp3_end");
extern const char test_flac_start[] asm("_binary_test_flac_start");
extern const char test_flac_end[] asm("_binary_test_flac_end");

#define TAG "SIMP_DEC_TEST"

typedef union {
    esp_m4a_dec_cfg_t m4a_cfg;
    esp_ts_dec_cfg_t  ts_cfg;
    esp_aac_dec_cfg_t aac_cfg;
} simp_dec_all_t;

typedef struct {
    uint8_t  *data;
    int       read_size;
    int       size;
} read_ctx_t;

typedef struct {
    uint8_t  *data;
    int       write_size;
    int       read_size;
    int       size;
    int       decode_size;
    FILE     *pcm_file;
} write_ctx_t;

static write_ctx_t                    write_ctx;
static read_ctx_t                     read_ctx;
static FILE                          *simp_dec_fp;
static FILE                          *simp_dec_wr_fp;
static char                          *cmp_buf;
static esp_audio_simple_dec_handle_t  simple_dec_hd = NULL;

static char reset_url1[][100] = {
    "/sdcard/audio_files/ut/test.aac",
    "/sdcard/audio_files/ut/test.mp3",
    "/sdcard/audio_files/ut/test.flac",
    "/sdcard/audio_files/amr/1_8000_1_12801_41.amrnb",
    "/sdcard/audio_files/amr/3_16000_1_24000_44.amrwb",
    "/sdcard/audio_files/ut/test.wav",
    "/sdcard/audio_files/ut/test.m4a",
    "/sdcard/audio_files/ut/test.ts",
};

static char reset_url2[][100] = {
    "/sdcard/audio_files/aac/2_22050_1_16_61000_256.aac",
    "/sdcard/audio_files/mp3/0_32000_2_16_88000_45.mp3",
    "/sdcard/audio_files/flac/0_44100_2_940662_170.flac",
    "/sdcard/audio_files/amr/2_8000_1_5219_2.amrnb",
    "/sdcard/audio_files/amr/1_16000_1_24000_56.amrwb",
    "/sdcard/audio_files/wav/1_48000_1_192000_17.wav",
    "/sdcard/audio_files/m4a/3_44100_1_192000_3.m4a",
    "/sdcard/audio_files/ts/1_32000_2_138000_109.ts",
};

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
    return fwrite(data, 1, size, simp_dec_wr_fp);
}

static int write_pcm_to_compare(uint8_t *data, int size)
{
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

static bool encoder_use_test_data(esp_audio_type_t type)
{
    switch (type) {
        case ESP_AUDIO_TYPE_MP3:
            write_ctx.size = (int)(test_mp3_end - test_mp3_start);
            write_ctx.write_size = (int)(test_mp3_end - test_mp3_start);
            write_ctx.data = (uint8_t *)test_mp3_start;
            return true;
        case ESP_AUDIO_TYPE_FLAC:
            write_ctx.size = (int)(test_flac_end - test_flac_start);
            write_ctx.write_size = (int)(test_flac_end - test_flac_start);
            write_ctx.data = (uint8_t *)test_flac_start;
            return true;
        default:
            return false;
    }
}

static int get_encode_data(audio_info_t *aud_info, esp_audio_type_t type)
{
    if (encoder_use_test_data(type)) {
        return 0;
    }
    int size = 40 * 1024;
    read_ctx.data = malloc(size);
    if (read_ctx.data == NULL) {
        return -1;
    }
    read_ctx.size = size;

    size = 20 * 1024;
    write_ctx.data = malloc(size);
    if (write_ctx.data == NULL) {
        return -1;
    }
    write_ctx.size = size;

    read_ctx.size = audio_codec_gen_pcm(aud_info, read_ctx.data, read_ctx.size);
    read_ctx.read_size = 0;

    audio_codec_test_cfg_t enc_cfg = {
        .read = encoder_read_pcm,
        .write = encoder_write_frame,
    };
    audio_encoder_test(type, &enc_cfg, aud_info, false);
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

static int simple_decoder_write_pcm_to_file(uint8_t *data, int size)
{
    fwrite(data, 1, size, write_ctx.pcm_file);
    write_ctx.decode_size += size;
    return size;
}

static int simple_decoder_write_pcm_to_compare(uint8_t *data, int size)
{
    uint8_t *cmp_buf = calloc(1, size);
    if (cmp_buf == NULL) {
        ESP_LOGE(TAG, "No memory for cmp_buf");
        return ESP_FAIL;
    }
    fread(cmp_buf, 1, size, write_ctx.pcm_file);
    if (memcmp(cmp_buf, data, size) != 0) {
        ESP_LOGE(TAG, "PCM data not match!(%d)", __LINE__);
        free(cmp_buf);
        return 0;
    }
    write_ctx.decode_size += size;
    free(cmp_buf);
    return size;
}

static void sdcard_url_save_cb(void *user_data, char *url)
{
    esp_err_t ret = sdcard_list_save(user_data, url);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to save sdcard url to sdcard playlist");
    }
}

static void change_extension_to_pcm(const char *input_path, char *output_path, size_t size)
{
    strncpy(output_path, input_path, size - 1);
    char *dot = strrchr(output_path, '.');
    if (dot != NULL) {
        strcpy(dot, ".pcm");
        output_path[(dot - output_path) + strlen(".pcm")] = '\0';
    } else {
        strncat(output_path, ".pcm", size - strlen(output_path) - 1);
    }
}

int audio_simple_decoder_test(esp_audio_simple_dec_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info, bool need_reset)
{
    esp_audio_simple_dec_handle_t decoder = simple_dec_hd;
    if (decoder == NULL) {
        esp_audio_dec_register_default();
        esp_audio_simple_dec_register_default();
    }
    esp_audio_err_t ret = ESP_AUDIO_ERR_OK;
    int max_out_size = 4096;
    int read_size = 512;
    uint8_t *in_buf = malloc(read_size);
    uint8_t *out_buf = malloc(max_out_size);
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
            .use_frame_dec = false,
        };
        get_simple_decoder_config(&dec_cfg);
        if (decoder == NULL) {
            ret = esp_audio_simple_dec_open(&dec_cfg, &decoder);
            if (ret != ESP_AUDIO_ERR_OK) {
                ESP_LOGI(TAG, "Fail to open simple decoder ret %d", ret);
                break;
            }
        }
        int total_decoded = 0;
        uint64_t decode_time = 0;
        esp_audio_simple_dec_raw_t raw = {
            .buffer = in_buf,
        };
        int frame_count = 0;
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
                    frame_count++;
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

    if (need_reset) {
        esp_audio_simple_dec_reset(decoder);
    } else {
        if (decoder) {
            esp_audio_simple_dec_close(decoder);
            decoder = NULL;
        }
        esp_audio_simple_dec_unregister_default();
        esp_audio_dec_unregister_default();
    }
    simple_dec_hd = decoder;
    if (in_buf) {
        free(in_buf);
    }
    if (out_buf) {
        free(out_buf);
    }
    return ret;
}

int audio_simple_decoder_test_file(char *in_file, char *out_file, audio_info_t *info, bool do_compare, bool do_reset)
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
    if (do_compare) {
        simp_dec_wr_fp = fopen(out_file, "rb");
        if (simp_dec_wr_fp == NULL) {
            ESP_LOGE(TAG, "File %s not found", out_file);
            return -1;
        }
        cmp_buf = calloc(1, 4096);
    } else {
        simp_dec_wr_fp = fopen(out_file, "wb");
        if (simp_dec_wr_fp == NULL) {
            ESP_LOGE(TAG, "File %s not found", out_file);
            return -1;
        }
    }
    audio_codec_test_cfg_t dec_cfg = {
        .read = read_raw_from_file,
        .write = do_compare ? write_pcm_to_compare : write_pcm_to_file,
    };
    int ret = audio_simple_decoder_test(type, &dec_cfg, info, do_reset);
    if (ret < 0) {
        ESP_LOGE(TAG, "Fail to do simple decoder process, ret:%d", ret);
    }
    fclose(simp_dec_fp);
    simp_dec_fp = NULL;
    fclose(simp_dec_wr_fp);
    simp_dec_wr_fp = NULL;
    if (do_compare) {
        free(cmp_buf);
        cmp_buf = NULL;
    }
    return ret;
}

static void compare_reset_pcm_files(const char *file1, const char *file2)
{
    FILE *fp1 = fopen(file1, "rb");
    TEST_ASSERT_NOT_NULL(fp1);
    FILE *fp2 = fopen(file2, "rb");
    TEST_ASSERT_NOT_NULL(fp2);
    uint8_t *buf1 = malloc(1024);
    TEST_ASSERT_NOT_NULL(buf1);
    uint8_t *buf2 = malloc(1024);
    TEST_ASSERT_NOT_NULL(buf2);
    while (1) {
        fread(buf1, 1, 1024, fp1);
        fread(buf2, 1, 1024, fp2);
        if (feof(fp1) || feof(fp2)) {
            break;
        }
        TEST_ASSERT_EQUAL_MEMORY(buf1, buf2, 1024);
    }
    if (fp1) {
        fclose(fp1);
    }
    if (fp2) {
        fclose(fp2);
    }
    if (buf1) {
        free(buf1);
    }
    if (buf2) {
        free(buf2);
    }
}

TEST_CASE("Specific music file test", CODEC_TEST_MODULE_NAME)
{
    esp_board_manager_init();
    void *playlist = {0};
    sdcard_list_create(&playlist);
    sdcard_scan(sdcard_url_save_cb, "/sdcard/audio_files",
                1, (const char *[]) {"mp3", "m4a", "flac", "amrnb", "amrwb", "ts", "aac", "wav"}, 8, playlist);
    audio_info_t aud_info = {0};
    int file_num = sdcard_list_get_url_num(playlist);
    char outname[100] = {0};
    for (int i = 0; i < file_num; i++) {
        char *inname = NULL;
        sdcard_list_choose(playlist, i, &inname);
        memset(outname, 0, sizeof(outname));
        ESP_LOGI(TAG, "inname: %s", &inname[6]);
        change_extension_to_pcm(&inname[6], outname, strlen(inname) - 6);
        audio_simple_decoder_test_file(&inname[6], outname, &aud_info, true, false);
    }
    sdcard_list_destroy(playlist);
    esp_board_manager_deinit();
}

TEST_CASE("AAC simple decoder test", CODEC_TEST_MODULE_NAME)
{
    // Backup original heap size
    int heap_size = esp_get_free_heap_size();

    // Prepare buffer to hold pcm data and encoded data
    memset(&read_ctx, 0, sizeof(read_ctx));

    memset(&write_ctx, 0, sizeof(write_ctx));
    audio_info_t aud_info = {
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel = 2,
        .no_file_header = false,
    };
    // Get encoded data and save into `write_ctx.data`
    TEST_ESP_OK(get_encode_data(&aud_info, ESP_AUDIO_TYPE_AAC));

    // Do simple decoder test to read encoded data, gather the decoded pcm data size to `write_ctx.decode_size`
    audio_info_t dec_info = {0};
    audio_codec_test_cfg_t dec_cfg = {
        .read = simple_decoder_read_data,
        .write = simple_decoder_write_pcm,
    };
    TEST_ESP_OK(audio_simple_decoder_test(ESP_AUDIO_SIMPLE_DEC_TYPE_AAC, &dec_cfg, &dec_info, false));

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

TEST_CASE("Simple decoder reset with same stream test", CODEC_TEST_MODULE_NAME)
{
    esp_audio_simple_dec_type_t types[] = {
        ESP_AUDIO_SIMPLE_DEC_TYPE_AAC,
        ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB,
        ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB,
        ESP_AUDIO_SIMPLE_DEC_TYPE_MP3,
        ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC,
    };
    esp_board_manager_init();
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        int sample_rate = 48000;
        uint8_t channel = 2;
        if (types[i] == ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB) {
            sample_rate = 8000;
            channel = 1;
        } else if (types[i] == ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB) {
            sample_rate = 16000;
            channel = 1;
        } else {
            sample_rate = 48000;
            channel = 2;
        }
        ESP_LOGI(TAG, "Testing simple decoder reset for type %s sample_rate %d channel %d",
                 esp_audio_codec_get_name(types[i]), sample_rate, channel);
        memset(&read_ctx, 0, sizeof(read_ctx));
        memset(&write_ctx, 0, sizeof(write_ctx));
        char decode_file[100];
        snprintf(decode_file, sizeof(decode_file), "/sdcard/test_%d.pcm", i);
        write_ctx.pcm_file = fopen(decode_file, "wb");
        TEST_ASSERT_NOT_NULL(write_ctx.pcm_file);
        audio_info_t aud_info = {
            .sample_rate = sample_rate,
            .bits_per_sample = 16,
            .channel = channel,
        };
        TEST_ESP_OK(get_encode_data(&aud_info, types[i]));
        audio_info_t dec_info = {0};
        audio_codec_test_cfg_t dec_cfg = {
            .read = simple_decoder_read_data,
            .write = simple_decoder_write_pcm_to_file,
        };
        TEST_ESP_OK(audio_simple_decoder_test(types[i], &dec_cfg, &dec_info, true));

        if (write_ctx.pcm_file) {
            fclose(write_ctx.pcm_file);
            write_ctx.pcm_file = NULL;
        }

        write_ctx.pcm_file = fopen(decode_file, "rb");
        TEST_ASSERT_NOT_NULL(write_ctx.pcm_file);
        write_ctx.read_size = 0;
        write_ctx.decode_size = 0;
        memset(&dec_info, 0, sizeof(dec_info));
        dec_cfg.read = simple_decoder_read_data;
        dec_cfg.write = simple_decoder_write_pcm_to_compare;
        TEST_ESP_OK(audio_simple_decoder_test(types[i], &dec_cfg, &dec_info, false));

        if (write_ctx.pcm_file) {
            fclose(write_ctx.pcm_file);
            write_ctx.pcm_file = NULL;
        }
        if (read_ctx.data) {
            free(read_ctx.data);
            read_ctx.data = NULL;
            if (write_ctx.data) {
                free(write_ctx.data);
                write_ctx.data = NULL;
            }
        }
    }
    esp_board_manager_deinit();
}

TEST_CASE("Simple decoder reset with file test", CODEC_TEST_MODULE_NAME)
{
    esp_audio_simple_dec_type_t types[] = {
        ESP_AUDIO_SIMPLE_DEC_TYPE_AAC,
        ESP_AUDIO_SIMPLE_DEC_TYPE_MP3,
        ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC,
        ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB,
        ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB,
        ESP_AUDIO_SIMPLE_DEC_TYPE_WAV,
        ESP_AUDIO_SIMPLE_DEC_TYPE_M4A,
        ESP_AUDIO_SIMPLE_DEC_TYPE_TS,
    };
    esp_board_manager_init();
    audio_info_t aud_info = {0};
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        ESP_LOGI(TAG, "Testing simple decoder reset for type %s", esp_audio_codec_get_name(types[i]));
        audio_simple_decoder_test_file(reset_url1[i], "/sdcard/test1.pcm", &aud_info, false, true);
        audio_simple_decoder_test_file(reset_url2[i], "/sdcard/test2.pcm", &aud_info, false, true);
        audio_simple_decoder_test_file(reset_url1[i], "/sdcard/test3.pcm", &aud_info, false, true);
        audio_simple_decoder_test_file(reset_url2[i], "/sdcard/test4.pcm", &aud_info, false, false);
        compare_reset_pcm_files("/sdcard/test1.pcm", "/sdcard/test3.pcm");
        compare_reset_pcm_files("/sdcard/test2.pcm", "/sdcard/test4.pcm");
    }
    esp_board_manager_deinit();
}

TEST_CASE("AAC simple decoder with frame test", CODEC_TEST_MODULE_NAME)
{
    TEST_ESP_OK(esp_aac_enc_register());
    TEST_ESP_OK(esp_aac_dec_register());
    esp_aac_enc_config_t aac_cfg = ESP_AAC_ENC_CONFIG_DEFAULT();
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_AAC,
        .cfg = &aac_cfg,
        .cfg_sz = sizeof(aac_cfg)};
    // Open encoder
    esp_audio_enc_handle_t encoder = NULL;
    TEST_ESP_OK(esp_audio_enc_open(&enc_cfg, &encoder));
    // Get needed buffer size and prepare memory
    int pcm_size = 0, raw_size = 0;
    esp_audio_enc_get_frame_size(encoder, &pcm_size, &raw_size);
    TEST_ASSERT_GREATER_THAN(0, pcm_size);
    TEST_ASSERT_GREATER_THAN(0, raw_size);
    uint8_t *pcm_data = malloc(pcm_size);
    uint8_t *raw_data = malloc(raw_size);
    TEST_ASSERT_NOT_NULL(pcm_data);
    TEST_ASSERT_NOT_NULL(raw_data);
    // Generate test pcm data
    audio_info_t aud_info = {
        .sample_rate = aac_cfg.sample_rate,
        .bits_per_sample = aac_cfg.bits_per_sample,
        .channel = aac_cfg.channel,
    };
    // Open Simple Decoder
    simp_dec_all_t all_cfg = {};
    esp_audio_simple_dec_cfg_t dec_cfg = {
        .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_AAC,
        .dec_cfg = &all_cfg,
        .cfg_size = sizeof(all_cfg),
        .use_frame_dec = true,
    };
    esp_audio_simple_dec_handle_t decoder = NULL;
    TEST_ESP_OK(esp_audio_simple_dec_open(&dec_cfg, &decoder));
    uint8_t *decode_data = malloc(pcm_size);
    TEST_ASSERT_NOT_NULL(decode_data);

    // Do encoding to decoding
    for (int i = 0; i < 20; i++) {
        audio_codec_gen_pcm(&aud_info, pcm_data, pcm_size);
        esp_audio_enc_in_frame_t in_frame = {
            .buffer = pcm_data,
            .len = pcm_size,
        };
        esp_audio_enc_out_frame_t out_frame = {
            .buffer = raw_data,
            .len = raw_size,
        };
        TEST_ESP_OK(esp_audio_enc_process(encoder, &in_frame, &out_frame));
        TEST_ASSERT_GREATER_THAN(0, out_frame.encoded_bytes);
        esp_audio_simple_dec_raw_t dec_raw = {
            .buffer = raw_data,
            .len = out_frame.encoded_bytes,
        };
        esp_audio_simple_dec_out_t dec_out = {
            .buffer = decode_data,
            .len = pcm_size,
        };
        TEST_ESP_OK(esp_audio_simple_dec_process(decoder, &dec_raw, &dec_out));
        TEST_ASSERT_EQUAL(dec_out.decoded_size, pcm_size);
    }
    esp_audio_enc_close(encoder);
    esp_audio_simple_dec_close(decoder);
    free(pcm_data);
    free(raw_data);
    free(decode_data);
}
