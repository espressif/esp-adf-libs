/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  HLS file type
 */
typedef enum {
    ESP_HLS_FILE_TYPE_PLAYLIST = 0,  /*!< Playlist file */
    ESP_HLS_FILE_TYPE_AUDIO    = 1,  /*!< Audio file */
    ESP_HLS_FILE_TYPE_VIDEO    = 2,  /*!< Video file */
    ESP_HLS_FILE_TYPE_AV       = 3,  /*!< Audio and video file */
} esp_hls_file_type_t;

/**
 * @brief  HLS file segment information
 */
typedef struct {
    uint32_t  format;  /*!< File format in FourCC code */
} esp_hls_file_seg_info_t;

/**
 * @brief  Callback invoked when a file segment is detected
 *
 * @param[in]  info  Segment information
 * @param[in]  ctx   User context
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*hls_file_seg_detected_cb)(esp_hls_file_seg_info_t *info, void *ctx);

/**
 * @brief  Open url callback
 *
 * @note  Read API use input contex to open file and return file handle
 *        File handle is used as context for following other IO operations in `esp_hls_stream_io_t`
 *
 * @param[in]  url        URL to open
 * @param[in]  input_ctx  User context for open file
 *
 * @return
 *       - NULL    On failure
 *       - Others  Pointer to the opened file
 */
typedef void *(*hls_open_func)(char *url, void *input_ctx);

/**
 * @brief  Reload url
 *
 * @param[in]  ctx  User context
 * @param[in]  url  URL to reload
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*hls_reload_func)(void *ctx, char *url);

/**
 * @brief  Cache read abort callback
 *
 * @param[in]  ctx  User context
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*hls_read_abort_func)(void *ctx);

/**
 * @brief  Read callback for input data
 *
 * @param[in]  buffer  Buffer to read data into
 * @param[in]  size    Size of the data to read
 * @param[in]  ctx     User context
 *
 * @return
 *       - >=      0    Number of bytes read, 0 means end of file
 *       - Others  On failure
 */
typedef int (*hls_read_func)(void *buffer, uint32_t size, void *ctx);

/**
 * @brief  Seek callback for input data
 *
 * @param[in]  position  Position to seek to
 * @param[in]  ctx       User context
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*hls_seek_func)(uint32_t position, void *ctx);

/**
 * @brief  Get total size callback for input
 *
 * @param[in]  ctx  User context
 *
 * @return
 *       - Total  size of the file
 */
typedef uint32_t (*hls_file_size_func)(void *ctx);

/**
 * @brief  HLS close callback
 *
 * @param[in]  ctx  User context
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*hls_close_func)(void *ctx);

/**
 * @brief  HLS stream IO callback collection
 */
typedef struct {
    hls_open_func        open;           /*!< Open the file */
    hls_reload_func      reload;         /*!< Reload the file */
    hls_file_size_func   get_file_size;  /*!< Get total size of the file */
    hls_read_func        read;           /*!< Read data from the file */
    hls_seek_func        seek;           /*!< Seek to the position in the file */
    hls_read_abort_func  read_abort;     /*!< Abort the read operation */
    hls_close_func       close;          /*!< Close the file */
    void                *input_ctx;      /*!< User context for open file */
    char                *m3u8;           /*!< M3U8 URL */
} esp_hls_stream_io_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
