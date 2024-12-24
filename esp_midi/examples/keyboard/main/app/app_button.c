/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "app_board.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "app_midi_keyboard.h"
#include "app_button.h"

static const char *TAG = "BUTTON";

static button_handle_t btn_a = NULL;
static button_handle_t btn_b = NULL;

#define BUTTON_ACTIVE_LEVEL 0

static void button_a_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_BUTTON,
        .value1 = 0,
        .value2 = event,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "BUTTON A %s", iot_button_get_event_str(event));
}

static void button_b_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_BUTTON,
        .value1 = 1,
        .value2 = event,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "BUTTON B %s", iot_button_get_event_str(event));
}

esp_err_t button_init(void *input_queue)
{
    esp_err_t ret = ESP_OK;
    uint8_t level = 0;

    button_config_t btn_cfg = {0};
    button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = KNOB_LEFT_PIND,
        .active_level = BUTTON_ACTIVE_LEVEL,
    };
    ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn_a);
    if (ret != ESP_OK || btn_a == NULL) {
        ESP_LOGE(TAG, "Failed to create button A");
        return ret;
    }
    iot_button_register_cb(btn_a, BUTTON_PRESS_DOWN, NULL, button_a_event_cb, input_queue);
    iot_button_register_cb(btn_a, BUTTON_PRESS_UP, NULL, button_a_event_cb, input_queue);
    iot_button_register_cb(btn_a, BUTTON_SINGLE_CLICK, NULL, button_a_event_cb, input_queue);
    level = iot_button_get_key_level(btn_a);
    ESP_LOGD(TAG, "button A level is %d", level);

    btn_gpio_cfg.gpio_num = KNOB_RIGHT_PIND;
    ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn_b);
    if (ret != ESP_OK || btn_b == NULL) {
        ESP_LOGE(TAG, "Failed to create button B");
        iot_button_delete(btn_a);
        return ret;
    }
    iot_button_register_cb(btn_b, BUTTON_PRESS_DOWN, NULL, button_b_event_cb, input_queue);
    iot_button_register_cb(btn_b, BUTTON_PRESS_UP, NULL, button_b_event_cb, input_queue);
    iot_button_register_cb(btn_b, BUTTON_SINGLE_CLICK, NULL, button_b_event_cb, input_queue);
    level = iot_button_get_key_level(btn_b);
    ESP_LOGD(TAG, "button B level is %d", level);

    return ESP_OK;
}

esp_err_t button_deinit(void)
{
    esp_err_t ret = ESP_OK;

    if (btn_a != NULL) {
        ret = iot_button_delete(btn_a);
        btn_a = NULL;
    }
    if (btn_b != NULL) {
        ret = iot_button_delete(btn_b);
        btn_b = NULL;
    }

    return ret;
}
