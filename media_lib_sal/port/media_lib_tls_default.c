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

#include <string.h>
#include "esp_log.h"
#include "media_lib_tls_reg.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#ifdef CONFIG_MEDIA_PROTOCOL_LIB_ENABLE

static media_lib_tls_handle_t _tls_new(const char *hostname, int hostlen, int port, const esp_tls_cfg_t *cfg)
{
#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0))
    return esp_tls_conn_new(hostname, strlen(hostname), port, cfg);
#else
    media_lib_tls_handle_t tls = esp_tls_init();
    if (esp_tls_conn_new_sync(hostname, strlen(hostname), port, cfg, (esp_tls_t *)tls) <= 0) {
        esp_tls_conn_delete((esp_tls_t *)tls);
        tls = NULL;
    }
    return tls;
#endif
}

static int _tls_write(media_lib_tls_handle_t tls, const void *data, size_t datalen)
{
    if (tls) {
        return esp_tls_conn_write((esp_tls_t *)tls, data, datalen);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_read(media_lib_tls_handle_t tls, void *data, size_t datalen)
{
    if (tls) {
        return esp_tls_conn_read((esp_tls_t *)tls, data, datalen);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_getsockfd(media_lib_tls_handle_t tls)
{
    if (tls) {
        return ((esp_tls_t *)tls)->sockfd;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_delete(media_lib_tls_handle_t tls)
{
    if (tls) {
        esp_tls_conn_delete((esp_tls_t *)tls);
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static int _tls_get_bytes_avail(media_lib_tls_handle_t tls)
{
    if (tls) {
        return esp_tls_get_bytes_avail((esp_tls_t *)tls);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t media_lib_add_default_tls_adapter(void)
{
    media_lib_tls_t tls_lib = {
        .tls_new = _tls_new,
        .tls_write = _tls_write,
        .tls_read = _tls_read,
        .tls_getsockfd = _tls_getsockfd,
        .tls_delete = _tls_delete,
        .tls_get_bytes_avail = _tls_get_bytes_avail,
    };
    return media_lib_tls_register(&tls_lib);
}
#endif
