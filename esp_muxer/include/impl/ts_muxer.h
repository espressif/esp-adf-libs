/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef TS_MUXER_H
#define TS_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TS muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config;         /*!< Base configuration */
    uint32_t           pat_resend_duration; /*!< Elapsed time for resending PAT and PMT table */
} ts_muxer_config_t;

/**
 * @brief Register muxer for TS container
 *
 * @return
 *      - ESP_MUXER_ERR_OK: Register ok
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MUXER_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t ts_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
