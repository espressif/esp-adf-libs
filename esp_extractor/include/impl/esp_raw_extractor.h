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

/**
 * @brief  Register an extractor for a raw container
 *
 * @note  The raw extractor is a special extractor that reads raw stream data directly
 *        It provides a unified API so it can be used to support all formats
 *        The user must provide stream information and a read callback
 *        Each call to `esp_extractor_read_frame()` invokes the read callback to
 *        retrieve one frame and stores it in the internal memory pool
 *
 *        The raw extractor supports only a single stream (either audio or video)
 *
 *        Supported extra controls:
 *          - ESP_EXTRACTOR_CTRL_TYPE_SET_STREAM_INFO:
 *            Set stream information (must be called before reading frames)
 *          - ESP_EXTRACTOR_CTRL_TYPE_SET_MAX_FRAME_SIZE:
 *            Set the maximum frame size If not set, the default value is `pool_size / 2`
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Registration successful
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Insufficient memory
 */
esp_extractor_err_t esp_raw_extractor_register(void);

/**
 * @brief  Unregister for raw container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_raw_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
