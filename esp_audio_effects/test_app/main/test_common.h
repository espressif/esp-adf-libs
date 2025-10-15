/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include "esp_http_client.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Audio information structure
 */
typedef struct {
    int  sample_rate;      /*!< The audio sample rate */
    int  channels;         /*!< The number of audio channels */
    int  bits_per_sample;  /*!< The audio bits per sample */
} ae_audio_info_t;

/**
 * @brief  HTTP context structure for audio data transfer
 */
typedef struct {
    esp_http_client_handle_t  client;       /*!< HTTP client handle */
    char                     *buffer;       /*!< Buffer for HTTP data */
    size_t                    buffer_size;  /*!< Size of the buffer */
    size_t                    buffer_pos;   /*!< Current position in buffer */
    size_t                    total_size;   /*!< Total size of data to transfer */
    size_t                    current_pos;  /*!< Current position in overall transfer */
    bool                      is_upload;    /*!< Flag indicating if this is an upload operation */
} ae_http_context_t;

/**
 * @brief  Initialize HTTP download operation
 *
 * @param[in]  ctx           HTTP context
 * @param[in]  filename      Name of file to download
 * @param[in]  download_url  Base URL for download
 *
 * @return
 *       - ESP_OK    on success
 *       - ESP_FAIL  on error
 */
esp_err_t ae_http_download_init(ae_http_context_t *ctx, const char *filename, char *download_url);

/**
 * @brief  Initialize HTTP upload operation
 *
 * @param[in]  ctx         HTTP context
 * @param[in]  filename    Name of file to upload
 * @param[in]  audio_info  Audio information structure
 * @param[in]  upload_url  URL for upload
 *
 * @return
 *       - ESP_OK    on success
 *       - ESP_FAIL  on error
 */
esp_err_t ae_http_upload_init(ae_http_context_t *ctx, const char *filename, ae_audio_info_t *audio_info, char *upload_url);

/**
 * @brief  Read data from HTTP connection
 *
 * @param[out]  ptr    Buffer to store read data
 * @param[in]   size   Size of each element to read
 * @param[in]   count  Number of elements to read
 * @param[in]   ctx    HTTP context
 *
 * @return
 *       - Number of elements read on success
 *       - -1 on error
 */
int ae_http_read(void *ptr, size_t size, size_t count, ae_http_context_t *ctx);

/**
 * @brief  Write data to HTTP connection
 *
 * @param[in]  ptr        Data to write
 * @param[in]  size       Size of each element to write
 * @param[in]  count      Number of elements to write
 * @param[in]  is_finish  Flag indicating if this is the final write
 * @param[in]  ctx        HTTP context
 *
 * @return
 *       - Number of elements written on success
 *       - -1 on error
 */
int ae_http_write(const void *ptr, size_t size, size_t count, bool is_finish, ae_http_context_t *ctx);

/**
 * @brief  Deinitialize HTTP context
 *
 * @param[in]  ctx  HTTP context to deinitialize
 */
void ae_http_deinit(ae_http_context_t *ctx);

/**
 * @brief  Parse WAV header from HTTP data
 *
 * @param[in]   ctx              HTTP context
 * @param[out]  sample_rate      Sample rate from WAV header
 * @param[out]  channels         Number of channels from WAV header
 * @param[out]  bits_per_sample  Bits per sample from WAV header
 * @param[out]  data_offset      Offset to audio data in WAV file
 */
void ae_http_parse_wav_header(ae_http_context_t *ctx, int *sample_rate, int *channels,
                              int *bits_per_sample, long *data_offset, uint32_t *data_size);
#ifdef __cplusplus
}
#endif  /* __cplusplus */
