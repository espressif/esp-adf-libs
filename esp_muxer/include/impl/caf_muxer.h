/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef CAF_MUXER_H
#define CAF_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief CAF muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config;      /*!< Base configuration */
} caf_muxer_config_t;

/**
 * @brief Register muxer for CAF (Apple Core Audio Format) container
 *
 * @return
 *      - ESP_MUXER_ERR_OK: Register ok
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MUXER_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t caf_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
