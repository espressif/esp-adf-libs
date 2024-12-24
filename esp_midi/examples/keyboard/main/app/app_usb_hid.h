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
 * @brief  Initialize USB HID
 *
 * @param  input_queue  Input queue handle
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t usb_hid_init(void *input_queue);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
