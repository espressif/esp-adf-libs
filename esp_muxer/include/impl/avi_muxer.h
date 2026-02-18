/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef AVI_MUXER_H
#define AVI_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  AVI index table type (Supports Legacy idx1)
 *
 * @note  The index table is specially designed for fast seeking operations.
 *        When placed at the start of the file, it preserves file space
 *        The table uses memory to store index entries for quick access
 *        Set to `AVI_MUXER_INDEX_NONE` to optimize resource usage when seeking functionality
 *        is not required (simple playback only)
 */
typedef enum {
    AVI_MUXER_INDEX_NONE     = 0,  /*!< No index table */
    AVI_MUXER_INDEX_AT_END   = 1,  /*!< Insert index table at the end of file */
    AVI_MUXER_INDEX_AT_START = 2,  /*!< Insert index table at the start of file (requires seek) */
} avi_muxer_index_type_t;

/**
 * @brief  AVI muxer configuration
 */
typedef struct {
    esp_muxer_config_t      base_config;  /*!< Base configuration */
    avi_muxer_index_type_t  index_type;   /*!< Index table type */
} avi_muxer_config_t;

/**
 * @brief  Register muxer for AVI container
 *
 * @return
 *       - ESP_MUXER_ERR_OK           Register ok
 *       - ESP_MUXER_ERR_INVALID_ARG  Invalid input argument
 *       - ESP_MUXER_ERR_NO_MEM       Memory not enough
 */
esp_muxer_err_t avi_muxer_register(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* AVI_MUXER_H */
