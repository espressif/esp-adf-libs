/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_log.h"
#include "media_lib_tls_reg.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "esp_tls.h"
#include "esp_log.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#ifdef CONFIG_MEDIA_LIB_TLS_ENABLE

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#define esp_tls_conn_delete esp_tls_conn_destroy
#endif

#define TAG "TLS_Lib"
typedef struct {
    esp_tls_t* tls;
    bool       is_server;
} media_lib_tls_inst_t;

static media_lib_tls_handle_t _tls_new(const char *hostname, int hostlen, int port, const media_lib_tls_cfg_t *cfg)
{
    esp_tls_cfg_t tls_cfg = {
        .cacert_buf = (const unsigned char *)cfg->cacert_buf,
        .cacert_bytes = (unsigned int)cfg->cacert_bytes,
        .clientcert_buf = (const unsigned char *)cfg->clientcert_buf,
        .clientcert_bytes = (unsigned int)cfg->clientcert_bytes,
        .clientkey_buf = (const unsigned char *)cfg->clientkey_buf,
        .clientkey_bytes = (unsigned int)cfg->clientkey_bytes,
        .clientkey_password = (const unsigned char *)cfg->clientkey_password,
        .clientkey_password_len = (unsigned int)cfg->clientkey_password_len,
        .non_block = cfg->non_block,
        .timeout_ms = cfg->timeout_ms,
        .use_global_ca_store = cfg->use_global_ca_store,
        .skip_common_name = cfg->skip_common_name,
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0))
        .use_secure_element = cfg->use_secure_element,
        .crt_bundle_attach = cfg->crt_bundle_attach,
#endif
    };
    media_lib_tls_inst_t * tls_lib = calloc(1, sizeof(media_lib_tls_inst_t));
    if (tls_lib == NULL) {
        ESP_LOGE(TAG, "No memory for instance");
        return NULL;
    }
#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0))
    esp_tls_t * tls = esp_tls_conn_new(hostname, strlen(hostname), port, &tls_cfg);
    if (tls == NULL) {
        ESP_LOGE(TAG, "Fail to connect client");
        free(tls_lib);
        return NULL;
    }
#else
    esp_tls_t* tls = esp_tls_init();
    if (tls == NULL ||
        esp_tls_conn_new_sync(hostname, strlen(hostname), port, &tls_cfg, tls) < 0) {
        esp_tls_conn_delete(tls);
        free(tls_lib);
        ESP_LOGE(TAG, "Fail to connect client");
        return NULL;
    }
#endif
    tls_lib->tls = tls;
    return (media_lib_tls_handle_t)tls_lib;
}

static media_lib_tls_handle_t _tls_new_server(int fd, const media_lib_tls_server_cfg_t *cfg)
{
#ifndef CONFIG_ESP_TLS_SERVER
    ESP_LOGE(TAG, "Please enable macro CONFIG_ESP_TLS_SERVER");
    return NULL;
#else
    esp_tls_cfg_server_t server_cfg = {
        .cacert_buf = (const unsigned char *)cfg->cacert_buf,
        .cacert_bytes = (unsigned int)cfg->cacert_bytes,
        .servercert_buf = (const unsigned char *)cfg->servercert_buf,
        .servercert_bytes = (unsigned int)cfg->servercert_bytes,
        .serverkey_buf = (const unsigned char *)cfg->serverkey_buf,
        .serverkey_bytes = (unsigned int)cfg->serverkey_bytes,
        .serverkey_password = (const unsigned char *)cfg->serverkey_password,
        .serverkey_password_len = (unsigned int)cfg->serverkey_password_len,
    };
    media_lib_tls_inst_t *tls_lib = calloc(1, sizeof(media_lib_tls_inst_t));
    if (tls_lib == NULL) {
        ESP_LOGE(TAG, "No memory for instance");
        return NULL;
    }
    esp_tls_t* tls = esp_tls_init();
    if (tls == NULL ||
        esp_tls_server_session_create(&server_cfg, fd, tls) < 0) {
        ESP_LOGE(TAG, "Fail to create server session");
        if (tls) { 
            free(tls);
        }
        free(tls_lib);
        return NULL;
    }
    tls_lib->tls = tls;
    tls_lib->is_server = true;
    return (media_lib_tls_handle_t)tls_lib;
#endif
}

static int _tls_write(media_lib_tls_handle_t tls, const void *data, size_t datalen)
{
    if (tls) {
        media_lib_tls_inst_t *tls_lib = (media_lib_tls_inst_t *)tls;
        return esp_tls_conn_write(tls_lib->tls, data, datalen);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_read(media_lib_tls_handle_t tls, void *data, size_t datalen)
{
    if (tls) {
        media_lib_tls_inst_t *tls_lib = (media_lib_tls_inst_t *)tls;
        return esp_tls_conn_read(tls_lib->tls, data, datalen);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_getsockfd(media_lib_tls_handle_t tls)
{
    if (tls) {
        media_lib_tls_inst_t *tls_lib = (media_lib_tls_inst_t *)tls;
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
        int sock_fd = -1;
        esp_tls_get_conn_sockfd(tls_lib->tls, &sock_fd);
        return sock_fd;
#else
        return tls_lib->tls->sockfd;
#endif
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

static int _tls_delete(media_lib_tls_handle_t tls)
{
    if (tls) {
        media_lib_tls_inst_t *tls_lib = (media_lib_tls_inst_t *)tls;
        if (tls_lib->is_server) {
#ifdef CONFIG_ESP_TLS_SERVER
            esp_tls_server_session_delete(tls_lib->tls);
#endif
        } else {
            esp_tls_conn_delete(tls_lib->tls);
        }
        free(tls);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static int _tls_get_bytes_avail(media_lib_tls_handle_t tls)
{
    if (tls) {
        media_lib_tls_inst_t *tls_lib = (media_lib_tls_inst_t *)tls;
        return esp_tls_get_bytes_avail(tls_lib->tls);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t media_lib_add_default_tls_adapter(void)
{
    media_lib_tls_t tls_lib = {
        .tls_new = _tls_new,
        .tls_new_server = _tls_new_server,
        .tls_write = _tls_write,
        .tls_read = _tls_read,
        .tls_getsockfd = _tls_getsockfd,
        .tls_delete = _tls_delete,
        .tls_get_bytes_avail = _tls_get_bytes_avail,
    };
    return media_lib_tls_register(&tls_lib);
}
#endif
