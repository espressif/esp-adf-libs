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

static const int matrix_index_array[5][5] = {
    {0, 2, 4, 5, 7},
    {9, 11, 12, 14, 16},
    {17, 19, 21, 23, 24},
    {1, 3, 6, 8, 10},
    {13, 15, 18, 20, 22},
};

/**
 * @brief  Initialize the matrix keyboard
 *
 * @param  input_queue  Input queue handle
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t matrix_keyboard_init(void *input_queue);

/**
 * @brief  Deinitialize the matrix keyboard
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t matrix_keyboard_deinit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
