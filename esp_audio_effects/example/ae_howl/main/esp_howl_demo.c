/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_ae_howl.h"
#include "esp_ae_mixer.h"
#include "esp_ae_types.h"
#include "esp_board_manager_includes.h"
#include "esp_codec_dev.h"
#include "esp_heap_caps.h"
#include "dev_button.h"

#define TAG              "ESP_HOWL_DEMO"
#define SAMPLE_RATE      16000
#define CHANNELS         1
#define BITS_PER_SAMPLE  16
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE >> 3)

static dev_audio_codec_handles_t *audio_codec_input_handle  = NULL;
static dev_audio_codec_handles_t *audio_codec_output_handle = NULL;
FILE                             *fin                       = NULL;
bool                              do_howl_process           = false;

static void *howl_aligned_calloc(size_t alignment, size_t n, size_t size)
{
#if CONFIG_SPIRAM
    return heap_caps_aligned_calloc(alignment, 1, n * size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
#else
    return heap_caps_aligned_calloc(alignment, 1, n * size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#endif  /* CONFIG_SPIRAM */
}

static void simple_button_event_handler(void *arg, void *data)
{
    button_handle_t btn_handle = (button_handle_t)arg;
    button_event_t event = iot_button_get_event(btn_handle);
    switch (event) {
        case BUTTON_SINGLE_CLICK:
            if (strcmp(data, "PLAY") == 0) {
                ESP_LOGI(TAG, "Switch on howl suppression");
                do_howl_process = true;
            } else if (strcmp(data, "SET") == 0) {
                ESP_LOGI(TAG, "Switch off howl suppression");
                do_howl_process = false;
            }
            break;
        default:
            break;
    }
}

static esp_err_t init_adc_button(void)
{
    ESP_LOGI(TAG, "=== Testing ADC Button ===");
    // Get button handle
    dev_button_handles_t *btn_handles = NULL;
    esp_err_t ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC button");
        return ESP_FAIL;
    }
    // Try to get button handle, support both single button and button group
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP, (void **)&btn_handles);
    if (ret != ESP_OK || btn_handles == NULL) {
        ESP_LOGE(TAG, "Failed to get ADC button handle");
        return ESP_FAIL;
    }
    ret = esp_board_device_callback_register(ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP, simple_button_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callback for button: %s", ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void demo_howl_process(void)
{
    ESP_LOGI(TAG, "=== Howl Suppression Demo ===");

    uint8_t *audio_buffer = NULL;
    uint8_t *music_buffer = NULL;
    uint8_t *out_buffer = NULL;
    esp_ae_howl_handle_t howl_handle = NULL;
    esp_ae_mixer_handle_t mixer_handle = NULL;

    ESP_LOGI(TAG, "[ 1 ] Initialize Howl Suppression");
    // Real-time optimized parameters: more sensitive thresholds for faster detection
    // These parameters are tuned for real-time embedded scenarios
    esp_ae_howl_cfg_t howl_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .papr_th = 8.0,
        .phpr_th = 45.0,
        .pnpr_th = 45.0,
        .imsd_th = 5.0,
        .enable_imsd = true,
    };
    esp_ae_err_t ret = esp_ae_howl_open(&howl_cfg, &howl_handle);
    if (ret != ESP_AE_ERR_OK) {
        ESP_LOGE(TAG, "Failed to create Howl handle: %d", ret);
        goto cleanup;
    }

    // Get frame size first
    uint32_t frame_size = 0;
    ret = esp_ae_howl_get_frame_size(howl_handle, &frame_size);
    if (ret != ESP_AE_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get frame size: %d", ret);
        goto cleanup;
    }

    ESP_LOGI(TAG, "[ 1.1 ] Initialize Mixer");
    esp_ae_mixer_info_t mixer_src_info[2] = {
        {
            .weight1 = 0.5f,
            .weight2 = 1.0f,
            .transit_time = 100,
        },
        {
            .weight1 = 0.5f,
            .weight2 = 1.0f,
            .transit_time = 100,
        },
    };
    esp_ae_mixer_cfg_t mixer_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .src_num = 2,
        .src_info = mixer_src_info,
    };
    ret = esp_ae_mixer_open(&mixer_cfg, &mixer_handle);
    if (ret != ESP_AE_ERR_OK) {
        ESP_LOGE(TAG, "Failed to create mixer handle: %d", ret);
        goto cleanup;
    }
    // Allocate buffer based on frame size (PSRAM when enabled, else internal RAM)
    audio_buffer = (uint8_t *)howl_aligned_calloc(16, 1, frame_size);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        goto cleanup;
    }
    music_buffer = (uint8_t *)howl_aligned_calloc(16, 1, frame_size);
    if (music_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate music buffer");
        goto cleanup;
    }
    out_buffer = (uint8_t *)howl_aligned_calloc(16, 1, frame_size);
    if (out_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate output buffer");
        goto cleanup;
    }
    ESP_LOGI(TAG, "[ 2 ] Start recording from codec, processing with Howl, and playing through codec");
    esp_ae_mixer_set_mode(mixer_handle, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
    esp_ae_mixer_set_mode(mixer_handle, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
    vTaskDelay(pdMS_TO_TICKS(2000));
    while (1) {
        // Read audio data from codec (recording) - must read exactly frame_size bytes
        int mic_read_ret = esp_codec_dev_read(audio_codec_input_handle->codec_dev,
                                              audio_buffer, frame_size);
        if (mic_read_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to read from codec: %d", mic_read_ret);
            break;
        }
        uint32_t samples_read = frame_size / (CHANNELS * BYTES_PER_SAMPLE);
        if (do_howl_process) {
            /* Process with Howl suppression + CPU loading stats */
            ret = esp_ae_howl_process(howl_handle, audio_buffer, audio_buffer);
            if (ret != ESP_AE_ERR_OK) {
                ESP_LOGE(TAG, "Howl process failed: %d", ret);
                break;
            }
        }
        // Mix mic data with music data
        size_t music_bytes_read = fread(music_buffer, 1, frame_size, fin);
        if (music_bytes_read != frame_size) {
            ESP_LOGW(TAG, "End of the music file");
            break;
        }
        esp_ae_sample_t mix_in_samples[2] = {
            (esp_ae_sample_t)audio_buffer,
            (esp_ae_sample_t)music_buffer,
        };
        ret = esp_ae_mixer_process(mixer_handle, samples_read, mix_in_samples, (esp_ae_sample_t)out_buffer);
        if (ret != ESP_AE_ERR_OK) {
            ESP_LOGE(TAG, "Mixer process failed: %d", ret);
            break;
        }
        // Write processed audio data to codec (playback)
        int written = esp_codec_dev_write(audio_codec_output_handle->codec_dev,
                                          out_buffer, frame_size);
        if (written != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write to codec: %d", written);
            break;
        }
    }
cleanup:
    if (mixer_handle != NULL) {
        esp_ae_mixer_close(mixer_handle);
        mixer_handle = NULL;
    }
    // Cleanup howl resources
    if (howl_handle != NULL) {
        esp_ae_howl_close(howl_handle);
        howl_handle = NULL;
    }
    if (audio_buffer != NULL) {
        free(audio_buffer);
        audio_buffer = NULL;
    }
    if (music_buffer != NULL) {
        free(music_buffer);
        music_buffer = NULL;
    }
    if (out_buffer != NULL) {
        free(out_buffer);
        out_buffer = NULL;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing board manager...");

    int ret = ESP_OK;
    do_howl_process = false;
    // Initialize input codec (for recording)
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize input codec device");
        goto cleanup;
    }
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_ADC,
                                              (void **)&audio_codec_input_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get input audio codec device");
        goto cleanup;
    }
    // Initialize output codec (for playback)
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize output codec device");
        goto cleanup;
    }
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC,
                                              (void **)&audio_codec_output_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get output audio codec device");
        goto cleanup;
    }
    // esp_codec_dev_set_out_mute(audio_codec_output_handle->codec_dev, true);
    // Initialize ADC button
    ret = init_adc_button();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC button");
        goto cleanup;
    }
    // Open SD card
    ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card");
        goto cleanup;
    }
    fin = fopen("/sdcard/test_dukou.pcm", "rb");
    if (fin == NULL) {
        ESP_LOGE(TAG, "Failed to open SD card1111");
        goto cleanup;
    }
    // Configure input codec
    esp_codec_dev_sample_info_t input_fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
    };
    ret = esp_codec_dev_open(audio_codec_input_handle->codec_dev, &input_fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open input audio codec device");
        goto cleanup;
    }
    esp_codec_dev_set_in_gain(audio_codec_input_handle->codec_dev, 30.0f);
    // Configure output codec
    esp_codec_dev_sample_info_t output_fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
    };
    ret = esp_codec_dev_open(audio_codec_output_handle->codec_dev, &output_fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open output audio codec device");
        goto cleanup;
    }
    // esp_codec_dev_set_out_mute(audio_codec_output_handle->codec_dev, false);
    ret = esp_codec_dev_set_out_vol(audio_codec_output_handle->codec_dev, 80);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGW(TAG, "Failed to set DAC volume");
    }
    ESP_LOGI(TAG, "Running Howl demo...");
    demo_howl_process();

cleanup:
    // Cleanup codec resources
    if (audio_codec_input_handle != NULL && audio_codec_input_handle->codec_dev != NULL) {
        esp_codec_dev_close(audio_codec_input_handle->codec_dev);
    }
    if (audio_codec_output_handle != NULL && audio_codec_output_handle->codec_dev != NULL) {
        esp_codec_dev_close(audio_codec_output_handle->codec_dev);
    }
    fclose(fin);
    esp_board_device_deinit(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    esp_board_device_deinit(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    esp_board_device_deinit(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    esp_board_device_deinit(ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP);
    ESP_LOGI(TAG, "Demo finished.");
}
