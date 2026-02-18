/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MP4_MUXER_H
#define MP4_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief MP4 muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config;      /*!< Base configuration */
    bool               display_in_order; /*!< Whether display in order: dts == pts */
    bool               moov_before_mdat; /*!< Put moov before mdat box or not*/
} mp4_muxer_config_t;

/**
 * @brief Register muxer for MP4 container
 *
 * @return
 *      - ESP_MUXER_ERR_OK: Register ok
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MUXER_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t mp4_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
