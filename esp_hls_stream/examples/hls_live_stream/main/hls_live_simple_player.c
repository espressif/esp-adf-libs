/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "esp_audio_simple_player.h"
#include "esp_audio_simple_player_advance.h"
#include "esp_codec_dev.h"
#include "esp_gmf_pool.h"
#include "esp_hls_io.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "esp_fourcc.h"
#include "esp_gmf_audio_dec.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_board_manager_includes.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#define TAG                 "HLS_LIVE_STREAM"
#define DEFAULT_VOLUME      (60)
#define HLS_STREAM_AAC_URL  "http://open.ls.qingting.fm/live/274/64k.m3u8?format=aac"
#define HLS_STREAM_URL      "https://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8"
#define HLS_PLAY_FOREVER    (0)
#define HLS_PLAY_DURATION   (30000)

int parse_hls(char *file, int duration);
int init_console(void);
void heap_leak_trace(bool start);

static int codec_out_cb(uint8_t *data, int data_size, void *ctx)
{
    esp_codec_dev_handle_t dev = (esp_codec_dev_handle_t)ctx;
    if (dev == NULL || data == NULL || data_size <= 0) {
        return -1;
    }
    esp_codec_dev_write(dev, data, data_size);
    return data_size;
}

static int player_event_cb(esp_asp_event_pkt_t *pkt, void *ctx)
{
    (void)ctx;
    if (pkt && pkt->type == ESP_ASP_EVENT_TYPE_STATE && pkt->payload_size == sizeof(esp_asp_state_t)) {
        esp_asp_state_t state = *(esp_asp_state_t *)pkt->payload;
        ESP_LOGI(TAG, "Player state:%s", esp_audio_simple_player_state_to_str(state));
    }
    return 0;
}

static int hls_media_type_cb(esp_hls_file_seg_info_t *info, void *ctx)
{
    static uint32_t last_format = 0;
    if (last_format != info->format) {
        esp_asp_handle_t player = (esp_asp_handle_t)ctx;
        esp_gmf_pipeline_handle_t pipeline = NULL;
        esp_gmf_element_handle_t dec_el = NULL;
        esp_audio_simple_player_get_pipeline(player, &pipeline);
        esp_gmf_info_sound_t music_info = {
            .sample_rates = CONFIG_AUDIO_SIMPLE_PLAYER_RESAMPLE_DEST_RATE,
            .channels = CONFIG_AUDIO_SIMPLE_PLAYER_CH_CVT_DEST,
            .bits = CONFIG_AUDIO_SIMPLE_PLAYER_BIT_CVT_DEST_16BIT,
        };
        music_info.format_id = info->format;
        esp_gmf_pipeline_get_el_by_name(pipeline, "aud_dec", &dec_el);
        // Old simple player use M2TS instead of TS
        if (info->format == ESP_FOURCC_TO_INT('T', 'S', ' ', ' ')) {
            music_info.format_id = ESP_FOURCC_M2TS;
        }
        esp_gmf_audio_dec_reconfig_by_sound_info(dec_el, &music_info);
        ESP_LOGI(TAG, "Detect file format %s", ESP_FOURCC_TO_STR(info->format));
    }
    return 0;
}

static void wifi_connect(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
}

static esp_codec_dev_handle_t prepare_codec(void)
{
    ESP_ERROR_CHECK(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC));
    dev_audio_codec_handles_t *device_handle = NULL;
    esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, (void **)&device_handle);
    if (device_handle == NULL || device_handle->codec_dev == NULL) {
        return NULL;
    }
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = CONFIG_AUDIO_SIMPLE_PLAYER_RESAMPLE_DEST_RATE,
        .channel = CONFIG_AUDIO_SIMPLE_PLAYER_CH_CVT_DEST,
        .bits_per_sample = 16,
    };
    esp_codec_dev_set_out_vol(device_handle->codec_dev, DEFAULT_VOLUME);
    esp_codec_dev_open(device_handle->codec_dev, &fs);
    return device_handle->codec_dev;
}

void app_main(void)
{
    // Install media library adapter
    media_lib_add_default_adapter();

    // Prepare audio codec device
    esp_codec_dev_handle_t playback_handle = prepare_codec();
    if (playback_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get audio codec device handle");
        return;
    }
    init_console();
    wifi_connect();
    heap_leak_trace(true);

    char *hls_test_stream[] = {HLS_STREAM_URL, HLS_STREAM_AAC_URL};
    // Do extractor test
    for (int i = 0; i < sizeof(hls_test_stream) / sizeof(hls_test_stream[0]); i++) {
        parse_hls(hls_test_stream[i], HLS_PLAY_DURATION);
    }

    // Do simple player playback through HLS IO
    esp_asp_cfg_t cfg = {
        .out = {
            .cb = codec_out_cb,
            .user_ctx = playback_handle},
        .task_prio = 5,
        .task_stack = 6 * 1024,
    };
    esp_asp_handle_t player = NULL;
    if (esp_audio_simple_player_new(&cfg, &player) != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to create audio simple player");
        return;
    }
    esp_gmf_pool_handle_t pool = NULL;
    esp_audio_simple_player_get_pool(player, &pool);
    // Added HLS IO into pool
    esp_gmf_io_handle_t hls_io = NULL;
    esp_hls_io_cfg_t hls_cfg = {
        .pool = pool,
        .name = "io_hls",
        .file_seg_cb = hls_media_type_cb,
        .ctx = player,
    };
    esp_gmf_io_hls_init(&hls_cfg, &hls_io);
    // Register HLS IO into player
    esp_audio_simple_player_register_io(player, hls_io);
    esp_audio_simple_player_set_event(player, player_event_cb, NULL);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
#if HLS_PLAY_FOREVER
    ESP_LOGI(TAG, "Start play %s forever", HLS_STREAM_URL);
    ret = esp_audio_simple_player_run_to_end(player, HLS_STREAM_URL, NULL);
    if (ret != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to play %s forever, ret:%d", HLS_STREAM_URL, ret);
    }
#else
    // Start playback to end
    for (int i = 0; i < sizeof(hls_test_stream) / sizeof(hls_test_stream[0]); i++) {
        ESP_LOGI(TAG, "Start play %s", hls_test_stream[i]);
        ret = esp_audio_simple_player_run(player, hls_test_stream[i], NULL);
        if (ret == ESP_GMF_ERR_OK) {
            media_lib_thread_sleep(HLS_PLAY_DURATION);
        } else {
            ESP_LOGE(TAG, "Failed to play %s, ret:%x", hls_test_stream[i], ret);
        }
        esp_audio_simple_player_stop(player);
        ESP_LOGI(TAG, "End to play %s", hls_test_stream[i]);
    }
#endif  /* HLS_PLAY_FOREVER */
    // Destroy player
    esp_audio_simple_player_destroy(player);
    heap_leak_trace(false);
}
