/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"
#include "esp_hls_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  HLS fetcher handle
 */
typedef void *hls_fetcher_handle_t;

/**
 * @brief  Configuration for opening an HLS fetcher instance
 */
typedef struct {
    esp_hls_stream_io_t  io;                 /*!< Stream IO callbacks */
    char                *m3u8_url;           /*!< Initial playlist URL */
    uint8_t              extract_mask;       /*!< Stream extraction mask */
    uint32_t             prefer_bitrate;     /*!< Preferred download bitrate */
    void *(*get_data_cache)(void *ctx);      /*!< Optional cache getter, set NULL if not used */
    void                      *ctx;          /*!< User context */
} hls_fetch_cfg_t;

/**
 * @brief  Data container returned by HLS fetcher read API
 */
typedef struct {
    uint8_t  *data;        /*!< Data buffer */
    uint32_t  size;        /*!< Buffer size */
    uint32_t  valid_size;  /*!< Valid bytes in buffer */
    bool      bos;         /*!< Beginning-of-stream flag */
    uint32_t  format;      /*!< Stream format (FourCC) */
} hls_fetch_stream_data_t;

/**
 * @brief  Open HLS fetcher with configuration
 *
 * @param[in]   cfg      HLS fetcher configuration
 * @param[out]  fetcher  Output fetcher handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - ESP_EXTRACTOR_ERR_NO_MEM       Memory not enough
 *       - Others                         Open failed
 */
esp_extractor_err_t hls_fetcher_open(hls_fetch_cfg_t *cfg, hls_fetcher_handle_t *fetcher);

/**
 * @brief  Read one stream frame/chunk into fetcher managed buffer
 *
 * @param[in]   fetcher      HLS fetcher handle
 * @param[in]   stream_type  Stream type to read
 * @param[out]  data_info    Output data information
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Read failed
 */
esp_extractor_err_t hls_fetcher_read_data(hls_fetcher_handle_t fetcher,
                                          esp_extractor_stream_type_t stream_type,
                                          hls_fetch_stream_data_t *data_info);

/**
 * @brief  Read stream data into user buffer
 *
 * @param[in]   fetcher      HLS fetcher handle
 * @param[in]   stream_type  Stream type to read
 * @param[out]  data         User buffer for data
 * @param[in]   size         User buffer size
 * @param[out]  read_size    Bytes actually read
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Read failed
 */
esp_extractor_err_t hls_fetcher_read(hls_fetcher_handle_t fetcher,
                                     esp_extractor_stream_type_t stream_type,
                                     uint8_t *data,
                                     uint32_t size,
                                     uint32_t *read_size);

/**
 * @brief  Seek all enabled streams to target PTS
 *
 * @param[in]  fetcher  HLS fetcher handle
 * @param[in]  pts      Target PTS value
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Seek failed
 */
esp_extractor_err_t hls_fetcher_seek(hls_fetcher_handle_t fetcher, uint32_t pts);

/**
 * @brief  Enable or disable specific stream type
 *
 * @param[in]  fetcher      HLS fetcher handle
 * @param[in]  stream_type  Stream type to set
 * @param[in]  enable       True to enable, false to disable
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Operation failed
 */
esp_extractor_err_t hls_fetcher_enable_stream(hls_fetcher_handle_t fetcher, esp_extractor_stream_type_t stream_type,
                                              bool enable);

/**
 * @brief  Abort current read operation
 *
 * @param[in]  fetcher  HLS fetcher handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Abort failed
 */
esp_extractor_err_t hls_fetcher_read_abort(hls_fetcher_handle_t fetcher);

/**
 * @brief  Detect HLS file type from target URL
 *
 * @param[in]   fetcher  HLS fetcher handle
 * @param[in]   url      URL to check
 * @param[out]  type     Detected file type
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Detection failed
 */
esp_extractor_err_t hls_fetcher_get_file_type(hls_fetcher_handle_t fetcher, char *url, esp_hls_file_type_t *type);

/**
 * @brief  Close fetcher and release resources
 *
 * @param[in]  fetcher  HLS fetcher handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK           On success
 *       - ESP_EXTRACTOR_ERR_INVALID_ARG  Invalid argument
 *       - Others                         Close failed
 */
esp_extractor_err_t hls_fetcher_close(hls_fetcher_handle_t fetcher);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
