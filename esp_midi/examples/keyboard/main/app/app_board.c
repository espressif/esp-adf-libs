/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "esp_board_manager.h"
#include "esp_board_periph.h"
#include "app_board.h"

static const char *TAG = "BOARD";

static i2s_chan_handle_t tx_handle = NULL;

esp_err_t app_board_power_on(void)
{
    esp_err_t ret = ESP_OK;
    ret = esp_board_periph_init("gpio_keep_power");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO keep power pin");
        return ret;
    }
    ret = esp_board_periph_init("gpio_power_key");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO power key pin");
        return ret;
    }
    ret = esp_board_periph_init("gpio_power_vdd");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO power vdd pin");
        return ret;
    }
    return ESP_OK;
}

esp_err_t app_board_pa_init(void)
{
    esp_err_t ret = esp_board_periph_init("gpio_pa_control");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO pa control pin");
        return ret;
    }
    return ESP_OK;
}

esp_err_t app_board_pa_deinit(void)
{
    return esp_board_periph_deinit("gpio_pa_control");
}

esp_err_t app_board_speaker_init(uint8_t i2s_port, uint32_t sample_rate)
{
    esp_err_t ret = ESP_OK;
    ret = esp_board_periph_init("i2s_audio_out");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S audio out pin");
        return ret;
    }
    ret = esp_board_periph_get_handle("i2s_audio_out", (void **)&tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get I2S audio out handle");
        return ret;
    }
    return ESP_OK;
}

esp_err_t app_board_speaker_deinit(void)
{
    return esp_board_periph_deinit("i2s_audio_out");
}

esp_err_t app_board_speaker_write(uint8_t *data, size_t size)
{
    size_t bytes_written = 0;
    return i2s_channel_write(tx_handle, data, size, &bytes_written, 1000);
}
