/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "midi_button.h"
#include "midi_play.h"

static adc_oneshot_unit_handle_t bsp_adc_handle = NULL;
static QueueHandle_t button_queue = NULL;
static const char *TAG = "MIDI_BUTTON";

static const button_config_t bsp_button_config[BSP_BUTTON_NUM] = {
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_REC,
        .adc_button_config.min = 2310, // middle is 2410mV
        .adc_button_config.max = 2510
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_MUTE,
        .adc_button_config.min = 1880, // middle is 1980mV
        .adc_button_config.max = 2080
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_PLAY,
        .adc_button_config.min = 1550, // middle is 1650mV
        .adc_button_config.max = 1750
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_SET,
        .adc_button_config.min = 1010, // middle is 1110mV
        .adc_button_config.max = 1210
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_VOLDOWN,
        .adc_button_config.min = 720, // middle is 820mV
        .adc_button_config.max = 920
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_handle = &bsp_adc_handle,
        .adc_button_config.adc_channel = ADC_CHANNEL_4, // ADC1 channel 4 is GPIO5
        .adc_button_config.button_index = BSP_BUTTON_VOLUP,
        .adc_button_config.min = 280, // middle is 380mV
        .adc_button_config.max = 480
    },
};

static void btn_handler(void *button_handle, void *usr_data)
{
    int button_pressed = (int)usr_data;
    xQueueSend(button_queue, &button_pressed, 0);
}

void midi_button_task(void *arg)
{
    int button_pressed = 0;
    TaskHandle_t midi_play_task_handle = (TaskHandle_t)arg;

    while (xQueueReceive(button_queue, &button_pressed, portMAX_DELAY) == pdPASS) {
        ESP_LOGD(TAG, "button_pressed: %d", button_pressed);
        switch (button_pressed) {
            case BSP_BUTTON_VOLUP:
                midi_volume_increase();
                break;
            case BSP_BUTTON_VOLDOWN:
                midi_volume_decrease();
                break;
            case BSP_BUTTON_SET:
                midi_file_index_increase();
                break;
            case BSP_BUTTON_PLAY:
                midi_event_play();
                break;
            case BSP_BUTTON_MUTE:
                midi_event_mute();
                break;
            case BSP_BUTTON_REC:
                midi_event_rec();
                break;
            default:
                break;
        }
    }
    
}

void midi_button_init(void)
{
    esp_err_t ret = ESP_OK;

    /* Init audio buttons */
    button_handle_t btns[BSP_BUTTON_NUM - 1];

    /* Initialize ADC and get ADC handle */
    ESP_ERROR_CHECK(bsp_adc_initialize());
    bsp_adc_handle = bsp_adc_get_handle();

    button_queue = xQueueCreate(10, sizeof(int));
    if (button_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return;
    }

    for (int i = 0; i < BSP_BUTTON_NUM - 1; i++) {
        btns[i] = iot_button_create(&bsp_button_config[i]);
        if (btns[i] == NULL) {
            ret = ESP_FAIL;
            break;
        }
    }

    for (int i = 0; i < BSP_BUTTON_NUM - 1; i++) {
        ESP_ERROR_CHECK(iot_button_register_cb(btns[i], BUTTON_PRESS_DOWN, btn_handler, (void *) i));
    }
}
