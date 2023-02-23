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
#ifndef RTMP_SERVER_H
#define RTMP_SERVER_H

#include <stdint.h>
#include "media_lib_err.h"
#include "media_lib_os.h"
#include "media_lib_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Authorize callback use stream key
 * @param         stream_key: Customized stream key like: stream?userid=a&password=a
 * @param         ctx: Input context
 * @return        - true: Allow to do futher transmission
 *                - false: Resist client to do futher action
 */
typedef bool (*rtmp_server_auth_cb) (char *stream_key, void * ctx);

/**
 * @brief RTMP server configuration
 */
typedef struct {
    uint32_t                     chunk_size;        /*!< Maximum chunk size */
    char                        *app_name;          /*!< Application name */
    uint16_t                     port;              /*!< Listen on port */
    uint8_t                      max_clients;       /*!< Limit of maximum client number */
    uint32_t                     client_cache_size; /*!< Maximum cache size for client send buffer, if not provided default use 160KB */
    media_lib_thread_cfg_t       thread_cfg;        /*!< Configuration for receiving data and connecting thread */
    rtmp_server_auth_cb          auth_cb;           /*!< Callback for client authorize, if not provided treated as allowed */
    void                        *ctx;               /*!< Input Context */
    media_lib_tls_server_cfg_t  *ssl_cfg;           /*!< Set when use RTMPS protocol */
    bool                         keep_src;          /*!< Keep source when Pusher closed */
} rtmp_server_cfg_t;

/**
 * @brief RTMP server handle
 */
typedef void *rtmp_server_handle_t;

/**
 * @brief RTMP puller active callback
 */
typedef int (*rtmp_server_puller_active_cb)(char *stream_name, bool active, void *ctx);

/**
 * @brief         Open RTMP server
 *                For resource on board is limited, please don't connect too many clients.
 *                When `max_clients` is not assigned, default value will set to 2.
 *                Currently server only support decode and encode AMF0 type, please use AMF0 for RTMP command.
 * @param         cfg:  Configuration for RTMP server
 * @return        - NULL: Fail to open instance
 *                - Others: RTMP server instance handle                
 */
rtmp_server_handle_t esp_rtmp_server_open(rtmp_server_cfg_t *cfg);

/**
 * @brief        Setup RTMP server
 * @param        server:  RTMP server handle
 * @return       - ESP_MEDIA_ERR_OK: On success
 *               - ESP_MEDIA_ERR_INVALID_ARG: Input wrong handle
 *               - ESP_MEDIA_ERR_CONNECT_FAIL: Fail to bind on port
 *               - ESP_MEDIA_ERR_FAIL: Fail to do server setup
 */
esp_media_err_t esp_rtmp_server_setup(rtmp_server_handle_t server);

/**
 * @brief        Query status of server, for debug only
 * @param        server:  RTMP server handle
 * @return       - ESP_MEDIA_ERR_OK: On success
 *               - ESP_MEDIA_ERR_INVALID_ARG: Wrong server handle
 */
esp_media_err_t esp_rtmp_server_query(rtmp_server_handle_t server);

/**
 * @brief        Set puller active callback
 *               NOTES: It is specially used when use local pusher
 *                      So that when no puller connected in local pusher can stop pushing data
 * @param        server:  RTMP server handle
 * @return       - ESP_MEDIA_ERR_OK: On success
 *               - ESP_MEDIA_ERR_INVALID_ARG: Wrong server handle
 */
esp_media_err_t esp_rtmp_server_monitor_puller(rtmp_server_handle_t server, rtmp_server_puller_active_cb cb);

/**
 * @brief        Close RTMP server
 * @param        server:  RTMP server handle
 * @return       - ESP_MEDIA_ERR_OK: On success
 *               - ESP_MEDIA_ERR_INVALID_ARG: Wrong server handle
 */
esp_media_err_t esp_rtmp_server_close(rtmp_server_handle_t server);

#ifdef __cplusplus
}
#endif

#endif
