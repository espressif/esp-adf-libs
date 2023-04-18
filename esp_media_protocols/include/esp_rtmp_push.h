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
#ifndef RTMP_PUSH_H
#define RTMP_PUSH_H

#include "esp_rtmp_types.h"
#include "media_lib_err.h"
#include "media_lib_os.h"
#include "media_lib_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration information for RTMP push
 */
typedef struct {
    char                  *url;        /*!< Server url format rtmp://ipaddress:port/app_name/stream_name */
    uint32_t               chunk_size; /*!< Maximum chunk size */
    media_lib_thread_cfg_t thread_cfg; /*!< Configuration for receiving data thread */
    media_lib_tls_cfg_t   *ssl_cfg;    /*!< Set when use RTMPS protocol */
    rtmp_event_cb          event_cb;   /*!< Callback for RTMP events */
    void                  *ctx;        /*!< Callback input context */
} rtmp_push_cfg_t;

/**
 * @brief RTMP push instance handle
 */
typedef void *rtmp_push_handle_t;

/**
 * @brief        Open RTMP push instance
 *
 * @param         cfg:  Configuration for RTMP push
 * @return        - NULL: Fail to open push instance
 *                - Others: Push instance handle               
 */
rtmp_push_handle_t esp_rtmp_push_open(rtmp_push_cfg_t *cfg);

/**
 * @brief        Set audio information for RTMP push
 *
 * @param         handle: Push handle
 * @param         audio_info: Audio information
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_NO_MEM: No memory left to store audio information
 */
esp_media_err_t esp_rtmp_push_set_audio_info(rtmp_push_handle_t handle, esp_rtmp_audio_info_t *audio_info);

/**
 * @brief        Set video information for RTMP push
 *
 * @param         handle:  Push handle
 * @param         video_info:  Video information
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_NO_MEM: No memory left to store video information
 */
esp_media_err_t esp_rtmp_push_set_video_info(rtmp_push_handle_t handle, esp_rtmp_video_info_t *video_info);

/**
 * @brief        Connect to RTMP server
 *               This is synchronized call, need to be called after audio and video information set.
 *               This operation can be cancelled by `esp_rtmp_push_close`
 * @param        handle:  Push handle
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_CONNECT_FAIL: Fail to connect to server
 */
esp_media_err_t esp_rtmp_push_connect(rtmp_push_handle_t handle);

/**
 * @brief        Push audio data to RTMP server
 * @param        handle:  Push handle
 * @param        audio_data:  Audio data information
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_WRONG_STATE: Connect to server fail or not finished yet
 *                - ESP_MEDIA_ERR_NO_MEM: Not enough memory to buffer send data
 *                - ESP_MEDIA_ERR_WRITE_DATA: Fail to send data to server
 */
esp_media_err_t esp_rtmp_push_audio(rtmp_push_handle_t handle, esp_rtmp_audio_data_t *audio_data);

/**
 * @brief        Push video data to RTMP server
 * @param        handle:  Push handle
 * @param        video_data:  Video data information
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_WRONG_STATE: Connect to server fail or not finished yet
 *                - ESP_MEDIA_ERR_NO_MEM: Not enough memory to buffer send data
 *                - ESP_MEDIA_ERR_WRITE_DATA: Fail to send data to server
 */
esp_media_err_t esp_rtmp_push_video(rtmp_push_handle_t handle, esp_rtmp_video_data_t *video_data);

/**
 * @brief        Push client send customized command
 *               NOTES: This is none-standard behavior
 *                      It is special used to send command to `esp_rtmp_server`
 *                      Please do not use when use common RTMP server
 * @param        handle: Push handle
 * @param        cmd: Command Data
 * @param        len: Length of command data
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 *                - ESP_MEDIA_ERR_WRITE_DATA: Fail to send data to server
 */
esp_media_err_t esp_rtmp_push_send_command(rtmp_push_handle_t handle, uint8_t *cmd, int len);

/**
 * @brief        Set callback to receive customized command from peer
 *               NOTES: This is none-standard behavior,
 *                      It is special used to get customized command from puller client when use `esp_rtmp_server`
 *                      Please do not use when use common RTMP server
 * @param        handle: Push handle
 * @param        cmd_cb: Command Callback
 * @param        ctx: Input context for callback
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 */
esp_media_err_t esp_rtmp_push_set_command_cb(
                rtmp_push_handle_t handle, 
                int (*cmd_cb)(uint8_t *cmd, int len, void *ctx),
                void *ctx);

/**
 * @brief        Close RTMP push instance
 * @param        handle:  Push handle
 * @return        - ESP_MEDIA_ERR_OK: On success
 *                - ESP_MEDIA_ERR_INVALID_ARG: Invalid argument is wrong
 */
esp_media_err_t esp_rtmp_push_close(rtmp_push_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
