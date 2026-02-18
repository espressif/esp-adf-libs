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
 * @brief  OGG vorbis specified information
 *
 * @note  Comment header is skipped for not used by decoder
 */
typedef struct {
    uint8_t  *info_header;   /*!< Information header data */
    uint32_t  info_size;     /*!< Information header size */
    uint8_t  *setup_header;  /*!< Setup header data */
    uint32_t  setup_size;    /*!< Setup header size */
} ogg_vorbis_spec_info_t;

/**
 * @brief  Register extractor for OGG container
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK             Register success
 *       - ESP_EXTRACTOR_ERR_ALREADY_EXIST  Already registered
 *       - ESP_EXTRACTOR_ERR_NO_MEM         Memory not enough
 */
esp_extractor_err_t esp_ogg_extractor_register(void);

/**
 * @brief  Unregister for OGG container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_ogg_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
