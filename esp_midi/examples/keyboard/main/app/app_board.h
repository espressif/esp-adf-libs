/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Knob pin definitions
 */
#define KNOB_LEFT_PINA  35
#define KNOB_LEFT_PINB  34
#define KNOB_LEFT_PIND  33
#define KNOB_RIGHT_PINA 37
#define KNOB_RIGHT_PINB 38
#define KNOB_RIGHT_PIND 36

/**
 * @brief  Matrix keyboard pin definitions
 */
#define MATRIX_ROW0 39
#define MATRIX_ROW1 40
#define MATRIX_ROW2 41
#define MATRIX_ROW3 42
#define MATRIX_ROW4 46
#define MATRIX_COL0 6
#define MATRIX_COL1 10
#define MATRIX_COL2 11
#define MATRIX_COL3 12
#define MATRIX_COL4 13

/**
 * @brief  Power on the board
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_power_on(void);

/**
 * @brief  Initialize the PA (Power Amplifier)
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_pa_init(void);

/**
 * @brief  Deinitialize the PA (Power Amplifier)
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_pa_deinit(void);

/**
 * @brief  Initialize the speaker
 *
 * @param  i2s_port     I2S port number
 * @param  sample_rate  Sample rate
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_speaker_init(uint8_t i2s_port, uint32_t sample_rate);

/**
 * @brief  Deinitialize the speaker
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_speaker_deinit(void);

/**
 * @brief  Write data to speaker
 *
 * @param  data  Audio data buffer
 * @param  size  Data size in bytes
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t app_board_speaker_write(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
