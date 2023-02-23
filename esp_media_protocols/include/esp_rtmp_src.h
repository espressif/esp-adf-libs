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

#include "media_lib_err.h"
#include "media_lib_os.h"
#include "media_lib_tls.h"
#include "esp_rtmp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTMP source stream information
 */
typedef struct {
    esp_rtmp_stream_type_t stream_type;
    union {
        esp_rtmp_audio_info_t audio_info;
        esp_rtmp_video_info_t video_info;
    };
} rtmp_src_stream_info_t;

/**
 * @brief RTMP source stream data
 */
typedef struct {
    esp_rtmp_stream_type_t stream_type;
    union {
        esp_rtmp_audio_data_t audio_data;
        esp_rtmp_video_data_t video_data;
    };
} rtmp_src_stream_data_t;

/**
 * @brief RTMP source stream callback
 */
typedef int (*rtmp_src_stream_cb)(rtmp_src_stream_info_t *stream_info, void *ctx);

/**
 * @brief RTMP source data callback
 */
typedef int (*rtmp_src_data_cb)(rtmp_src_stream_data_t *stream_data, void *ctx);

/**
 * @brief RTMP source configuration
 *        NOTES: RTMP support two output mode
 *        1: When `fifo_size` is set, it will output FLV format.
 *           Users can use API `esp_rtmp_src_read` to get output data.
 *        2: When `stream_cb` and `data_cb` is set, it will not mux audio and video data.
 *           Users can get stream information and stream data from callback directly.
 */
typedef struct {
    char                  *url;        /*!< Server url, format: rtmp://ipaddress:port/app_name/stream_name */
    uint32_t               chunk_size; /*!< Maximum chunk size */
    media_lib_thread_cfg_t thread_cfg; /*!< Configuration for receiving data thread */
    media_lib_tls_cfg_t   *ssl_cfg;    /*!< Set when use RTMPS protocol */
    uint32_t               fifo_size;  /*!< Ringfifo size to cache input media data, when output format is FLV */
    rtmp_src_stream_cb     stream_cb;  /*!< Callback to receive stream information */
    rtmp_src_data_cb       data_cb;    /*!< Callback to receive stream data */
    rtmp_event_cb          event_cb;   /*!< Callback for RTMP events */
    void                  *ctx;        /*!< Callback input context */
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
 * @brief         Set whether to receive media data
 *                NOTES: When set to true, it starts to receive audio and video data including metadata from scratch
 *                       When set to false, media data will not be received, connection and command receiving are kept
 * @param         handle:  Source handle
 * @param         allow:  Allow media data input or not
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid source handle
 *                - ESP_MEDIA_ERR_CONNECT_FAIL: Fail to connect to server
 */
esp_media_err_t esp_rtmp_src_receive_media(rtmp_src_handle_t handle, bool allow);

/**
 * @brief         Read data from RTMP server
 *                This API is synchronized call
 *                It will wait for all data received or return back when meet error
 * @param         handle:  Source handle
 * @param         data:  Data pointer to read
 * @param         size:  Data size to read
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid source handle
 *                - ESP_MEDIA_ERR_NOT_SUPPORT: Not support if `stream_cb` is set
 *                - ESP_MEDIA_ERR_RESET: Peer closed need read and parse again from scratch
 *                - ESP_MEDIA_ERR_READ_DATA: Read data fail
 */
esp_media_err_t esp_rtmp_src_read(rtmp_src_handle_t handle, uint8_t *data, int size);

/**
 * @brief        Send customized command to Pusher
 * @param        handle: Push handle
 * @param        cmd: Command Data
 * @param        len: Length of command data
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_WRITE_DATA: Fail to send data to server
 */
esp_media_err_t esp_rtmp_src_send_command(rtmp_src_handle_t handle, uint8_t *cmd, int len);

/**
 * @brief        Set callback to receive customized command from peer
 * @param        handle: Push handle
 * @param        cmd_cb: Command callback
 * @param        ctx: Input context for callback
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 */
esp_media_err_t esp_rtmp_src_set_command_cb(
                rtmp_src_handle_t handle, 
                int (*cmd_cb)(uint8_t *cmd, int len, void *ctx),
                void *ctx);

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
