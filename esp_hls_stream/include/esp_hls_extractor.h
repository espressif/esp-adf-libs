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
 * @brief  Definition of HLS extractor type
 */
#define ESP_EXTRACTOR_TYPE_HLS  EXTRACTOR_4CC('H', 'L', 'S', ' ')

/**
 * @brief  Set extra IO for HLS extractor
 *
 * @note  Argument is `esp_hls_stream_io_t`
 */
#define ESP_EXTRACTOR_CTRL_TYPE_SET_HLS_IO_EX  (0x50)

/**
 * @brief  Set read abort for HLS extractor
 */
#define ESP_EXTRACTOR_CTRL_TYPE_SET_HLS_READ_ABORT  (0x51)

/**
 * @brief  Register extractor for HLS container
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_hls_extractor_register(void);

/**
 * @brief  Unregister for HLS container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not registered yet
 */
esp_extractor_err_t esp_hls_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
