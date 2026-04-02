/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"
#include "esp_hls_extractor.h"
#include "esp_hls_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    esp_extractor_config_t  base_cfg;  /*!< Base extractor configuration for HLS playlist */
    esp_hls_stream_io_t     hls_io;    /*!< Segment/playlist IO, forwarded by HLS extractor through ctrl */
} esp_hls_extractor_cfg_t;

/**
 * @brief  Initialize HLS extractor helper configuration with file based IO
 *
 * @note  This API pre-opens `url` by `hls_io->open` and binds aligned read/seek/size callbacks
 *        into `base_cfg` for HLS playlist probing/reading
 *
 * @param[in]  url            Source URL of the HLS playlist
 * @param[in]  extract_mask   Stream mask used by extractor
 * @param[in]  out_pool_size  Output pool size in bytes
 * @param[in]  out_align      Output alignment in bytes
 *
 * @return
 *       - Pointer  to helper configuration on success
 *       - NULL     on failure
 */
esp_hls_extractor_cfg_t *esp_hls_extractor_file_cfg_init(const char *url,
                                                         uint8_t extract_mask,
                                                         uint32_t out_pool_size,
                                                         uint16_t out_align);

/**
 * @brief  Deinitialize helper configuration created by file based init
 *
 * @param[in]  cfg  Helper configuration instance
 */
void esp_hls_extractor_file_cfg_deinit(esp_hls_extractor_cfg_t *cfg);

/**
 * @brief  Initialize HLS extractor helper configuration with external pool
 *
 * @param[in]  url            Source URL of the HLS playlist
 * @param[in]  gmf_pool       External GMF pool handle
 * @param[in]  extract_mask   Stream mask used by extractor
 * @param[in]  out_pool_size  Output pool size in bytes
 * @param[in]  out_align      Output alignment in bytes
 *
 * @return
 *       - Pointer  to helper configuration on success
 *       - NULL     on failure
 */
esp_hls_extractor_cfg_t *esp_hls_extractor_io_cfg_init(const char *url,
                                                       void *gmf_pool,
                                                       uint8_t extract_mask,
                                                       uint32_t out_pool_size,
                                                       uint16_t out_align);

/**
 * @brief  Deinitialize helper configuration created by IO based init
 *
 * @param[in]  cfg  Helper configuration instance
 */
void esp_hls_extractor_io_cfg_deinit(esp_hls_extractor_cfg_t *cfg);

/**
 * @brief  Open extractor with helper configuration and inject HLS IO through control API
 *
 * @param[in]   cfg        Helper configuration instance
 * @param[out]  extractor  Extractor handle output
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - ESP_EXTRACTOR_ERR_NO_MEM       Memory not enough
 *       - Others                         Failed to open extractor
 */
esp_extractor_err_t esp_hls_extractor_open_with_cfg(esp_hls_extractor_cfg_t *cfg,
                                                    esp_extractor_handle_t *extractor);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
