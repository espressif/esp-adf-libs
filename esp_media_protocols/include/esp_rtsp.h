/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in
 * which case, it is free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef _ESP_RTSP_H_
#define _ESP_RTSP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _esp_rtsp *esp_rtsp_handle_t;

/**
 * @brief RTSP codec type
 */
typedef enum {
    RTSP_ACODEC_G711A,
    RTSP_ACODEC_G711U,
    RTSP_VCODEC_MJPEG,
    RTSP_VCODEC_H264,
} rtsp_payload_codec_t;

/**
 * @brief RTSP client transport type
 */
typedef enum {
    RTSP_TRANSPORT_UDP,
    RTSP_TRANSPORT_TCP,
} esp_rtsp_transport_t;

/**
 * @brief RTSP mode
 */
typedef enum {
    RTSP_SERVER,
    RTSP_CLIENT_PUSH,
    RTSP_CLIENT_PLAY,
} esp_rtsp_mode_t;

/**
 * @brief RTSP session state
 */
typedef enum {
    RTSP_STATE_NONE,
    RTSP_STATE_OPTIONS,
    RTSP_STATE_ANNOUNCE,
    RTSP_STATE_SETUP,
    RTSP_STATE_DESCRIBE,
    RTSP_STATE_PLAY,
    RTSP_STATE_RECORD,
    RTSP_STATE_TEARDOWN,
} esp_rtsp_state_t;

typedef int (*esp_rtsp_state_handler)(esp_rtsp_state_t event, void *ctx);
typedef int (*__esp_rtsp_send_audio)(unsigned char *data, int len, void *ctx);
typedef int (*__esp_rtsp_receive_audio)(unsigned char *data, int len, void *ctx);
typedef int (*__esp_rtsp_send_video)(unsigned char *data, unsigned int *len, void *ctx);
typedef int (*__esp_rtsp_receive_video)(unsigned char *data, int len, void *ctx);

/**
 * @brief RTSP session data callback
 */
typedef struct {
    __esp_rtsp_send_audio        send_audio;
    __esp_rtsp_receive_audio     receive_audio;
    __esp_rtsp_send_video        send_video;
    __esp_rtsp_receive_video     receive_video;
} esp_rtsp_data_cb_t;

/**
 * @brief ESP RTSP video info
 */
typedef struct {
    rtsp_payload_codec_t    vcodec;     /*!< Video codec type*/
    int                     width;      /*!< Video width */
    int                     height;     /*!< Video height */
    int                     fps;        /*!< Video maximum fps */
    int                     len;        /*!< Video frame maximum length */
} esp_rtsp_video_info_t;

/**
 * @brief ESP RTSP session configurations
 */
typedef struct {
    void                    *ctx;           /*!< RTSP session user context */
    bool                    video_enable;   /*!< Enable video */
    bool                    audio_enable;   /*!< Enable audio */
    const char              *uri;           /*!< Client push/play uri */
    int                     local_port;     /*!< Local server port */
    const char              *local_addr;    /*!< Local address */
    int                     stack_size;     /*!< Task stack size */
    int                     task_prio;      /*!< Task priority */
    rtsp_payload_codec_t    acodec;         /*!< Audio codec */
    esp_rtsp_mode_t         mode;           /*!< Set server or PUSH/PLAY mode */
    esp_rtsp_video_info_t   *video_info;    /*!< Video info */
    esp_rtsp_data_cb_t      *data_cb;       /*!< Data callback */
    esp_rtsp_state_handler  state;          /*!< State handler */
    esp_rtsp_transport_t    trans;          /*!< Client default transport */
} esp_rtsp_config_t;

/**
 * @brief      ESP rtsp server start
 *
 * @param[in]  config   The ESP rtsp configuration
 *
 * @return     The rtsp handle if successfully started, NULL on error
 */
esp_rtsp_handle_t esp_rtsp_server_start(esp_rtsp_config_t *config);

/**
 * @brief      Stop rtsp server
 *
 * @param[in]  rtsp   The rtsp handle
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtsp_server_stop(esp_rtsp_handle_t rtsp);

/**
 * @brief      ESP rtsp client start
 *
 * @param[in]  config   The ESP rtsp configuration
 *
 * @return     The rtsp handle if successfully started, NULL on error
 */
esp_rtsp_handle_t esp_rtsp_client_start(esp_rtsp_config_t *config);

/**
 * @brief      Stop rtsp client
 *
 * @param[in]  rtsp   The rtsp handle
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtsp_client_stop(esp_rtsp_handle_t rtsp);

#ifdef __cplusplus
}
#endif

#endif
