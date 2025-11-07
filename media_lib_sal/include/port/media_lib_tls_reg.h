/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_TLS_REG_H
#define MEDIA_LIB_TLS_REG_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    const char  *cacert_buf;               /*!< Certificate Authority's certificate in a buffer */
    int          cacert_bytes;             /*!< Size of Certificate Authority certificate */
    const char  *clientcert_buf;           /*!< Client certificate legacy name */
    int          clientcert_bytes;         /*!< Size of client certificate legacy name */
    const char  *clientkey_buf;            /*!< Client key legacy name */
    int          clientkey_bytes;          /*!< Size of client key legacy name */
    const char  *clientkey_password;       /*!< Client key decryption password string */
    int          clientkey_password_len;   /*!< String length of the password */
    bool         non_block;                /*!< Configure non-blocking mode */
    bool         use_secure_element;       /*!< Enable this option to use secure element */
    int          timeout_ms;               /*!< Network timeout in milliseconds */
    bool         use_global_ca_store;      /*!< Use a global ca_store for all the connections */
    bool         skip_common_name;         /*!< Skip any validation of server certificate CN field */
    int (*crt_bundle_attach)(void *conf);  /*!< Function pointer to esp_crt_bundle_attach */
} media_lib_tls_cfg_t;

typedef struct {
    const char  *cacert_buf;              /*!< Client CA certificate in a buffer */
    int          cacert_bytes;            /*!< Size of client CA certificate */
    const char  *servercert_buf;          /*!< Server certificate in a buffer */
    int          servercert_bytes;        /*!< Size of server certificate */
    const char  *serverkey_buf;           /*!< Server key in a buffer */
    int          serverkey_bytes;         /*!< Size of server key */
    const char  *serverkey_password;      /*!< Server key decryption password string */
    int          serverkey_password_len;  /*!< String length of the password */
} media_lib_tls_server_cfg_t;

typedef void *media_lib_tls_handle_t;
typedef media_lib_tls_handle_t (*__media_lib_tls_new)(const char *hostname, int hostlen, int port,
                                                      const media_lib_tls_cfg_t *cfg);
typedef media_lib_tls_handle_t (*__media_lib_tls_new_server)(int sock_fd, const media_lib_tls_server_cfg_t *cfg);
typedef int (*__media_lib_tls_write)(media_lib_tls_handle_t tls, const void *data, size_t datalen);
typedef int (*__media_lib_tls_read)(media_lib_tls_handle_t tls, void *data, size_t datalen);
typedef int (*__media_lib_tls_getsockfd)(media_lib_tls_handle_t tls);
typedef int (*__media_lib_tls_delete)(media_lib_tls_handle_t tls);
typedef int (*__media_lib_tls_get_bytes_avail)(media_lib_tls_handle_t tls);

typedef struct {
    __media_lib_tls_new              tls_new;              /*!< Tls lib new */
    __media_lib_tls_new_server       tls_new_server;       /*!< Tls lib new server */
    __media_lib_tls_write            tls_write;            /*!< Tls lib write */
    __media_lib_tls_read             tls_read;             /*!< Tls lib read */
    __media_lib_tls_getsockfd        tls_getsockfd;        /*!< Tls lib getsockfd */
    __media_lib_tls_delete           tls_delete;           /*!< Tls lib delete */
    __media_lib_tls_get_bytes_avail  tls_get_bytes_avail;  /*!< Tls lib get bytes avail */
} media_lib_tls_t;

/**
 * @brief  Register tls related wrapper functions for media library
 *
 * @param  tls_lib  Tls wrapper function lists
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Some members of tls lib not set
 */
esp_err_t media_lib_tls_register(media_lib_tls_t *tls_lib);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_TLS_REG_H */
