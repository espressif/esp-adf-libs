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

#ifndef ESP_MUXER_H
#define ESP_MUXER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_muxer_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_MUXER_MAX_SLICE_DURATION (0xFFFFFFFF)

typedef void* esp_muxer_handle_t;

/**
 * @brief Muxer container type
 */
typedef enum {
    ESP_MUXER_TYPE_TS,  /*!< Muxer to TS */
    ESP_MUXER_TYPE_MP4, /*!< Muxer to MP4 */
    ESP_MUXER_TYPE_FLV, /*!< Muxer to FLV */
    ESP_MUXER_TYPE_WAV, /*!< Muxer to WAV */
    ESP_MUXER_TYPE_CAF, /*!< Muxer to CAF */
    ESP_MUXER_TYPE_OGG, /*!< Muxer to OGG */
    ESP_MUXER_TYPE_MAX,
} esp_muxer_type_t;

/**
 * @brief         File pattern callback for getting file save path
 * @param[out]    file_path: Filepath for save data after mux
 * @param         len: Filepath limit length
 * @param         slice_idx: File slice index start from 0
 */
typedef int (*muxer_url_pattern)(char* file_path, int len, int slice_idx);

/**
 * @brief Struct for callback data
 */
typedef struct {
    uint8_t* data;
    uint32_t size;
} esp_muxer_data_info_t;

/**
 * @brief Write to buffer callback
 */
typedef int (*muxer_data_callback)(esp_muxer_data_info_t* data, void* ctx);

/**
 * @brief Muxer basic configuration
 *        If use certain container should use container extended configuration
 *        Ex: when use MP4 muxer should use `mp4_muxer_config_t` instead
 */
typedef struct {
    esp_muxer_type_t    muxer_type;          /*!< Muxer container type */
    uint32_t            slice_duration;      /*!< Muxer file segment duration, unit millisecond */
    muxer_url_pattern   url_pattern;         /*!< Muxer file path pattern callback for each segment */
    muxer_data_callback data_cb;             /*!< Muxer output callback can be coexist with url pattern.
                                                  It is suitable for living stream scenario.
                                                  When use MP4 muxer, please do not set. */
    void*               ctx;                 /*!< Muxer output callback context */
    uint32_t            ram_cache_size;      /*!< The file system has better performance when writing with aligned internal RAM.
                                                  Whereas typically provided stream data can't meet this requirement, so extra RAM cache is imported
                                                  Optimized cache size setting differs with different card, recommend to use 16K or above.
                                                  To finetune it, user can do speed test firstly (take `README.md` for reference) */
    bool                no_key_frame_verify; /*!< Whether disable internal logic to verify video packet is key frame or not
                                                  Internal parse logic is used to correct the `key_frame` flag when not set by user */                                          
} esp_muxer_config_t;

/**
 * @brief Muxer stream type
 */
typedef enum {
    ESP_MUXER_STREAM_TYPE_NONE,
    ESP_MUXER_STREAM_TYPE_AUDIO,  /*!< Audio stream type */
    ESP_MUXER_STREAM_TYPE_VIDEO,  /*!< Video stream type */
} esp_muxer_stream_type_t;

/**
 * @brief Muxer video codec type
 */
typedef enum {
    ESP_MUXER_VDEC_NONE,
    ESP_MUXER_VDEC_MJPEG, /*!< Motion JPEG video type */
    ESP_MUXER_VDEC_H264,  /*!< H264 video type */
} esp_muxer_video_codec_t;

/**
 * @brief Muxer audio codec type
 */
typedef enum {
    ESP_MUXER_ADEC_NONE,
    ESP_MUXER_ADEC_AAC,    /*!< AAC audio type */
    ESP_MUXER_ADEC_PCM,    /*!< PCM audio type, only support little endian */
    ESP_MUXER_ADEC_MP3,    /*!< MP3 audio type */
    ESP_MUXER_ADEC_ADPCM,  /*!< ADPCM audio type */
    ESP_MUXER_ADEC_G711_A, /*!< G711 alaw audio type */
    ESP_MUXER_ADEC_G711_U, /*!< G711 ulaw audio type */
    ESP_MUXER_ADEC_AMR_NB, /*!< AMR-NB audio type */
    ESP_MUXER_ADEC_AMR_WB, /*!< AMR-WB audio type */
    ESP_MUXER_ADEC_ALAC,   /*!< ALAC audio type */
    ESP_MUXER_ADEC_OPUS,   /*!< OPUS audio type */
} esp_muxer_audio_codec_t;

/**
 * @brief Muxer video stream info
 */
typedef struct {
    esp_muxer_video_codec_t codec;               /*!< Video codec type */
    uint16_t                width;               /*!< Video width */
    uint16_t                height;              /*!< Video height */
    uint8_t                 fps;                 /*!< Video framerate per second */
    uint32_t                min_packet_duration; /*!< Minimal packet duration (ms), used to estimate table size */
    void*                   codec_spec_info;     /*!< Video codec specified info.
                                                      For H264 need to provide SPS and PPS information:
                                                        Formed by Startcode + SPS + Startcode + PPS
                                                      Other video format no need to provide */
    int                     spec_info_len;       /*!< Video codec specified info length */
} esp_muxer_video_stream_info_t;

/**
 * @brief Muxer audio stream information
 */
typedef struct {
    esp_muxer_audio_codec_t codec;               /*!< Audio codec type */
    uint8_t                 channel;             /*!< Audio channel */
    uint8_t                 bits_per_sample;     /*!< Audio bits per sample */
    uint16_t                sample_rate;         /*!< Audio sample rate */
    uint32_t                min_packet_duration; /*!< Minimal packet duration (ms), used to estimate table size */
    void*                   codec_spec_info;     /*!< Audio codec specified info.
                                                      For AAC without ADTS header need provide AudioSpecicConfig:
                                                        Refer to: `https://wiki.multimedia.cx/index.php/MPEG-4_Audio`
                                                      Other audio format no need to provide */
    int                     spec_info_len;       /*!< Audio codec specified info length */
} esp_muxer_audio_stream_info_t;

/**
 * @brief Muxer video packet information
 */
typedef struct {
    void*    data;      /*!< Video packet data */
    int      len;       /*!< Video packet length */
    uint32_t pts;       /*!< Video packet pts */
    uint32_t dts;       /*!< Video packet dts, if not exists set to 0 */
    bool     key_frame; /*!< whether I frame or not */
} esp_muxer_video_packet_t;

/**
 * @brief Muxer audio packet information
 */
typedef struct {
    void*    data; /*!< Audio packet data */
    int      len;  /*!< Audio packet length */
    uint32_t pts;  /*!< Audio packet pts */
} esp_muxer_audio_packet_t;

/**
 * @brief Muxer file writer function table
 */
typedef struct {
    void* (*on_open)(char* path);               /*!< Function pointer for file open */
    int (*on_write)(void* writer, void* buffer,
                    int len);                   /*!< Function pointer for file write need return bytes wrote */
    int (*on_seek)(void* writer, uint64_t pos); /*!< Function pointer for file seek, return 0 means ok */
    int (*on_close)(void* writer);              /*!< Function pointer for file close, return 0 means ok */
} esp_muxer_file_writer_t;

/**
 * @brief Muxer register information
 */
typedef struct {
    esp_muxer_handle_t (*open)(esp_muxer_config_t* cfg, uint32_t size);
    esp_muxer_err_t (*add_video_stream)(esp_muxer_handle_t muxer, esp_muxer_video_stream_info_t* video_info, int* stream_index);
    esp_muxer_err_t (*add_audio_stream)(esp_muxer_handle_t muxer, esp_muxer_audio_stream_info_t* audio_info, int* stream_index);
    esp_muxer_err_t (*set_writer)(esp_muxer_handle_t muxer, esp_muxer_file_writer_t* writer);
    esp_muxer_err_t (*add_video_packet)(esp_muxer_handle_t muxer, int stream_index, esp_muxer_video_packet_t* packet);
    esp_muxer_err_t (*add_audio_packet)(esp_muxer_handle_t muxer, int stream_index, esp_muxer_audio_packet_t* packet);
    esp_muxer_err_t (*close)(esp_muxer_handle_t muxer);
} esp_muxer_reg_info_t;

/**
 * @brief Register muxer for certain container type
 *
 * @param         type: Muxer type
 * @param         reg_info: Register function table
 * @return
 *      - ESP_MUXER_ERR_OK: Register ok or already registered
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MUXER_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t esp_muxer_reg(esp_muxer_type_t type, esp_muxer_reg_info_t* reg_info);

/**
 * @brief Unregister for all container type
 */
void esp_muxer_unreg_all(void);

/**
 * @brief Open muxer
 *
 * @param         cfg: Muxer configuration
 * @param         size: Sizeof configuration, need set according size of related muxer type
 * @return        Handle of muxer instance, NULL for fail
 */
esp_muxer_handle_t esp_muxer_open(esp_muxer_config_t* cfg, uint32_t size);

/**
 * @brief Set file writer for muxer to storage output data
 * This function is specially used for customizing write file behavior.
 * When not provided, it will use default function internally.
 *
 * @param         muxer: Muxer handle
 * @param         writer: File writer function table
 * @return
 *      - ESP_MUXER_ERR_OK: On success
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid writer settings
 */
esp_muxer_err_t esp_muxer_set_file_writer(esp_muxer_handle_t muxer, esp_muxer_file_writer_t* writer);

/**
 * @brief Add video stream to muxer
 *
 * @param         muxer: Muxer handle
 * @param         video_info: Video stream information to add
 * @param[out]    stream_index: Output stream index used to identify which video stream, used later for adding video packet
 * @return
 *      - ESP_MUXER_ERR_OK: On success
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid settings
 *      - Others: Fail to add video stream
 */
esp_muxer_err_t esp_muxer_add_video_stream(esp_muxer_handle_t muxer, esp_muxer_video_stream_info_t* video_info, int* stream_index);

/**
 * @brief Add audio stream to muxer
 *
 * @param         muxer: Muxer handle
 * @param         audio_info: Audio stream information to add
 * @param[out]    stream_index: Output stream index used to identify which audio stream, used later for adding audio packet
 * @return
 *      - ESP_MUXER_ERR_OK: On success
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid settings
 *      - Others: Fail to add audio stream
 */
esp_muxer_err_t esp_muxer_add_audio_stream(esp_muxer_handle_t muxer, esp_muxer_audio_stream_info_t* audio_info, int* stream_index);

/**
 * @brief Add video packet data to muxer
 *
 * @param         muxer: Muxer handle
 * @param         stream_index: Stream index to distinguish which video stream
 * @param         packet: Video packet to be muxed
 * @return
 *      - ESP_MUXER_ERR_OK: Success to add video packet
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input arguments
 *      - Others: Fail to add video packet
 */
esp_muxer_err_t esp_muxer_add_video_packet(esp_muxer_handle_t muxer, int stream_index,
                                           esp_muxer_video_packet_t* packet);

/**
 * @brief Add audio packet data to muxer
 *
 * @param         muxer: Muxer handle
 * @param         stream_index: Stream index to distinguish which audio stream
 * @param         packet: Audio packet to be muxed
 * @return
 *      - ESP_MUXER_ERR_OK: Success to add audio packet
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input arguments
 *      - Others: Fail to add audio packet
 */
esp_muxer_err_t esp_muxer_add_audio_packet(esp_muxer_handle_t muxer, int stream_index,
                                           esp_muxer_audio_packet_t* packet);

/**
 * @brief Close muxer
 *
 * @param         muxer: Muxer handle
 * @return
 *      - ESP_MUXER_ERR_OK: On success
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input arguments
 *      - Others: Fail to close
 */
esp_muxer_err_t esp_muxer_close(esp_muxer_handle_t muxer);

#ifdef __cplusplus
}
#endif

#endif
