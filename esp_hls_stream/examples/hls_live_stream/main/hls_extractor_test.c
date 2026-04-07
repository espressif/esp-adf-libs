/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_hls_extractor.h"
#include "esp_hls_helper.h"
#include "esp_extractor_defaults.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_io_file.h"
#include "esp_gmf_io_http.h"
#include "media_lib_os.h"
#include "esp_timer.h"
#include "esp_log.h"

#define TAG  "HLS_EXTRACTOR_TEST"

#define FOURCC_TO_CHARS(cc)  ((char *)&cc)[0], ((char *)&cc)[1], ((char *)&cc)[2], ((char *)&cc)[3]

static esp_gmf_pool_handle_t pool = NULL;

static int prepare()
{
    if (pool == NULL) {
        esp_gmf_pool_init(&pool);
        // Prepare pool
        http_io_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
        esp_gmf_io_handle_t http = NULL;
        http_cfg.dir = ESP_GMF_IO_DIR_READER;
        http_cfg.event_handle = NULL;
        esp_gmf_io_http_init(&http_cfg, &http);
        esp_gmf_pool_register_io(pool, http, NULL);

        file_io_cfg_t fs_cfg = FILE_IO_CFG_DEFAULT();
        fs_cfg.dir = ESP_GMF_IO_DIR_READER;
        esp_gmf_io_handle_t fs = NULL;
        esp_gmf_io_file_init(&fs_cfg, &fs);
        esp_gmf_pool_register_io(pool, fs, NULL);
    }

    // register extractor
    esp_hls_extractor_register();
    esp_ts_extractor_register();
    esp_audio_es_extractor_register();
    return 0;
}

static void release()
{
    if (pool) {
        esp_gmf_pool_deinit(pool);
        pool = NULL;
    }
    esp_hls_extractor_unregister();
    esp_ts_extractor_unregister();
    esp_audio_es_extractor_unregister();
}

int parse_hls(char *file, int duration)
{
    if (file == NULL || duration == 0) {
        return -1;
    }
    int ret = 0;
    prepare();
    // Generate configuration
    esp_hls_extractor_cfg_t *cfg = esp_hls_extractor_io_cfg_init(file, pool, ESP_EXTRACT_MASK_AV, 100 * 1024, 64);
    esp_extractor_handle_t extractor = NULL;
    do {
        if (cfg == NULL) {
            ret = -1;
            break;
        }
        // Open with configuration
        ret = esp_hls_extractor_open_with_cfg(cfg, &extractor);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to open extractor ret %d", ret);
            break;
        }

        // Parse stream
        ret = esp_extractor_parse_stream(extractor);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to parse stream ret %d", ret);
            break;
        }
        int audio_frame_count = 0;
        int video_frame_count = 0;
        uint32_t cur_time = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t end_time = cur_time + duration;
        // Loop to get all frames
        while (cur_time < end_time) {
            cur_time = (uint32_t)(esp_timer_get_time() / 1000);
            esp_extractor_frame_info_t frame = {};
            ret = esp_extractor_read_frame(extractor, &frame);
            if (ret == ESP_EXTRACTOR_ERR_OK) {
                if (frame.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
                    if (audio_frame_count == 0) {
                        esp_extractor_stream_info_t stream_info = {};
                        ret = esp_extractor_get_stream_info(extractor, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, 0, &stream_info);
                        ESP_LOGI(TAG, "Audio codec %c%c%c%c sample_rate:%d channel:%d",
                                 FOURCC_TO_CHARS(stream_info.audio_info.format),
                                 (int)stream_info.audio_info.sample_rate,
                                 stream_info.audio_info.channel);
                    }
                    ESP_LOGI(TAG, "%d Audio size:%d pts:%d", audio_frame_count, (int)frame.frame_size, (int)frame.pts);
                    for (int i = 0; i < 7; i++) {
                        printf("%02x ", frame.frame_buffer[i]);
                    }
                    printf("\n");
                    audio_frame_count++;
                } else if (frame.stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
                    if (video_frame_count == 0) {
                        esp_extractor_stream_info_t stream_info = {};
                        ret = esp_extractor_get_stream_info(extractor, ESP_EXTRACTOR_STREAM_TYPE_VIDEO, 0, &stream_info);
                        ESP_LOGI(TAG, "Video codec %c%c%c%c %dx%d fps:%d",
                                 FOURCC_TO_CHARS(stream_info.video_info.format),
                                 (int)stream_info.video_info.width, stream_info.video_info.height,
                                 stream_info.video_info.fps);
                        ESP_LOGI(TAG, "%d Video size:%d pts:%d", audio_frame_count, (int)frame.frame_size, (int)frame.pts);
                    }
                    ESP_LOGI(TAG, "%d Video size:%d pts:%d", video_frame_count, (int)frame.frame_size, (int)frame.pts);
                    for (int i = 0; i < 16; i++) {
                        printf("%02x ", frame.frame_buffer[i]);
                    }
                    printf("\n");
                    video_frame_count++;
                }
                esp_extractor_release_frame(extractor, &frame);
            } else if (ret == ESP_EXTRACTOR_ERR_EOS) {
                break;
            } else if (ret == ESP_EXTRACTOR_ERR_WAITING_OUTPUT) {
                media_lib_thread_sleep(20);
                continue;
            } else {
                ESP_LOGE(TAG, "Failed to read frame ret %d", ret);
                break;
            }
        }
    } while (0);
    if (extractor) {
        esp_extractor_close(extractor);
    }
    if (cfg) {
        esp_hls_extractor_io_cfg_deinit(cfg);
    }
    release();
    return ret;
}
