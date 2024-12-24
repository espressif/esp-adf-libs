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
#endif

/**
 * @brief  Initialize the knob
 *
 * @param  input_queue  Input queue handle
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t knob_init(void *input_queue);

/**
 * @brief  Deinitialize the knob
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t knob_deinit(void);

#ifdef __cplusplus
}
#endif
