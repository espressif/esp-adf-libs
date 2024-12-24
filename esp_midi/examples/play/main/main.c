/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "midi_io.h"
#include "midi_button.h"
#include "midi_play.h"

static const char *TAG = "MAIN";

void app_main()
{
    // init sdcard
    mount_sd();

    // init midi play
    ESP_LOGI(TAG, "midi play init start");
    midi_player_init();
    xTaskCreate(midi_play_task, "midi_play_task", 20 * 1024, NULL, 5, NULL);

    // init button
    ESP_LOGI(TAG, "midi button init start");
    midi_button_init();
    xTaskCreate(midi_button_task, "midi_button_task", 15 * 1024, NULL, 5, NULL);
}
