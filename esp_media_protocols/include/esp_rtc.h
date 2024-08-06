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

#ifndef _ESP_RTC_H_
#define _ESP_RTC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _esp_rtc_handle *esp_rtc_handle_t;

/**
 * @brief RTC audio codec type
 */
typedef enum {
    RTC_ACODEC_NULL,
    RTC_ACODEC_G711A,
    RTC_ACODEC_G711U,
    RTC_ACODEC_OPUS,
} rtc_payload_acodec_t;

/**
 * @brief RTC video codec type
 */
typedef enum {
    RTC_VCODEC_NULL,
    RTC_VCODEC_MJPEG,
    RTC_VCODEC_H264,
} rtc_payload_vcodec_t;

/**
 * @brief RTC session event
 */
typedef enum {
    ESP_RTC_EVENT_NULL = 0,
    ESP_RTC_EVENT_REGISTERED,
    ESP_RTC_EVENT_INCOMING,
    ESP_RTC_EVENT_CALLING,
    ESP_RTC_EVENT_HANGUP,
    ESP_RTC_EVENT_ERROR,
    ESP_RTC_EVENT_UNREGISTERED,
    ESP_RTC_EVENT_AUDIO_SESSION_BEGIN,
    ESP_RTC_EVENT_AUDIO_SESSION_END,
    ESP_RTC_EVENT_VIDEO_SESSION_BEGIN,
    ESP_RTC_EVENT_VIDEO_SESSION_END,
} esp_rtc_event_t;

typedef int (*esp_rtc_event_handle)(esp_rtc_event_t event, void *ctx);
typedef int (*__esp_rtc_send_audio)(unsigned char *data, int len, void *ctx);
typedef int (*__esp_rtc_receive_audio)(unsigned char *data, int len, void *ctx);
typedef int (*__esp_rtc_send_video)(unsigned char *data, unsigned int *len, void *ctx);
typedef int (*__esp_rtc_receive_video)(unsigned char *data, int len, void *ctx);
typedef int (*__esp_rtc_receive_dtmf)(unsigned char *data, int len, void *ctx);

/**
 * @brief RTC session data callback
 *
 * @note When a DTMF (RFC2833) event is received, it will be returned through the audio data channel in the format of "DTMF-ID".
 */
typedef struct {
    __esp_rtc_send_audio        send_audio;
    __esp_rtc_receive_audio     receive_audio;
    __esp_rtc_send_video        send_video;
    __esp_rtc_receive_video     receive_video;
    __esp_rtc_receive_dtmf      receive_dtmf;
} esp_rtc_data_cb_t;

/**
 * @brief ESP RTC video info
 */
typedef struct {
    rtc_payload_vcodec_t    vcodec;     /*!< Video codec type*/
    int                     width;      /*!< Video width */
    int                     height;     /*!< Video height */
    int                     fps;        /*!< Video fps */
    int                     len;        /*!< Video length */
} esp_rtc_video_info_t;

/**
 * @brief RTC session configurations
 */
typedef struct {
    void                        *ctx;                /*!< RTC session user context */
    const char                  *local_addr;         /*!< Local address */
    const char                  *uri;                /*!< "Transport://user:pass@server:port/path" */
    rtc_payload_acodec_t        acodec_type;         /*!< Audio codec type */
    esp_rtc_video_info_t        *vcodec_info;        /*!< Video codec info */
    esp_rtc_data_cb_t           *data_cb;            /*!< RTC data callback */
    esp_rtc_event_handle        event_handler;       /*!< RTC session event handler */
    bool                        use_public_addr;     /*!< Use the public IP address returned by the server (RFC3581) */
    bool                        send_options;        /*!< Use 'OPTIONS' messages replace keep-alive to server for keep NAT hole opened */
    int                         keepalive;           /*!< Send keep-alive or 'OPTIONS' messages interval in seconds (defaults is 30s) */
    const char                  *cert_pem;           /*!< SSL server certification, PEM format as string, if the client requires to verify server */
    const char                  *client_cert_pem;    /*!< SSL client certification, PEM format as string, if the server requires to verify client */
    const char                  *client_key_pem;     /*!< SSL client key, PEM format as string, if the server requires to verify client */
    int                         (*crt_bundle_attach)(void *conf);
                                                     /*!< Function pointer to esp_crt_bundle_attach. Enables the use of certification
                                                          bundle for server verification, must be enabled in menuconfig */
    int                         register_interval;   /*!< Registration interval in seconds (defaults is 3600s) */
    const char                  *user_agent;         /*!< Set user agent field (defaults is "ESP32 SIP/2.0") */
} esp_rtc_config_t;

/**
 * @brief      Intialize rtc service
 *
 * @param[in]  config   The rtc configuration
 *
 * @return     The rtc handle if successfully created, NULL on error
 */
esp_rtc_handle_t esp_rtc_service_init(esp_rtc_config_t *config);

/**
 * @brief      Start a rtc session
 *
 * @param[in]  esp_rtc      The rtc handle
 * @param[in]  remote_user  Remote user id
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_STATE on wrong rtc state
 *     - ESP_ERR_INVALID_ARG on wrong handle
 *     - ESP_ERR_NO_MEM on not enough memory
 */
int esp_rtc_call(esp_rtc_handle_t esp_rtc, const char *remote_user);

/**
 * @brief      Answer the rtc session
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_STATE on wrong rtc state
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_answer(esp_rtc_handle_t esp_rtc);

/**
 * @brief      Get rtc session peer name
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return     remote peer name
 */
const char *esp_rtc_get_peer(esp_rtc_handle_t esp_rtc);

/**
 * @brief      Hang up or cancel
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_STATE on wrong rtc state
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_bye(esp_rtc_handle_t esp_rtc);

/**
 * @brief      Deintialize rtc service
 *
 * @param[in]  esp_rtc   The rtc handle
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_service_deinit(esp_rtc_handle_t esp_rtc);

/**
 * @brief      Send DTMF event ( Only support out band method (RFC2833) )
 *
 * @param[in]  esp_rtc      The rtc handle
 * @param[in]  dtmf_event   DTMF event ID (0-15)
 * @param[in]  volume       Tone volume
 * @param[in]  duration     Tone last duration (unit ms)
 * 
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_send_dtmf(esp_rtc_handle_t esp_rtc, uint8_t dtmf_event, uint8_t volume, uint16_t duration);

/**
 * @brief      Set private header
 *
 * @param[in]  esp_rtc      The rtc handle
 * @param[in]  pheader      Private header data
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_set_private_header(esp_rtc_handle_t esp_rtc, const char *pheader);

#ifdef __cplusplus
}
#endif

#endif
