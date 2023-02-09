/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef RTMP_SRC_H
#define RTMP_SRC_H

#include <stdint.h>
#include "media_lib_err.h"
#include "media_lib_os.h"
#include "media_lib_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTMP source configuration
 */
typedef struct {
    char                  *url;        /*!< Server url, format: rtmp://ipaddress:port/app_name/stream_name */
    uint32_t               chunk_size; /*!< Maximum chunk size */
    uint32_t               fifo_size;  /*!< Ringfifo size to cache input media data */
    media_lib_thread_cfg_t thread_cfg; /*!< Configuration for receiving data thread */
    media_lib_tls_cfg_t   *ssl_cfg;    /*!< Set when use RTMPS protocol */
} rtmp_src_cfg_t;

/**
 * @brief RTMP source handle
 */
typedef void *rtmp_src_handle_t;

/**
 * @brief         Open RTMP source to read media data
 *
 * @param         cfg:  Configuration for RTMP source
 * @return        - NULL: Fail to open instance
 *                - Others: RTMP source instance handle
 */
rtmp_src_handle_t esp_rtmp_src_open(rtmp_src_cfg_t *cfg);

/**
 * @brief         Connect to RTMP server
 *
 * @param         handle:  Source handle
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid source handle
 *                - ESP_MEDIA_ERR_CONNECT_FAIL: Fail to connect to server
 */
esp_media_err_t esp_rtmp_src_connect(rtmp_src_handle_t handle);

/**
 * @brief         Read data from RTMP server
 *                This API is synchronized call
 *                It will wait for all data received or return back when meet error
 * @param         handle:  Source handle
 * @param         data:  Data pointer to read
 * @param         size:  Data size to read
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid source handle
 *                - ESP_MEDIA_ERR_READ_DATA: Read data fail
 */
esp_media_err_t esp_rtmp_src_read(rtmp_src_handle_t handle, uint8_t *data, int size);

/**
 * @brief         Close RTMP source
 *
 * @param         handle:  Source handle
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid source handle
 */
esp_media_err_t esp_rtmp_src_close(rtmp_src_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
