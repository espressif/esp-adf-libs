/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_gmf_io.h"
#include "esp_gmf_pool.h"
#include "esp_hls_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Default HLS IO configuration
 */
#define HLS_DEFAULT_TASK_STACK         (20 * 1024)
#define HLS_DEFAULT_TASK_PRIO          (20)
#define HLS_DEFAULT_TASK_CORE          (1)
#define HLS_DEFAULT_TASK_STACK_IN_EXT  (1)
#define HLS_DEFAULT_READ_SIZE          (512)
#define HLS_DEFAULT_BUFFER_SIZE        (600 * 1024)

#define DEFAULT_HLS_IO_CFG()  {                         \
    .thread = {                                         \
        .stack        = HLS_DEFAULT_TASK_STACK,         \
        .prio         = HLS_DEFAULT_TASK_PRIO,          \
        .core         = HLS_DEFAULT_TASK_CORE,          \
        .stack_in_ext = HLS_DEFAULT_TASK_STACK_IN_EXT,  \
    },                                                  \
    .buffer_cfg = {                                     \
        .io_size     = HLS_DEFAULT_READ_SIZE,           \
        .buffer_size = HLS_DEFAULT_BUFFER_SIZE,         \
        .read_filter = NULL,                            \
    },                                                  \
    .enable_speed_monitor = false                       \
}

/**
 * @brief  HLS IO get IO configuration callback
 *
 * @param[in]   file_type  HLS file type
 * @param[out]  io_cfg     IO configuration
 * @param[in]   ctx        User context
 *
 * @return
 *       - 0       On success
 *       - Others  On failure
 */
typedef int (*_hls_get_io_cfg_cb)(esp_hls_file_type_t file_type, esp_gmf_io_cfg_t *io_cfg, void *ctx);

/**
 * @brief  HLS IO initialization configuration structure
 *
 * @note  Since there is only one HLS_IO instance per application, this configuration
 *        serves both buffer management and task settings. The HLS IO implementation
 *        will attempt to open actual IO connections (e.g., HTTP) for data retrieval.
 *
 *       Playlist and media files may require different IO configurations.
 *       To accommodate this, users can provide a callback function to dynamically
 *       specify the appropriate IO configuration for each file type.
 *
 *       If no callback is provided, the default IO configuration will be applied
 *       to all operations.
 */
typedef struct {
    const char               *name;           /*!< Instance name */
    hls_file_seg_detected_cb  file_seg_cb;    /*!< Callback triggered when a new file segment is detected */
    _hls_get_io_cfg_cb        get_io_cfg_cb;  /*!< Optional callback to retrieve IO configuration for specific files
                                                  - When NULL, inherits from pool IO configuration */
    void                     *ctx;            /*!< User-defined context passed to callback functions */
    esp_gmf_pool_handle_t     pool;           /*!< Handle to external GMF pool of elements and IOs (Required) */
    esp_gmf_io_cfg_t          io_cfg;         /*!< HLS IO configuration (Optional)
                                                   Defaultly set to DEFAULT_HLS_IO_CFG() */
} esp_hls_io_cfg_t;

/**
 * @brief  Initializes the hls stream I/O with the provided configuration
 *
 * @param[in]   config  Pointer to the HLS IO configuration
 * @param[out]  io      Pointer to the HLS IO handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_hls_init(esp_hls_io_cfg_t *config, esp_gmf_io_handle_t *io);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
