/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_ERRCODE_H
#define MEDIA_LIB_ERRCODE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "esp_err.h"

typedef enum {
    ESP_MEDIA_ERR_OK               = ESP_OK,
    ESP_MEDIA_ERR_FAIL             = ESP_FAIL,
    ESP_MEDIA_ERR_NO_MEM           = ESP_ERR_NO_MEM,
    ESP_MEDIA_ERR_INVALID_ARG      = ESP_ERR_INVALID_ARG,
    ESP_MEDIA_ERR_WRONG_STATE      = ESP_ERR_INVALID_STATE,
    ESP_MEDIA_ERR_INVALID_SIZE     = ESP_ERR_INVALID_SIZE,
    ESP_MEDIA_ERR_NOT_FOUND        = ESP_ERR_NOT_FOUND,
    ESP_MEDIA_ERR_NOT_SUPPORT      = ESP_ERR_NOT_SUPPORTED,
    ESP_MEDIA_ERR_TIMEOUT          = ESP_ERR_TIMEOUT,
    ESP_MEDIA_ERR_INVALID_RESPONSE = ESP_ERR_INVALID_RESPONSE,
    ESP_MEDIA_ERR_INVALID_CRC      = ESP_ERR_INVALID_CRC,
    ESP_MEDIA_ERR_INVALID_VERSION  = ESP_ERR_INVALID_VERSION,

    ESP_MEDIA_ERR_BASE         = 0x90000,
    ESP_MEDIA_ERR_READ_DATA    = (ESP_MEDIA_ERR_BASE + 1),
    ESP_MEDIA_ERR_WRITE_DATA   = (ESP_MEDIA_ERR_BASE + 2),
    ESP_MEDIA_ERR_BAD_DATA     = (ESP_MEDIA_ERR_BASE + 3),
    ESP_MEDIA_ERR_EXCEED_LIMIT = (ESP_MEDIA_ERR_BASE + 4),
    ESP_MEDIA_ERR_CONNECT_FAIL = (ESP_MEDIA_ERR_BASE + 5),
    ESP_MEDIA_ERR_RESET        = (ESP_MEDIA_ERR_BASE + 6)
} esp_media_err_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_ERRCODE_H */
