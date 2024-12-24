/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "app_board.h"
#include "app_knob.h"
#include "app_button.h"
#include "app_matrix.h"
#include "app_usb_hid.h"
#include "app_midi_keyboard.h"
#include "esp_midi_sound.h"
#include "esp_midi_flash_loader.h"

static const char *TAG = "MAIN";

void app_main()
{
    app_board_power_on();

    void *input_queue = keyboard_input_init();
    if (input_queue == NULL) {
        ESP_LOGE(TAG, "Failed to initialize keyboard input");
        return;
    }

#if CONFIG_INPUT_KEY_USB_KEYBOARD
    ESP_ERROR_CHECK(usb_hid_init(input_queue));
#elif CONFIG_INPUT_KEY_MATRIX_KEYBOARD
    ESP_ERROR_CHECK(matrix_keyboard_init(input_queue));
#endif
    ESP_ERROR_CHECK(knob_init(input_queue));
    ESP_ERROR_CHECK(button_init(input_queue));
    ESP_ERROR_CHECK(app_board_speaker_init(0, 16000));
    ESP_ERROR_CHECK(app_board_pa_init());
    ESP_ERROR_CHECK(midi_player_init());
}
