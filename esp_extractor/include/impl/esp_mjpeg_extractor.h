/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_MJPEG_EXTRACTOR_MAX_BOUNDARY_LEN  (70)
#define ESP_MJPEG_EXTRACTOR_DEFAULT_FPS       (25)

/**
 * @brief  Register extractor for concatenated or multipart MJPEG streams
 *
 * @note  Supported extra controls:
 *          - ESP_EXTRACTOR_CTRL_TYPE_SET_MJPEG_FPS:
 *            Set frames per second using a non-zero uint16_t. The default is 25.
 *          - ESP_EXTRACTOR_CTRL_TYPE_SET_MJPEG_BOUNDARY:
 *            Set a NUL-terminated multipart boundary string, excluding the leading "--".
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_mjpeg_extractor_register(void);

/**
 * @brief  Unregister the MJPEG stream extractor
 *
 * @note  Do not unregister while an extractor is in use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_mjpeg_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
