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
 * @brief File-level metadata key-value pair
 *        Stored under moov/udta/meta/ilst (ffmpeg compatible)
 */
typedef struct {
    const char *k; /*!< Metadata key, e.g. "title", "artist" */
    const char *v; /*!< Metadata value */
} esp_muxer_meta_t;

/**
 * @brief Metadata array
 */
typedef struct {
    esp_muxer_meta_t *meta_arr; /*!< Metadata entries */
    int               arr_num;  /*!< Number of entries */
} esp_muxer_meta_arr_t;

/**
 * @brief  Set file-level metadata for MP4 muxer
 *         Metadata is deep-copied and applied to every slice.
 *
 * @note  Timing: call after esp_muxer_open() and before the first
 *        esp_muxer_add_audio_packet()/esp_muxer_add_video_packet().
 *        Once muxer starts writing (writer created), this API is rejected.
 *
 * @param[in]  muxer  MP4 muxer handle from esp_muxer_open
 * @param[in]  meta   Metadata array, NULL to clear
 *
 * @return
 *      - ESP_MUXER_ERR_OK           On success
 *      - ESP_MUXER_ERR_INVALID_ARG  Invalid input argument
 *      - ESP_MUXER_ERR_WRONG_STATE  Writer already created (packet mux started)
 *      - ESP_MUXER_ERR_NO_MEM       Memory not enough
 */
esp_muxer_err_t mp4_muxer_set_meta(esp_muxer_handle_t muxer, esp_muxer_meta_arr_t *meta);

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
