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

/**
 * Thread names created by the RTC/SIP stack.
 * Override stack, priority and core through the matching esp_rtc_config_t field.
 * stack_size 0 keeps the protocol default for that thread (stack, priority and core).
 *
 * sip_task:      SIP signaling (connect, REGISTER, INVITE/BYE, OPTIONS/keep-alive)
 * listen_task:   Accept incoming SIP TCP (P2P / TCP server); not created for UDP
 * audio_recv:    RTP audio RX → receive_audio; TX is an esp_timer, not this task
 * video_recv:    RTP video RX / depayload → receive_video; TX is an esp_timer
 */
#define ESP_RTC_THREAD_SIP                 "sip_task"
#define ESP_RTC_THREAD_LISTEN              "listen_task"
#define ESP_RTC_THREAD_AUDIO_RECV          "_rtp_audio_recv"
#define ESP_RTC_THREAD_VIDEO_RECV          "_rtp_video_recv"

#define ESP_RTC_THREAD_SIP_STACK           (10 * 1024)
#define ESP_RTC_THREAD_SIP_PRIO            20
#define ESP_RTC_THREAD_SIP_CORE            0
#define ESP_RTC_THREAD_LISTEN_STACK        (2 * 1024)
#define ESP_RTC_THREAD_LISTEN_PRIO         20
#define ESP_RTC_THREAD_LISTEN_CORE         0
#define ESP_RTC_THREAD_AUDIO_RECV_STACK    (4 * 1024)
#define ESP_RTC_THREAD_AUDIO_RECV_PRIO     20
#define ESP_RTC_THREAD_AUDIO_RECV_CORE     0
#define ESP_RTC_THREAD_VIDEO_RECV_STACK    (3 * 1024)
#define ESP_RTC_THREAD_VIDEO_RECV_PRIO     15
#define ESP_RTC_THREAD_VIDEO_RECV_CORE     1

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
    ESP_RTC_EVENT_CALL_ANSWERED,
    ESP_RTC_EVENT_HANGUP,
    ESP_RTC_EVENT_ERROR,
    ESP_RTC_EVENT_UNREGISTERED,
    ESP_RTC_EVENT_MESSAGE,
    ESP_RTC_EVENT_MESSAGE_SENT,
    ESP_RTC_EVENT_AUDIO_SESSION_BEGIN,
    ESP_RTC_EVENT_AUDIO_SESSION_END,
    ESP_RTC_EVENT_VIDEO_SESSION_BEGIN,
    ESP_RTC_EVENT_VIDEO_SESSION_END,
    ESP_RTC_EVENT_KEEPALIVE,
} esp_rtc_event_t;

/**
 * @brief SRTP negotiation mode
 */
typedef enum
{
    ESP_RTC_SRTP_OFF = 0,  /*!< RTP/AVP only, no a=crypto */
    ESP_RTC_SRTP_PREFER,   /*!< Prefer SDES-SRTP; fall back to cleartext RTP if negotiation fails */
    ESP_RTC_SRTP_REQUIRED, /*!< Require SDES-SRTP; never send cleartext RTP */
} esp_rtc_srtp_mode_t;

/**
 * @brief Call reject reason (valid on ESP_RTC_EVENT_ERROR)
 */
typedef enum
{
    ESP_RTC_REJECT_NONE = 0,
    ESP_RTC_REJECT_SRTP_REQUIRED,  /*!< Peer has no usable crypto, or local SRTP setup failed */
    ESP_RTC_REJECT_SRTP_DOWNGRADE, /*!< re-INVITE removed or weakened crypto */
} esp_rtc_reject_reason_t;

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
 * @brief SIP MESSAGE data
 */
typedef struct
{
    const char *content_type; /*!< Content-Type header, e.g. "text/plain", "application/json" */
    const char *body;         /*!< Message body content */
    int         body_len;     /*!< Body length (0 = auto-calculate with strlen) */
    const char *peer_uri;     /*!< For received: sender URI; For sending: target URI (NULL = use server) */
} esp_rtc_msg_data_t;

/**
 * @brief RTC/SIP worker thread configuration
 *
 *         If stack_size is 0, the protocol default for that thread is used
 *         (stack, priority and core together). If stack_size is non-zero,
 *         priority and core_id are taken as given (core_id 0 is valid).
 */
typedef struct {
    uint16_t                        stack_size;          /*!< Stack in bytes, 0 = protocol default */
    uint8_t                         priority;            /*!< Task priority */
    uint8_t                         core_id;             /*!< CPU core, 0 is a valid core */
} esp_rtc_thread_cfg_t;

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
    bool                        suspend_reg_on_call; /*!< Suspend refresh register on call */
    int                         keepalive;           /*!< Send keep-alive or 'OPTIONS' messages interval in seconds (defaults is 30s) */
    int                         rw_timeout_ms;       /*!< Read/Write transport timeout setting, in milliseconds (defaults to 3s) */
    int                         connect_timeout_ms;  /*!< Connection timeout setting, in milliseconds (defaults to 3s) */
    const char                  *cert_pem;           /*!< SSL server certification, PEM format as string, if the client requires to verify server */
    const char                  *client_cert_pem;    /*!< SSL client certification, PEM format as string, if the server requires to verify client */
    const char                  *client_key_pem;     /*!< SSL client key, PEM format as string, if the server requires to verify client */
    int                         (*crt_bundle_attach)(void *conf);
                                                     /*!< Function pointer to esp_crt_bundle_attach. Enables the use of certification
                                                          bundle for server verification, must be enabled in menuconfig */
    int                         register_interval;   /*!< Registration interval in seconds (defaults is 3600s) */
    int                         aud_frame_size;      /*!< Audio RTP frame buffer size for send and receive */
    const char                  *user_agent;         /*!< Set user agent field (defaults is "ESP32 SIP/2.0") */
    int                         fixed_local_port;    /*!< Set fixed local port (defaults is 0) */
    bool                        p2p_mode;            /*!< When work in P2P mode it will skip register step and do invite or accept invite directly from peer */
    esp_rtc_srtp_mode_t         srtp_mode;           /*!< SRTP mode (default OFF) */
    const char                  *domain;             /*!< Set domain(optional), this domain constructs the host of SIP URIs, supports a single server divided into multiple domains */
    uint8_t                     video_payload_type;  /*!< SDP video payload type */
    const char                  *private_header;     /*!< Set private header since the initial stage */
    esp_rtc_thread_cfg_t        sip_task;            /*!< SIP signaling task: connect, REGISTER, INVITE/BYE, keep-alive.
                                                          stack_size 0 = 10K / prio 20 / core 0 */
    esp_rtc_thread_cfg_t        listen_task;         /*!< SIP TCP accept for incoming P2P/server connections (not used on UDP).
                                                          stack_size 0 = 2K / prio 20 / core 0 */
    esp_rtc_thread_cfg_t        audio_recv;          /*!< RTP audio receive task; calls receive_audio. TX uses an esp_timer.
                                                          stack_size 0 = 4K / prio 20 / core 0 */
    esp_rtc_thread_cfg_t        video_recv;          /*!< RTP video receive/depayload task; calls receive_video. TX uses an esp_timer.
                                                          stack_size 0 = 3K / prio 15 / core 1 */
} esp_rtc_config_t;

/**
 * @brief      Hangup message
 */
typedef struct {
    char *reason;  /*!< Hangup reason */
} esp_rtc_hangup_msg_t;

/**
 * @brief SIP Messages header info
 */
typedef struct {
    char *via;     /*!< SIP Messages via fields  */
    char *from;    /*!< SIP Messages from fields  */
    char *to;      /*!< SIP Messages to fields  */
    char *contact; /*!< SIP Messages contact fields  */
    char *special; /*!< SIP Messages special fields defined by user, 
                        Support multiple fields, last field should not contain "\r\n"
                        Lib will auto add "\r\n" for last field */
} esp_rtc_sip_message_info_t;

/**
 * @brief      Initialize rtc service
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
 * @note       Call from the RTC event callback on `ESP_RTC_EVENT_INCOMING` (or later while the
 *             call is active). The returned pointer refers to internal storage; copy it if needed
 *             beyond the callback. Not thread-safe.
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return     remote peer name, or NULL if unavailable
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
 * @brief      Set custom invite info
 * 
 * @note User can call it multiple times before call `esp_rtc_call`
 *
 * @param[in]  esp_rtc      The rtc handle
 * @param[in]  sip_set_info Set Via, From, To, Contact fields
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 *     - ESP_ERR_INVALID_ARG
 */
int esp_rtc_set_invite_info(esp_rtc_handle_t esp_rtc, const esp_rtc_sip_message_info_t *sip_set_info);

/**
 * @brief      Read incoming sip message
 *
 * @note       Call from the RTC event callback while a call is active.
 *             - Incoming call (UAS): on `ESP_RTC_EVENT_INCOMING`.
 *             - Outgoing call (UAC): on `ESP_RTC_EVENT_CALLING`.
 *             Copy into user buffers in `sip_read_info` before returning from the callback.
 *             Not thread-safe.
 *
 * @param[in]  esp_rtc       The rtc handle
 * @param[in]  sip_read_info Read Via, From, To, Contact fields, we will copy to user buffer.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 *     - ESP_ERR_INVALID_ARG
 */
int esp_rtc_read_incoming_messages(esp_rtc_handle_t esp_rtc, esp_rtc_sip_message_info_t *sip_read_info);

/**
 * @brief      Read raw SIP headers from incoming message
 *
 *             Returns the complete original header text of the current incoming SIP message
 *             (from request/status line up to but not including the blank line before body).
 *             Users can search for any header (e.g. Alert-Info, Call-Info) using strstr/strcasestr.
 *
 * @note       Timing: call this from the RTC event callback while a call is active.
 *             - Incoming call (UAS): call on `ESP_RTC_EVENT_INCOMING` to read the INVITE headers.
 *             - Outgoing call (UAC): call on `ESP_RTC_EVENT_CALLING` to read provisional response headers.
 *             Headers remain valid until the call ends (`ESP_RTC_EVENT_HANGUP`).
 *             `ESP_RTC_EVENT_INCOMING` is reported periodically during ringing; handle auto-answer only once.
 *             Copy into `buf` before returning from the callback. Not thread-safe.
 *
 *             Buffer sizing: pass `buf == NULL` to query the required length without copying.
 *             Then allocate at least `len + 1` bytes (including the null terminator) and call again.
 *             If `buf` is too small, an error is logged and the function returns `-1` without copying.
 *
 * @param[in]  esp_rtc   The rtc handle
 * @param[out] buf       User-allocated buffer to copy raw headers into (NULL to query length only)
 * @param[in]  buf_size  Size of user buffer (including space for null terminator)
 *
 * @return     Length of raw headers on success, or `-1` on error
 */
int esp_rtc_read_raw_headers(esp_rtc_handle_t esp_rtc, char *buf, int buf_size);

/**
 * @brief      Get hangup message when receive `ESP_RTC_EVENT_HANGUP`
 *
 * @note       Call from the RTC event callback on `ESP_RTC_EVENT_HANGUP`.
 *             `msg->reason` points to internal storage; copy it before returning from the callback.
 *             Not thread-safe.
 *
 * @param esp_rtc  The rtc handle
 * @param msg      Hangup message
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 */
int esp_rtc_get_hangup_msg(esp_rtc_handle_t esp_rtc, esp_rtc_hangup_msg_t *msg);

/**
 * @brief      Deinitialize rtc service
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

/**
 * @brief      Send an out-of-dialog SIP MESSAGE request
 *
 *             Sends instant text or application data to a peer or the configured SIP server
 *             without establishing a voice/video call (SIP MESSAGE method).
 *             Completion is reported via `ESP_RTC_EVENT_MESSAGE_SENT`. To receive messages,
 *             handle `ESP_RTC_EVENT_MESSAGE` and call `esp_rtc_get_message`.
 *
 *             Typical use: P2P text chat, IoT or control payloads (e.g. `application/json`),
 *             or replying to an incoming MESSAGE while registered or during a call.
 *
 * @note       May be called from any task (not limited to the event callback).
 *             `content_type`, `body`, and `peer_uri` are copied internally; the pointers in
 *             `msg_data` need only be valid for the duration of this call.
 *             Sending is asynchronous: the SIP task performs the actual request later.
 *             Do not call again until `ESP_RTC_EVENT_MESSAGE_SENT` is reported for the
 *             previous send; overlapping calls are not thread-safe.
 *
 * @param[in]  esp_rtc    The rtc handle
 * @param[in]  msg_data   Message to send (`peer_uri` NULL uses the configured server)
 *
 * @return
 *     - ESP_OK on success (message queued)
 *     - ESP_ERR_INVALID_ARG on wrong handle or missing fields
 *     - ESP_ERR_INVALID_STATE if not registered or in an invalid state
 */
int esp_rtc_send_message(esp_rtc_handle_t esp_rtc, const esp_rtc_msg_data_t *msg_data);

/**
 * @brief      Get received SIP MESSAGE
 *
 * @note       Call from the RTC event callback on `ESP_RTC_EVENT_MESSAGE`.
 *             `content_type`, `body`, and `peer_uri` in `msg_data` point to internal storage;
 *             copy them before returning from the callback. Not thread-safe.
 *
 * @param[in]  esp_rtc    The rtc handle
 * @param[out] msg_data   Received message fields
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on wrong handle
 *     - ESP_ERR_INVALID_STATE if no message is pending
 */
int esp_rtc_get_message(esp_rtc_handle_t esp_rtc, esp_rtc_msg_data_t *msg_data);

/**
 * @brief      Query whether the current session negotiated SRTP
 *
 * @note       Call after `ESP_RTC_EVENT_CALL_ANSWERED` or `ESP_RTC_EVENT_AUDIO_SESSION_BEGIN`.
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return     true if SRTP protect/unprotect is active, false otherwise
 */
bool esp_rtc_is_srtp_active(esp_rtc_handle_t esp_rtc);

/**
 * @brief      Get the last call reject reason
 *
 * @note       Call from the RTC event callback on `ESP_RTC_EVENT_ERROR`.
 *
 * @param[in]  esp_rtc  The rtc handle
 *
 * @return     Reject reason, or `ESP_RTC_REJECT_NONE`
 */
esp_rtc_reject_reason_t esp_rtc_get_reject_reason(esp_rtc_handle_t esp_rtc);

#ifdef __cplusplus
}
#endif

#endif
