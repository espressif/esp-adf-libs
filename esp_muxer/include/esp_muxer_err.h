/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef ESP_MUXER_ERR_H
#define ESP_MUXER_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

typedef enum {
    ESP_MUXER_ERR_OK                        = ESP_OK,
    ESP_MUXER_ERR_FAIL                      = ESP_FAIL,
    ESP_MUXER_ERR_NO_MEM                    = ESP_ERR_NO_MEM,
    ESP_MUXER_ERR_INVALID_ARG               = ESP_ERR_INVALID_ARG,
    ESP_MUXER_ERR_WRONG_STATE               = ESP_ERR_INVALID_STATE,
    ESP_MUXER_ERR_INVALID_SIZE              = ESP_ERR_INVALID_SIZE,
    ESP_MUXER_ERR_NOT_FOUND                 = ESP_ERR_NOT_FOUND,
    ESP_MUXER_ERR_NOT_SUPPORT               = ESP_ERR_NOT_SUPPORTED,
    ESP_MUXER_ERR_TIMEOUT                   = ESP_ERR_TIMEOUT,
    ESP_MUXER_ERR_INVALID_RESPONSE          = ESP_ERR_INVALID_RESPONSE,
    ESP_MUXER_ERR_INVALID_CRC               = ESP_ERR_INVALID_CRC,
    ESP_MUXER_ERR_INVALID_VERSION           = ESP_ERR_INVALID_VERSION,

    ESP_MUXER_ERR_BASE                      = 0x10000,
    ESP_MUXER_ERR_READ_DATA                 = (ESP_MUXER_ERR_BASE + 1),
    ESP_MUXER_ERR_WRITE_DATA                = (ESP_MUXER_ERR_BASE + 2),
    ESP_MUXER_ERR_BAD_DATA                  = (ESP_MUXER_ERR_BASE + 3),
    ESP_MUXER_ERR_EXCEED_LIMIT              = (ESP_MUXER_ERR_BASE + 4),
} esp_muxer_err_t;

#ifdef __cplusplus
}
#endif

#endif
