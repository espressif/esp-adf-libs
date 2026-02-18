/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Four-CC representation for extractor
 */
#define EXTRACTOR_4CC(a, b, c, d)  ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

/**
 * @brief  End of stream flag for extractor frame
 */
#define EXTRACTOR_FRAME_FLAG_EOS  (1 << 0)

/**
 * @brief  Whether frame for extractor is ended
 */
#define EXTRACTOR_IS_EOS(flag)  ((flag & EXTRACTOR_FRAME_FLAG_EOS) != 0)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Extractor stream type
 */
typedef enum {
    ESP_EXTRACTOR_STREAM_TYPE_NONE  = 0,  /*!< Invalid stream type */
    ESP_EXTRACTOR_STREAM_TYPE_AUDIO = 1,  /*!< Audio stream type */
    ESP_EXTRACTOR_STREAM_TYPE_VIDEO = 2,  /*!< Video stream type */
} esp_extractor_stream_type_t;

/**
 * @brief  Extractor format
 */
typedef enum {
    ESP_EXTRACTOR_FORMAT_NONE         = 0,  /*!< Invalid format */
    // Audio format definition
    ESP_EXTRACTOR_AUDIO_FORMAT_PCM    = EXTRACTOR_4CC('P', 'C', 'M', ' '),  /*!< PCM audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_ADPCM  = EXTRACTOR_4CC('A', 'D', 'P', 'C'),  /*!< ADPCM audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_AAC    = EXTRACTOR_4CC('A', 'A', 'C', ' '),  /*!< AAC audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_MP3    = EXTRACTOR_4CC('M', 'P', '3', ' '),  /*!< MP3 audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_AC3    = EXTRACTOR_4CC('A', 'C', '3', ' '),  /*!< AC3 audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_VORBIS = EXTRACTOR_4CC('V', 'O', 'B', 'S'),  /*!< VORBIS audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_OPUS   = EXTRACTOR_4CC('O', 'P', 'U', 'S'),  /*!< OPUS audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_FLAC   = EXTRACTOR_4CC('F', 'L', 'A', 'C'),  /*!< FLAC audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_AMRNB  = EXTRACTOR_4CC('A', 'M', 'R', 'N'),  /*!< AMR-NB audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_AMRWB  = EXTRACTOR_4CC('A', 'M', 'R', 'W'),  /*!< AMR-WB audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_G711A  = EXTRACTOR_4CC('A', 'L', 'A', 'W'),  /*!< G711-Alaw audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_G711U  = EXTRACTOR_4CC('U', 'L', 'A', 'W'),  /*!< G711-Ulaw audio format */
    ESP_EXTRACTOR_AUDIO_FORMAT_ALAC   = EXTRACTOR_4CC('A', 'L', 'A', 'C'),  /*!< ALAC audio format */
    // Video format definition
    ESP_EXTRACTOR_VIDEO_FORMAT_H264   = EXTRACTOR_4CC('H', '2', '6', '4'),  /*!< H264 video format */
    ESP_EXTRACTOR_VIDEO_FORMAT_MJPEG  = EXTRACTOR_4CC('M', 'J', 'P', 'G'),  /*!< MJPEG video format */
} esp_extractor_format_t;

/**
 * @brief  Video stream information
 */
typedef struct {
    esp_extractor_format_t  format;  /*!< Video format */
    uint16_t                fps;     /*!< Video frames per second */
    uint16_t                width;   /*!< Video width */
    uint16_t                height;  /*!< Video height */
} esp_extractor_video_stream_info_t;

/**
 * @brief  Audio stream information
 */
typedef struct {
    esp_extractor_format_t  format;           /*!< Audio format */
    uint8_t                 channel;          /*!< Audio output channel */
    uint8_t                 bits_per_sample;  /*!< Audio bits per sample */
    uint32_t                sample_rate;      /*!< Audio sample rate */
} esp_extractor_audio_stream_info_t;

/**
 * @brief  Stream information
 */
typedef struct {
    esp_extractor_stream_type_t  stream_type;    /*!< Stream type */
    uint16_t                     stream_id;      /*!< Stream id */
    uint32_t                     duration;       /*!< Stream total duration (unit milliseconds) */
    uint32_t                     bitrate;        /*!< Stream bitrate */
    uint8_t                     *spec_info;      /*!< Stream specified information */
    uint32_t                     spec_info_len;  /*!< Stream specified information length */
    union {
        esp_extractor_video_stream_info_t  video_info;  /*!< Video stream information */
        esp_extractor_audio_stream_info_t  audio_info;  /*!< Audio stream information */
    };
} esp_extractor_stream_info_t;

/**
 * @brief  Output frame information
 */
typedef struct {
    esp_extractor_stream_type_t  stream_type;   /*!< Stream type */
    uint16_t                     stream_idx;    /*!< Stream index of the stream */
    uint8_t                      frame_flag;    /*!< Frame flag (like eos etc) */
    uint32_t                     pts;           /*!< Stream PTS (unit milliseconds) */
    uint8_t                     *frame_buffer;  /*!< Frame data pointer (output only) */
    uint32_t                     frame_size;    /*!< Frame data size */
    uint32_t                     frame_pos;     /*!< Frame byte position */
} esp_extractor_frame_info_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
