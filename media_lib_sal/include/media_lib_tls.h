/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_TLS_H
#define MEDIA_LIB_TLS_H

#include "media_lib_tls_reg.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Wrapper for create tls client instance
 * @return
 *       - Others  Returned by wrapper function directly
 */
media_lib_tls_handle_t media_lib_tls_new(const char *hostname, int hostlen, int port, const media_lib_tls_cfg_t *cfg);

/**
 * @brief  Wrapper for create tls server instance
 * @return
 *       - Others  Returned by wrapper function directly
 */
media_lib_tls_handle_t media_lib_tls_new_server(int fd, const media_lib_tls_server_cfg_t *cfg);

/**
 * @brief  Wrapper for esp_tls write
 * @return
 *       - Others  Returned by wrapper function directly
 */
int media_lib_tls_write(media_lib_tls_handle_t tls, const void *data, size_t datalen);

/**
 * @brief  Wrapper for esp_tls read
 * @return
 *       - Others  Returned by wrapper function directly
 */
int media_lib_tls_read(media_lib_tls_handle_t tls, void *data, size_t datalen);

/**
 * @brief  Wrapper for esp_tls getsockfd
 * @return
 *       - Others  Returned by wrapper function directly
 */
int media_lib_tls_getsockfd(media_lib_tls_handle_t tls);

/**
 * @brief  Wrapper for esp_tls delete
 * @return
 */
int media_lib_tls_delete(media_lib_tls_handle_t tls);

/**
 * @brief  Wrapper for esp_tls get bytes avail
 * @return
 *       - Others  Returned by wrapper function directly
 */
int media_lib_tls_get_bytes_avail(media_lib_tls_handle_t tls);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_TLS_H */
