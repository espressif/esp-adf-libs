/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2023 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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
#ifndef ESP_RTMP_TYPES_H
#define ESP_RTMP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTMP stream type
 */
typedef enum {
    RTMP_STREAM_TYPE_NONE,
    RTMP_STREAM_TYPE_AUDIO,
    RTMP_STREAM_TYPE_VIDEO,
} esp_rtmp_stream_type_t;

/**
 * @brief RTMP video codec type
 */
typedef enum {
    RTMP_VIDEO_CODEC_NONE,  /*!< Invalid video type */
    RTMP_VIDEO_CODEC_H264,  /*!< H264 video type */
    RTMP_VIDEO_CODEC_MJPEG, /*!< MJPEG video type */
} esp_rtmp_video_codec_t;

/**
 * @brief RTMP audio codec type
 */
typedef enum {
    RTMP_AUDIO_CODEC_NONE,  /*!< Invalid audio type */
    RTMP_AUDIO_CODEC_AAC,   /*!< AAC audio type */
    RTMP_AUDIO_CODEC_MP3,   /*!< MP3 audio type */
    RTMP_AUDIO_CODEC_PCM,   /*!< PCM audio type */
    RTMP_AUDIO_CODEC_G711A, /*!< G711 alaw audio type */
    RTMP_AUDIO_CODEC_G711U, /*!< G711 ulaw audio type */
} esp_rtmp_audio_codec_t;

/**
 * @brief Audio information for RTMP
 */
typedef struct {
    esp_rtmp_audio_codec_t codec;           /*!< Audio codec type */
    uint8_t                channel;         /*!< Audio channel */
    uint8_t                bits_per_sample; /*!< Audio bits per sample */
    uint32_t               sample_rate;     /*!< Audio sample rate */
    void                  *codec_spec_info; /*!< Audio codec specified information */
    int                    spec_info_len;   /*!< Audio codec specified information length */
} esp_rtmp_audio_info_t;

/**
 * @brief Video information for RTMP
 */
typedef struct {
    void                  *codec_spec_info; /*!< Video codec specified info, 
                                                 For H264 need to provide SPS and PPS information*/
    int                    spec_info_len;   /*!< Video codec specified information length */
    esp_rtmp_video_codec_t codec;           /*!< Video codec type */
    uint16_t               width;           /*!< Video width */
    uint16_t               height;          /*!< Video height */
    uint8_t                fps;             /*!< Video framerate per second */
} esp_rtmp_video_info_t;

/**
 * @brief Audio data for RTMP
 */
typedef struct {
    uint32_t pts;  /*!< PTS of audio data */
    uint8_t* data; /*!< Audio data pointer */
    uint32_t size; /*!< Audio data size */
    bool     eos;  /*!< End of stream data*/
} esp_rtmp_audio_data_t;

/**
 * @brief Video data for RTMP
 */
typedef struct {
    uint32_t pts;       /*!< PTS of video data */
    bool     key_frame; /*!< Whether it is key frame */
    uint8_t *data;      /*!< Video data pointer */
    uint32_t size;      /*!< Video data size */
    bool     eos;       /*!< End of stream data */
} esp_rtmp_video_data_t;

/**
 * @brief RTMP event
 */
typedef enum {
    RTMP_EVENT_NONE,
    RTMP_EVENT_CLOSED_BY_SERVER,  /*!< Peer closed */
} esp_rtmp_event_t;

typedef int (*rtmp_event_cb)(esp_rtmp_event_t event, void* ctx);

#ifdef __cplusplus
}
#endif

#endif