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
#include "keyboard_button.h"
#include "app_board.h"
#include "app_midi_keyboard.h"
#include "app_matrix.h"

static const char *TAG = "MATRIX";

static keyboard_btn_handle_t kbd_handle = NULL;

static void keyboard_cb(keyboard_btn_handle_t kbd_handle, keyboard_btn_report_t kbd_report, void *user_data)
{
    QueueHandle_t input_queue = (QueueHandle_t)user_data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_MATRIX,
        .value1 = 0,
        .value2 = 0,
    };

    if (kbd_report.key_pressed_num == 0) {
        input.value1 = -1;
        input.value2 = -1;
        ESP_LOGD(TAG, "All keys released");
        return;
    }
    for (int i = 0; i < kbd_report.key_pressed_num; i++) {
        input.value1 = kbd_report.key_data[i].output_index;
        input.value2 = kbd_report.key_data[i].input_index;
        xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    }
}

static keyboard_btn_handle_t kbd_init(void)
{
    keyboard_btn_config_t cfg = {
        .output_gpios = (int[])
        {
            MATRIX_ROW0, MATRIX_ROW1, MATRIX_ROW2, MATRIX_ROW3, MATRIX_ROW4
        },
        .output_gpio_num = 5,
        .input_gpios = (int[])
        {
            MATRIX_COL0, MATRIX_COL1, MATRIX_COL2, MATRIX_COL3, MATRIX_COL4
        },
        .input_gpio_num = 5,
        .active_level = 1,
        .debounce_ticks = 5,
        .ticks_interval = 500,
        .enable_power_save = true,
    };
    keyboard_btn_handle_t kbd_handle = NULL;
    keyboard_button_create(&cfg, &kbd_handle);
    return kbd_handle;
}

esp_err_t matrix_keyboard_init(void *input_queue)
{
    esp_err_t ret = ESP_OK;
    kbd_handle = kbd_init();
    if (kbd_handle == NULL) {
        ESP_LOGE(TAG, "Keyboard initialization failed");
        return ESP_FAIL;
    }

    keyboard_btn_cb_config_t cb_cfg = {
        .event = KBD_EVENT_PRESSED,
        .callback = keyboard_cb,
        .user_data = input_queue,
    };
    ret = keyboard_button_register_cb(kbd_handle, cb_cfg, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register keyboard callback");
        return ret;
    }

    return ESP_OK;
}

esp_err_t matrix_keyboard_deinit(void)
{
    esp_err_t ret = ESP_OK;
    if (kbd_handle != NULL) {
        ret = keyboard_button_delete(kbd_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete keyboard handle");
            return ret;
        }
        kbd_handle = NULL;
    }

    return ESP_OK;
}
