/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_video_enc_default.h"
#include "esp_audio_enc_default.h"
#include "esp_board_device.h"
#include "esp_board_manager_defs.h"
#include "esp_muxer_default.h"
#include "esp_capture.h"
#include "esp_capture_defaults.h"
#include "esp_capture_sink.h"
#ifdef CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT
#include "dev_camera.h"
#endif  /* CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT */
#include "dev_audio_codec.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"
#include "esp_board_manager_defs.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG  "VIDEO_MUXER"

#define MAX_RECORD_DURATION    (60000)  // Unit milliseconds
#define RECORD_SLICE_DURATION  (MAX_RECORD_DURATION + 1000)
#define VIDEO_RECORD_FORMAT    (ESP_MUXER_VDEC_MJPEG)
#define VIDEO_RECORD_FORMAT_2  (ESP_MUXER_VDEC_H264)
#if CONFIG_IDF_TARGET_ESP32P4
#define VIDEO_RECORD_WIDTH   (1280)
#define VIDEO_RECORD_HEIGHT  (720)
#define VIDEO_RECORD_FPS     (30)
#else
#define VIDEO_RECORD_WIDTH   (640)
#define VIDEO_RECORD_HEIGHT  (480)
#define VIDEO_RECORD_FPS     (5)
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
#define AUDIO_RECORD_FORMAT          (ESP_MUXER_ADEC_AAC)
#define AUDIO_RECORD_SAMPLE_RATE     (16000)
#define AUDIO_RECORD_CHANNEL         (2)
#define AUDIO_RECORD_BIT_PER_SAMPLE  (16)

typedef union {
    ts_muxer_config_t   ts_cfg;
    mp4_muxer_config_t  mp4_cfg;
    flv_muxer_config_t  flv_cfg;
    wav_muxer_config_t  wav_cfg;
    caf_muxer_config_t  caf_cfg;
    ogg_muxer_config_t  ogg_cfg;
    avi_muxer_config_t  avi_cfg;
} muxer_all_cfg_t;

typedef struct {
    esp_capture_handle_t        capture;     /*!< Capture handle */
    esp_capture_audio_src_if_t *aud_src;     /*!< Audio source interface for video capture */
    esp_capture_video_src_if_t *vid_src;     /*!< Video source interface */
    esp_muxer_type_t            muxer_type;  /*!< Muxer type */
} muxer_res_t;

static esp_muxer_type_t cur_muxer_type;
static esp_muxer_video_codec_t cur_vid_codec;

static void capture_scheduler(const char *thread_name, esp_capture_thread_schedule_cfg_t *schedule_cfg)
{
    if (strcmp(thread_name, "venc_0") == 0) {
        // For H264 may need huge stack if use hardware encoder can set it to small value
        schedule_cfg->core_id = 0;
        schedule_cfg->stack_size = 40 * 1024;
        schedule_cfg->priority = 1;
    } else if (strcmp(thread_name, "aenc_0") == 0) {
        // For OPUS encoder it need huge stack, when use G711 can set it to small value
        schedule_cfg->stack_size = 40 * 1024;
        schedule_cfg->priority = 2;
        schedule_cfg->core_id = 1;
    } else if (strcmp(thread_name, "AUD_SRC") == 0) {
        schedule_cfg->priority = 15;
    }
}

static esp_capture_video_src_if_t *create_video_source(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT
    dev_camera_handle_t *camera_handle = NULL;
    esp_err_t ret = esp_board_device_get_handle(ESP_BOARD_DEVICE_NAME_CAMERA, (void **)&camera_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get camera device");
        return NULL;
    }
    esp_capture_video_v4l2_src_cfg_t v4l2_cfg = {
        .buf_count = 2,
    };
    strncpy(v4l2_cfg.dev_name, camera_handle->dev_path, sizeof(v4l2_cfg.dev_name) - 1);
    return esp_capture_new_video_v4l2_src(&v4l2_cfg);
#else
    return NULL;
#endif  /* CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT */
}

static int muxer_init(muxer_res_t *muxer_res)
{
    // Create video source firstly
    muxer_res->vid_src = create_video_source();
    if (muxer_res->vid_src == NULL) {
        ESP_LOGE(TAG, "Fail to create video source");
        return -1;
    }
    esp_codec_dev_handle_t record_handle = NULL;
    dev_audio_codec_handles_t *codec_handle = NULL;
    esp_err_t ret = esp_board_device_get_handle(ESP_BOARD_DEVICE_NAME_AUDIO_ADC, (void **)&codec_handle);
    if (ret == ESP_OK) {
        record_handle = codec_handle->codec_dev;
    }
    if (record_handle) {
        // Set default input gain to be 32db
        esp_codec_dev_set_in_gain(record_handle, 32);
        esp_capture_audio_dev_src_cfg_t codec_cfg = {
            .record_handle = record_handle
        };
        muxer_res->aud_src = esp_capture_new_audio_dev_src(&codec_cfg);
        if (muxer_res->aud_src == NULL) {
            ESP_LOGE(TAG, "Fail to create audio source");
            return -1;
        }
    }
    esp_capture_cfg_t capture_cfg = {
        .sync_mode = ESP_CAPTURE_SYNC_MODE_AUDIO,
        .audio_src = muxer_res->aud_src,
        .video_src = muxer_res->vid_src,
    };
    esp_capture_open(&capture_cfg, &muxer_res->capture);
    if (muxer_res->capture == NULL) {
        ESP_LOGE(TAG, "Fail to create capture");
        return -1;
    }
    return 0;
}

static void muxer_deinit(muxer_res_t *muxer_res)
{
    if (muxer_res->capture) {
        esp_capture_close(muxer_res->capture);
        muxer_res->capture = NULL;
    }
    if (muxer_res->aud_src) {
        free(muxer_res->aud_src);
        muxer_res->aud_src = NULL;
    }
    if (muxer_res->vid_src) {
        free(muxer_res->vid_src);
        muxer_res->vid_src = NULL;
    }
}

static int slice_cb(char* file_path, int len, int slice_idx)
{
    char *ext = NULL;
    char *codec = NULL;
    switch (cur_muxer_type) {
        case ESP_MUXER_TYPE_AVI:
            ext = "avi";
            break;
        case ESP_MUXER_TYPE_MP4:
            ext = "mp4";
            break;
        case ESP_MUXER_TYPE_TS:
            ext = "ts";
            break;
        case ESP_MUXER_TYPE_FLV:
            ext = "flv";
            break;
        default:
            return -1;
    }
    switch (cur_vid_codec) {
        case ESP_MUXER_VDEC_H264:
            codec = "h264";
            break;
        case ESP_MUXER_VDEC_MJPEG:
            codec = "mjpeg";
            break;
        default:
            return -1;
    }
    snprintf(file_path, len, "/sdcard/muxed_%s.%s", codec, ext);
    ESP_LOGI(TAG, "Start to write to file %s", file_path);
    return 0;
}

static int get_muxer_config(muxer_res_t *muxer_res, muxer_all_cfg_t *cfg, int duration)
{
    memset(cfg, 0, sizeof(muxer_all_cfg_t));
    esp_muxer_config_t *base_cfg = &cfg->ts_cfg.base_config;
    base_cfg->muxer_type = muxer_res->muxer_type;
    base_cfg->url_pattern = slice_cb;
    base_cfg->slice_duration = RECORD_SLICE_DURATION;
    base_cfg->ram_cache_size = 16 * 1024; // This will consume 16k RAM space
    int cfg_size = 0;
    switch (muxer_res->muxer_type) {
        case ESP_MUXER_TYPE_TS:
            cfg_size = sizeof(ts_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_MP4:
            cfg_size = sizeof(mp4_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_FLV:
            cfg_size = sizeof(flv_muxer_config_t);
            break;
        case ESP_MUXER_TYPE_AVI:
            cfg_size = sizeof(avi_muxer_config_t);
            cfg->avi_cfg.index_type = AVI_MUXER_INDEX_AT_END;
            break;
        default:
            break;
    }
    return cfg_size;
}

static int record_file(muxer_res_t *muxer_res, int duration, esp_muxer_video_codec_t vid_codec)
{
    int ret = 0;
    do {
        esp_capture_sink_cfg_t sink_cfg = {
            .video_info = {
                .format_id = (esp_capture_format_id_t)vid_codec,
                .width = VIDEO_RECORD_WIDTH,
                .height = VIDEO_RECORD_HEIGHT,
                .fps = VIDEO_RECORD_FPS,
            },
            .audio_info = {
                .format_id = (esp_capture_format_id_t)AUDIO_RECORD_FORMAT,
                .sample_rate = AUDIO_RECORD_SAMPLE_RATE,
                .channel = AUDIO_RECORD_CHANNEL,
                .bits_per_sample = AUDIO_RECORD_BIT_PER_SAMPLE,
            },
        };
        esp_capture_sink_handle_t sink = NULL;
        ret = esp_capture_sink_setup(muxer_res->capture, 0, &sink_cfg, &sink);
        if (ret != ESP_CAPTURE_ERR_OK) {
            ESP_LOGE(TAG, "Fail to setup sink");
            break;
        }
#ifndef CONFIG_IDF_TARGET_ESP32P4
        if (muxer_res->vid_src) {
            // Force to use RGB565
            esp_capture_video_info_t fixed_caps = {
                .format_id = ESP_CAPTURE_FMT_ID_RGB565,
                .width = VIDEO_RECORD_WIDTH,
                .height = VIDEO_RECORD_HEIGHT,
                .fps = VIDEO_RECORD_FPS,
            };
            muxer_res->vid_src->set_fixed_caps(muxer_res->vid_src, &fixed_caps);
        }
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
        // Information for file slice path
        cur_vid_codec = vid_codec;
        cur_muxer_type = muxer_res->muxer_type;

        muxer_all_cfg_t all_cfg = {0};
        esp_capture_muxer_cfg_t muxer_cfg = {
            .base_config = &all_cfg.ts_cfg.base_config,
        };
        muxer_cfg.cfg_size = get_muxer_config(muxer_res, &all_cfg, duration);
        ret = esp_capture_sink_add_muxer(sink, &muxer_cfg);
        // Streaming while muxer no need special settings
        if (ret != ESP_CAPTURE_ERR_OK) {
            ESP_LOGW(TAG, "Fail to add muxer return %d", ret);
        }
        // Enable muxer but disable streaming output of audio and video
        esp_capture_sink_enable_muxer(sink, true);
        esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_VIDEO);
        esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_AUDIO);

        // Enable sink and start
        esp_capture_sink_enable(sink, ESP_CAPTURE_RUN_MODE_ALWAYS);
        ret = esp_capture_start(muxer_res->capture);
        if (ret != ESP_CAPTURE_ERR_OK) {
            ESP_LOGE(TAG, "Fail to start video capture");
            break;
        }
        // Sleep until time reached
        uint32_t start_time = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t cur_time = start_time;
        while (cur_time < start_time + duration) {
            vTaskDelay(100);
            cur_time = (uint32_t)(esp_timer_get_time() / 1000);
        }
        ret = esp_capture_stop(muxer_res->capture);
        if (ret != ESP_CAPTURE_ERR_OK) {
            ESP_LOGE(TAG, "Fail to start video capture");
            break;
        }
    } while (0);
    return ret;
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_err_t ret;
#if CONFIG_IDF_TARGET_ESP32P4
    // TMP solution, remove in future
    esp_board_periph_init(ESP_BOARD_PERIPH_NAME_LDO_MIPI);
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_CAMERA);
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Skip muxer for mount sdcard failed");
        return;
    }

    // Register default encoders and muxer
    esp_audio_enc_register_default();
    esp_video_enc_register_default();
    esp_muxer_register_default();

    // Set scheduler
    esp_capture_set_thread_scheduler(capture_scheduler);

    muxer_res_t muxer_res = {0};
    muxer_init(&muxer_res);
    esp_muxer_type_t muxer_lists[] = { ESP_MUXER_TYPE_AVI, ESP_MUXER_TYPE_MP4,
                                       ESP_MUXER_TYPE_TS, ESP_MUXER_TYPE_FLV};
    for (int i = 0; i < sizeof(muxer_lists) / sizeof(muxer_lists[0]); i++) {
        muxer_res.muxer_type = muxer_lists[i];
        record_file(&muxer_res, MAX_RECORD_DURATION, VIDEO_RECORD_FORMAT);
        // Record for second video codec if supported
#ifdef VIDEO_RECORD_FORMAT_2
        record_file(&muxer_res, MAX_RECORD_DURATION, VIDEO_RECORD_FORMAT_2);
#endif  /* VIDEO_RECORD_FORMAT_2 */
    }
    muxer_deinit(&muxer_res);
    esp_audio_enc_unregister_default();
    esp_video_enc_unregister_default();
    esp_muxer_unregister_default();
}
