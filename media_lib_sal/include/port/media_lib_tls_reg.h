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
#ifndef MEDIA_LIB_TLS_REG_H
#define MEDIA_LIB_TLS_REG_H

#include "esp_err.h"
#include "esp_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* media_lib_tls_handle_t;
typedef media_lib_tls_handle_t (*__media_lib_tls_new)(const char *hostname, int hostlen, int port, const esp_tls_cfg_t *cfg);
typedef int (*__media_lib_tls_write)(media_lib_tls_handle_t tls, const void *data, size_t datalen);
typedef int (*__media_lib_tls_read)(media_lib_tls_handle_t tls, void *data, size_t datalen);
typedef int (*__media_lib_tls_getsockfd)(media_lib_tls_handle_t tls);
typedef int (*__media_lib_tls_delete)(media_lib_tls_handle_t tls);
typedef int (*__media_lib_tls_get_bytes_avail)(media_lib_tls_handle_t tls);

typedef struct {
    __media_lib_tls_new             tls_new;                    /*!< tls lib new */
    __media_lib_tls_write           tls_write;                  /*!< tls lib write */
    __media_lib_tls_read            tls_read;                   /*!< tls lib read */
    __media_lib_tls_getsockfd       tls_getsockfd;              /*!< tls lib getsockfd */
    __media_lib_tls_delete          tls_delete;                 /*!< tls lib delete */
    __media_lib_tls_get_bytes_avail tls_get_bytes_avail;        /*!< tls lib get bytes avial */
} media_lib_tls_t;

/**
 * @brief     Register tls related wrapper functions for media library
 *
 * @param      tls_lib  tls wrapper function lists
 *
* @return
*             - ESP_OK: on success
*             - ESP_ERR_INVALID_ARG: some members of tls lib not set
*/
esp_err_t media_lib_tls_register(media_lib_tls_t *tls_lib);

#ifdef __cplusplus
}
#endif

#endif
