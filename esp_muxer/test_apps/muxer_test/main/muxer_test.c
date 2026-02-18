/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <math.h>
#include "unity.h"
#include "unity_test_utils_memory.h"
#include "esp_muxer.h"
#include "esp_muxer_default.h"
#include "esp_muxer_version.h"
#include "esp_audio_enc.h"
#include "esp_audio_enc_default.h"
#include "esp_board_manager.h"
#include "esp_board_device.h"
#include "esp_log.h"

#define MUXER_MODULE_NAME       "[esp_muxer]"
#define TAG                     "MUXER_TEST"
#define WAVE_AMPLITUDE          (16000.0)
#define WAVE_FREQUECY           (1000)
#define LEAKS                   (400)
#define ELEMS(arr)              (sizeof(arr) / sizeof(arr[0]))
#define MAX_MUXER_NUM           (16)
#define DEFAULT_FRAME_DURATION  (20)  // ms
#define DEFAULT_SAMPLE_RATE     (16000)
#define DEFAULT_CHANNEL         (2)
#define FOURCC_TO_CHARS(cc)     ((char *)&cc)[0], ((char *)&cc)[1], ((char *)&cc)[2], ((char *)&cc)[3]
#define FILE_SLICE_PATTERN      "/sdcard/slice_%d.%s"

#define ASSIGN_BASIC_CFG(cfg)  {                   \
    cfg->sample_rate     = info->sample_rate;      \
    cfg->bits_per_sample = info->bits_per_sample;  \
    cfg->channel         = info->channel;          \
}

typedef union {
    ts_muxer_config_t   ts_cfg;
    mp4_muxer_config_t  mp4_cfg;
    flv_muxer_config_t  flv_cfg;
    wav_muxer_config_t  wav_cfg;
    caf_muxer_config_t  caf_cfg;
    ogg_muxer_config_t  ogg_cfg;
} muxer_all_cfg_t;

typedef struct {
    esp_muxer_type_t    muxer_type;
    int                 cfg_size;
    int                 audio_stream_idx;
    esp_muxer_handle_t  muxer;
    int                 muxer_pts;
    int                 total_muxer_size;
} muxer_info_t;

/**
 * @brief  Basic audio information
 */
typedef struct {
    uint8_t  channel;          /*!< Audio channel */
    uint8_t  bits_per_sample;  /*!< Audio bits per sample */
    int      sample_rate;      /*!< Sample rate */
    uint8_t *pcm_data;         /*!< PCM data */
    int      pcm_size;         /*!< PCM data size */
} audio_info_t;

typedef union {
    esp_aac_enc_config_t   aac_cfg;
    esp_alac_enc_config_t  alac_cfg;
    esp_opus_enc_config_t  opus_cfg;
} enc_all_cfg_t;

typedef struct {
    esp_audio_type_t        enc_type;
    esp_audio_enc_handle_t  enc_handle;
    uint8_t                *spec_info;
    int                     spec_info_size;
    uint32_t                pts;
    uint32_t                pcm_pos;
    uint32_t                pcm_frame_size;
    uint32_t                encoded_frame_size;
    uint8_t                *encoded_data;
} aud_enc_info_t;

static bool mount_success = false;

static void gen_pcm(audio_info_t *info)
{
    uint8_t *data = info->pcm_data;
    int size = info->pcm_size;
    int sample_size = info->bits_per_sample * info->channel >> 3;
    // align to samples
    int sample_count = size / sample_size;
    // only support 30k
    if (info->bits_per_sample == 16) {
        int i = 0;
        int16_t *v = (int16_t *)data;
        int fs = info->sample_rate >= 32000 ? WAVE_FREQUECY : info->sample_rate / 32;
        // Gen 1KHZ tone
        float coef = 6.2831853 * fs / info->sample_rate;
        while (i < sample_count) {
            int16_t t = (int16_t)(WAVE_AMPLITUDE * sin(coef * i));
            *(v++) = t;
            if (info->channel == 2) {
                *(v++) = t;
            }
            i++;
        }
    }
}

static esp_muxer_audio_codec_t get_prefer_audio_codec(esp_muxer_type_t type)
{
    switch (type) {
        default:
            return ESP_MUXER_ADEC_AAC;
        case ESP_MUXER_TYPE_OGG:
            return ESP_MUXER_ADEC_OPUS;
        case ESP_MUXER_TYPE_CAF:
            return ESP_MUXER_ADEC_ALAC;
    }
}

static bool is_muxer_support_streaming(esp_muxer_type_t type)
{
    const static esp_muxer_type_t streaming_types[] = {
        ESP_MUXER_TYPE_TS,
        ESP_MUXER_TYPE_FLV,
        ESP_MUXER_TYPE_WAV,
        ESP_MUXER_TYPE_OGG,
    };
    for (int i = 0; i < ELEMS(streaming_types); i++) {
        if (streaming_types[i] == type) {
            return true;
        }
    }
    return false;
}

static int get_muxer_types(bool streaming, esp_muxer_type_t *muxer_arr)
{
    static const esp_muxer_type_t streaming_types[] = {
        ESP_MUXER_TYPE_TS, ESP_MUXER_TYPE_FLV, ESP_MUXER_TYPE_WAV, ESP_MUXER_TYPE_OGG, ESP_MUXER_TYPE_CAF};
    int muxer_num = 0;
    for (int i = 0; i < ELEMS(streaming_types); i++) {
        if (streaming == false) {
            muxer_arr[muxer_num++] = streaming_types[i];
            continue;
        }
        if (is_muxer_support_streaming(streaming_types[i])) {
            muxer_arr[muxer_num++] = streaming_types[i];
        }
    }
    return muxer_num;
}

static int get_encoder_config(esp_audio_type_t type, esp_audio_enc_config_t *enc_cfg, audio_info_t *info)
{
    enc_all_cfg_t *all_cfg = (enc_all_cfg_t *)(enc_cfg->cfg);
    switch (type) {
        case ESP_AUDIO_TYPE_AAC: {
            esp_aac_enc_config_t *cfg = &all_cfg->aac_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_aac_enc_config_t);
            cfg->bitrate = 90000;
            cfg->adts_used = true;
            break;
        }
        case ESP_AUDIO_TYPE_OPUS: {
            esp_opus_enc_config_t *cfg = &all_cfg->opus_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_opus_enc_config_t);
            cfg->bitrate = 90000;
            cfg->frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS;
            cfg->application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO;
            break;
        }
        case ESP_AUDIO_TYPE_ALAC: {
            esp_alac_enc_config_t *cfg = &all_cfg->alac_cfg;
            ASSIGN_BASIC_CFG(cfg);
            enc_cfg->cfg_sz = sizeof(esp_alac_enc_config_t);
            break;
        }
        default:
            ESP_LOGE(TAG, "Not supported encoder type %d", type);
            return -1;
    }
    return 0;
}

static int get_muxer_audio_frame(aud_enc_info_t *enc_info, audio_info_t *info, esp_muxer_audio_packet_t *audio_packet)
{
    if (enc_info->enc_handle == NULL) {
        // Try to open encoder
        enc_all_cfg_t all_cfg = {0};
        esp_audio_enc_config_t enc_cfg = {
            .type = enc_info->enc_type,
            .cfg = &all_cfg,
        };
        int ret = get_encoder_config(enc_info->enc_type, &enc_cfg, info);
        if (ret != 0) {
            return ret;
        }
        ret = esp_audio_enc_open(&enc_cfg, &enc_info->enc_handle);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGE(TAG, "Failed to open encoder %c%c%c%c ret: %d", FOURCC_TO_CHARS(enc_info->enc_type), ret);
            return ret;
        }
        esp_audio_enc_info_t enc_info_out = {0};
        esp_audio_enc_get_info(enc_info->enc_handle, &enc_info_out);
        if (enc_info_out.spec_info_len) {
            enc_info->spec_info_size = enc_info_out.spec_info_len;
            enc_info->spec_info = (uint8_t *)enc_info_out.codec_spec_info;
        }
        enc_info->pcm_pos = 0;
        enc_info->pcm_frame_size = DEFAULT_FRAME_DURATION * info->sample_rate / 1000 * info->channel * info->bits_per_sample / 8;
        int in_size = (int)enc_info->pcm_frame_size;
        int out_size = 0;
        esp_audio_enc_get_frame_size(enc_info->enc_handle, &in_size, &out_size);
        enc_info->pcm_frame_size = (uint32_t)in_size;
        enc_info->encoded_frame_size = (uint32_t)out_size;
        enc_info->encoded_data = (uint8_t *)malloc(enc_info->encoded_frame_size);
        if (enc_info->encoded_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for encoded data");
            return -1;
        }
    }
    if (enc_info->pcm_pos + enc_info->pcm_frame_size > info->pcm_size) {
        // EOS
        return 1;
    }
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = info->pcm_data + enc_info->pcm_pos,
        .len = enc_info->pcm_frame_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = enc_info->encoded_data,
        .len = enc_info->encoded_frame_size,
    };
    int ret = esp_audio_enc_process(enc_info->enc_handle, &in_frame, &out_frame);
    if (ret != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "Failed to encode audio frame %d", ret);
        return ret;
    }
    enc_info->pcm_pos += enc_info->pcm_frame_size;
    audio_packet->pts = (uint32_t)out_frame.pts;
    audio_packet->data = out_frame.buffer;
    audio_packet->len = out_frame.encoded_bytes;
    return 0;
}

const static char *get_file_ext(esp_muxer_type_t muxer_type)
{
    switch (muxer_type) {
        case ESP_MUXER_TYPE_TS:
            return "ts";
        case ESP_MUXER_TYPE_MP4:
            return "mp4";
        case ESP_MUXER_TYPE_FLV:
            return "flv";
        case ESP_MUXER_TYPE_WAV:
            return "wav";
        case ESP_MUXER_TYPE_CAF:
            return "caf";
        case ESP_MUXER_TYPE_OGG:
            return "ogg";
        default:
            return NULL;
    }
}

static int muxer_slice_reached(esp_muxer_slice_info_t *info, void *ctx)
{
    muxer_info_t *muxer_info = (muxer_info_t *)ctx;
    snprintf(info->file_path, info->len, FILE_SLICE_PATTERN, info->slice_index, get_file_ext(muxer_info->muxer_type));
    ESP_LOGI(TAG, "Begin to write slice %s", info->file_path);
    return 0;
}

static uint32_t get_file_size(muxer_info_t *muxer_info)
{
    char file_name[64];
    snprintf(file_name, sizeof(file_name), FILE_SLICE_PATTERN, 0, get_file_ext(muxer_info->muxer_type));
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    uint32_t file_size = (uint32_t)ftell(file);
    fclose(file);
    return file_size;
}

static int muxer_data_cb(esp_muxer_data_info_t *data, void *ctx)
{
    muxer_info_t *muxer_info = (muxer_info_t *)ctx;
    muxer_info->total_muxer_size += data->size;
    return 0;
}

static void get_muxer_config(muxer_info_t *muxer_info, muxer_all_cfg_t *cfg, bool to_file, bool streaming, int duration)
{
    memset(cfg, 0, sizeof(muxer_all_cfg_t));
    esp_muxer_config_t *base_cfg = &cfg->ts_cfg.base_config;  // Use any member since base_config is at start
    base_cfg->muxer_type = muxer_info->muxer_type;
    if (to_file) {
        base_cfg->url_pattern_ex = muxer_slice_reached;
        base_cfg->ctx = muxer_info;
    } else {
        base_cfg->data_cb = muxer_data_cb;
        base_cfg->ctx = muxer_info;
    }
    base_cfg->slice_duration = duration + 1000;  // Large enough for one slice
    base_cfg->ram_cache_size = 16 * 1024;        // This will consume 16k RAM space
    switch (muxer_info->muxer_type) {
        case ESP_MUXER_TYPE_TS:
            muxer_info->cfg_size = sizeof(ts_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_MP4:
            muxer_info->cfg_size = sizeof(mp4_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_FLV:
            muxer_info->cfg_size = sizeof(flv_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_WAV:
            muxer_info->cfg_size = sizeof(wav_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_CAF:
            muxer_info->cfg_size = sizeof(caf_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_OGG:
            muxer_info->cfg_size = sizeof(ogg_muxer_config_t);
            break;
        default:
            break;
    }
}

static int one_muxer_test(audio_info_t *audio_info, esp_muxer_type_t muxer_type, bool to_file, bool streaming, int duration)
{
    aud_enc_info_t enc_info = {};
    enc_info.enc_type = get_prefer_audio_codec(muxer_type);

    muxer_info_t muxer_info = {0};
    muxer_info.muxer_type = muxer_type;
    // Get muxer config
    muxer_all_cfg_t all_cfg = {0};
    get_muxer_config(&muxer_info, &all_cfg, to_file, streaming, duration);

    int ret = 0;
    // Open muxer
    muxer_info.muxer = esp_muxer_open((esp_muxer_config_t *)&all_cfg, muxer_info.cfg_size);
    if (muxer_info.muxer == NULL) {
        ESP_LOGE(TAG, "Failed to open muxer %c%c%c%c", FOURCC_TO_CHARS(muxer_type));
        ret = -1;
        goto _clearup;
    }
    printf("\n");
    ESP_LOGI(TAG, "Muxer %c%c%c%c started", FOURCC_TO_CHARS(muxer_type));
    bool stream_added = false;
    while (1) {
        esp_muxer_audio_packet_t audio_packet = {0};
        ret = get_muxer_audio_frame(&enc_info, audio_info, &audio_packet);
        if (ret != 0) {
            if (ret != 1) {
                ESP_LOGE(TAG, "Failed to get muxer audio frame ret: %d", ret);
            } else {
                ESP_LOGI(TAG, "Muxer %c%c%c%c EOS", FOURCC_TO_CHARS(muxer_type));
                ret = 0;
                break;
            }
            goto _clearup;
        }
        if (stream_added == false) {
            stream_added = true;
            // Add audio stream
            esp_muxer_audio_stream_info_t audio_stream = {
                .min_packet_duration = DEFAULT_FRAME_DURATION,
                .bits_per_sample = audio_info->bits_per_sample,
                .sample_rate = (uint16_t)audio_info->sample_rate,
                .channel = audio_info->channel,
                .codec = enc_info.enc_type,
            };
            if (enc_info.spec_info_size > 0) {
                audio_stream.codec_spec_info = enc_info.spec_info;
                audio_stream.spec_info_len = enc_info.spec_info_size;
            }
            ret = esp_muxer_add_audio_stream(muxer_info.muxer, &audio_stream, &muxer_info.audio_stream_idx);
            if (ret != 0) {
                ESP_LOGE(TAG, "Failed to add audio stream %d", ret);
                goto _clearup;
            }
        }
        muxer_info.muxer_pts = (int)audio_packet.pts;
        // Add audio packet
        ret = esp_muxer_add_audio_packet(muxer_info.muxer, muxer_info.audio_stream_idx, &audio_packet);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to add audio packet %d", ret);
            goto _clearup;
        }
    }
    ESP_LOGI(TAG, "Muxer %c%c%c%c test success", FOURCC_TO_CHARS(muxer_type));
    if (streaming) {
        ESP_LOGI(TAG, "Muxer size: %d", muxer_info.total_muxer_size);
    }
    ESP_LOGI(TAG, "Muxer pts: %d", muxer_info.muxer_pts);
_clearup:
    // Close muxer
    if (muxer_info.muxer) {
        esp_muxer_close(muxer_info.muxer);
        muxer_info.muxer = NULL;
    }
    if (enc_info.enc_handle) {
        esp_audio_enc_close(enc_info.enc_handle);
        enc_info.enc_handle = NULL;
    }
    if (enc_info.encoded_data) {
        free(enc_info.encoded_data);
        enc_info.encoded_data = NULL;
    }
    // Get file size after close muxer to save file into storage
    if (ret == 0 && to_file) {
        uint32_t file_size = get_file_size(&muxer_info);
        ESP_LOGI(TAG, "Muxer file size: %d", (int)file_size);
    }
    return ret;
}

static int muxer_test(bool to_file, bool streaming, int duration)
{
    audio_info_t audio_info = {
        .channel = DEFAULT_CHANNEL,
        .bits_per_sample = 16,
        .sample_rate = DEFAULT_SAMPLE_RATE,
    };
    duration = duration / DEFAULT_FRAME_DURATION * DEFAULT_FRAME_DURATION;
    audio_info.pcm_size = duration * DEFAULT_SAMPLE_RATE / 1000 * audio_info.channel * audio_info.bits_per_sample / 8;
    audio_info.pcm_data = (uint8_t *)malloc(audio_info.pcm_size);
    if (audio_info.pcm_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for pcm data");
        return ESP_ERR_NO_MEM;
    }
    gen_pcm(&audio_info);

    // Register default muxer and encoder
    esp_muxer_register_default();
    esp_audio_enc_register_default();

    esp_muxer_type_t muxer_arr[MAX_MUXER_NUM] = {0};
    int muxer_num = get_muxer_types(streaming, muxer_arr);
    int fail_count = 0;
    for (int i = 0; i < muxer_num; i++) {
        int ret = one_muxer_test(&audio_info, muxer_arr[i], to_file, streaming, duration);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to test muxer %c%c%c%c", FOURCC_TO_CHARS(muxer_arr[i]));
            fail_count++;
        }
    }
    if (audio_info.pcm_data) {
        free(audio_info.pcm_data);
    }
    // Unregister default muxer and encoder
    esp_muxer_unregister_default();
    esp_audio_enc_unregister_default();
    return fail_count ? -1 : 0;
}

TEST_CASE("Muxer to file only", MUXER_MODULE_NAME)
{
    if (mount_success) {
        TEST_ESP_OK(muxer_test(true, false, 10000));
    } else {
        ESP_LOGW(TAG, "Skip test of muxer to file only");
    }
}

TEST_CASE("Muxer streaming out only", MUXER_MODULE_NAME)
{
    TEST_ESP_OK(muxer_test(false, true, 10000));
}

TEST_CASE("Muxer to file while streaming", MUXER_MODULE_NAME)
{
    if (mount_success) {
        TEST_ESP_OK(muxer_test(true, true, 10000));
    } else {
        ESP_LOGW(TAG, "Skip test of muxer to file while streaming");
    }
}

#if CONFIG_IDF_TARGET_ESP32S3 && !defined(CONFIG_HEAP_TRACING_OFF)
#include "esp_heap_trace.h"
#include "esp_heap_caps.h"
#define MAX_LEAK_TRACE_RECORDS  100
#endif  /* CONFIG_IDF_TARGET_ESP32S3 && !defined(CONFIG_HEAP_TRACING_OFF) */

static void trace_for_leak(bool start)
{
#if CONFIG_IDF_TARGET_ESP32S3 && !defined(CONFIG_HEAP_TRACING_OFF)
    static heap_trace_record_t *trace_record;
    if (trace_record == NULL) {
        trace_record = heap_caps_malloc(MAX_LEAK_TRACE_RECORDS * sizeof(heap_trace_record_t), MALLOC_CAP_SPIRAM);
        heap_trace_init_standalone(trace_record, MAX_LEAK_TRACE_RECORDS);
    }
    if (trace_record == NULL) {
        ESP_LOGE(TAG, "No memory to start trace");
        return;
    }
    static bool started = false;
    if (start) {
        if (started == false) {
            heap_trace_start(HEAP_TRACE_LEAKS);
            started = true;
        }
    } else {
        heap_trace_dump();
    }
#endif  /* CONFIG_IDF_TARGET_ESP32S3 && !defined(CONFIG_HEAP_TRACING_OFF) */
}

void setUp(void)
{
    // Mount sdcard
    esp_err_t ret = esp_board_device_init("fs_sdcard");
    mount_success = true;
    if (ret != ESP_OK) {
        mount_success = false;
    }
    unity_utils_record_free_mem();
    trace_for_leak(true);
}

void tearDown(void)
{
    esp_board_device_deinit("fs_sdcard");
    mount_success = false;
    unity_utils_evaluate_leaks_direct(LEAKS);
    trace_for_leak(false);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Start test for esp_muxer version %s", esp_muxer_get_version());
    float v = 1.0;
    printf("This line is specially used for pre-allocate float print memory %.2f\n", v);
    unity_run_menu();
}
