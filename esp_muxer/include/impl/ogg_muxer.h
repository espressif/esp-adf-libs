/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef OGG_MUXER_H
#define OGG_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OGG muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config;     /*!< Base configuration */
    uint32_t           page_cache_size; /*!< Cache data until data size exceed cache size (optional)
                                             OGG files contain multiple of pages, small input frames can be gathered into page
                                             Page is flushed into storage when page cache is full
                                             Through this method to decrease the page head overloading */
} ogg_muxer_config_t;

/**
 * @brief Register muxer for OGG container
 *
 * @return
 *      - ESP_MEDIA_ERR_OK: Register ok
 *      - ESP_MEDIA_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MEDIA_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t ogg_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
