/**
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Video codec fourcc definition
 */
#define ESP_VIDEO_CODEC_FOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

/**
 * @brief  Video codec error code
 */
typedef enum {
    ESP_VC_ERR_NO_MORE_DATA   = 2,   /*!< No more data, do not process again */
    ESP_VC_ERR_BUF_NOT_ENOUGH = 1,   /*!< Buffer not enough, need reallocate and retry */
    ESP_VC_ERR_OK             = 0,   /*!< Status code for success */
    ESP_VC_ERR_FAIL           = -1,  /*!< General error code */
    ESP_VC_ERR_INVALID_ARG    = -2,  /*!< Invalid argument */
    ESP_VC_ERR_NO_MEMORY      = -3,  /*!< Memory not enough */
    ESP_VC_ERR_INVALID_STATE  = -4,  /*!< Invalid state */
    ESP_VC_ERR_NOT_SUPPORTED  = -5,  /*!< Not supported */
    ESP_VC_ERR_INTERNAL_ERROR = -6,  /*!< Internal error */
    ESP_VC_ERR_NOT_EXISTED    = -7,  /*!< Not existed */
    ESP_VC_ERR_WRONG_DATA     = -10, /*!< Wrong frame data */
} esp_vc_err_t;

/**
 * @brief  Video codec type
 */
typedef enum {
    ESP_VIDEO_CODEC_TYPE_NONE  = 0, /*!< Invalid codec type */
    ESP_VIDEO_CODEC_TYPE_H264  = 1, /*!< H264 codec type */
    ESP_VIDEO_CODEC_TYPE_MJPEG = 2, /*!< MJPEG codec type */
} esp_video_codec_type_t;

/**
 * @brief  Video codec pixel format
 */
typedef enum {
    ESP_VIDEO_CODEC_PIXEL_FMT_NONE        = 0, /*!< Invalid pixel format */
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE   = 1, /*!< RGB565 little endian pixel format */
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE   = 2, /*!< RGB565 big endian pixel format */
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB888      = 3, /*!< RGB888 pixel format */
    ESP_VIDEO_CODEC_PIXEL_FMT_BGR888      = 4, /*!< BGR888 pixel format */
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P     = 5, /*!< YUV420P pixel format (YY.. x n, U.. x n/2, V.. x n/2) */
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV422P     = 6, /*!< YUV422P pixel format (YY.. x n, U.. x n, V.. x n) */
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV422      = 7, /*!< YUV422 pixel format (YUYV.. x n) */
    ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422     = 8, /*!< UYVY422 pixel format (UYVY.. x n)*/
    ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY = 9, /*!< Odd line is UYY.. and even line is VYY.. */
} esp_video_codec_pixel_fmt_t;

/**
 * @brief  Video codec frame type
 */
typedef enum {
    ESP_VIDEO_CODEC_FRAME_TYPE_NONE = 0, /*!< Invalid frame type */
    ESP_VIDEO_CODEC_FRAME_TYPE_IDR  = 1, /*!< IDR frame type */
    ESP_VIDEO_CODEC_FRAME_TYPE_I    = 2, /*!< I frame type */
    ESP_VIDEO_CODEC_FRAME_TYPE_P    = 3, /*!< P frame type */
    ESP_VIDEO_CODEC_FRAME_TYPE_B    = 4, /*!< B frame type */
} esp_video_codec_frame_type_t;

/**
 * @brief  Video codec description, specially used for codec registration
 */
typedef struct {
    esp_video_codec_type_t codec_type; /*!< Codec type */
    bool                   is_hw;      /*!< Whether the codec is hardware */
    uint32_t               codec_cc;   /*!< Fourcc representing for the codec */
} esp_video_codec_desc_t;

/**
 * @brief  Video codec query setting
 */
typedef struct {
    esp_video_codec_type_t codec_type; /*!< Codec type */
    uint32_t               codec_cc;   /*!< Fourcc representing for the codec
                                            If set to 0, it will try to prefer hardware implement,
                                            Otherwise, it will match registered codec with the fourcc exactly. */
} esp_video_codec_query_t;

/**
 * @brief  Video resolution for video codec
 */
typedef struct {
    uint32_t width;  /*!< Video width */
    uint32_t height; /*!< Video height */
} esp_video_codec_resolution_t;

/**
 * @brief  Rectangle define for video codec
 */
typedef struct {
    uint32_t x;      /*!< Rectangle position x */
    uint32_t y;      /*!< Rectangle position y */
    uint32_t width;  /*!< Rectangle width */
    uint32_t height; /*!< Rectangle height */
} esp_video_codec_rect_t;

/**
 * @brief  Video codec frame info
 */
typedef struct {
    esp_video_codec_resolution_t origin_res; /*!< Original resolution */
    esp_video_codec_resolution_t res;        /*!< Decoded resolution */
    uint32_t                     bitrate;    /*!< Video bitrate */
    uint8_t                      fps;        /*!< Video frames per second */
} esp_video_codec_frame_info_t;

/**
 * @brief  H264 codec specified information
 *
 * @note  This structure is only used for H264 codec
 *        When SPS and PPS data is not contained in decode input data, need provide it manually.
 *        When SPS and PPS data is contained in decode input data, this structure can be ignored.
 */
typedef struct {
    uint8_t  *sps;      /*!< H264 SPS(sequence parameter) data */
    uint8_t  *pps;      /*!< H264 PPS (picture parameter) data */
    uint16_t  sps_size; /*!< H264 SPS data size */
    uint16_t  pps_size; /*!< H264 PPS data size */
} esp_video_codec_h264_spec_info_t;

/**
 * @brief  Video codec chroma subsampling
 */
typedef enum {
    ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_NONE = 0, /*!< Invalid chroma subsampling */
    ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_GRAY = 1, /*!< Gray chroma subsampling */
    ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_444  = 2, /*!< 4:4:4 chroma subsampling */
    ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422  = 3, /*!< 4:2:2 chroma subsampling */
    ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_420  = 4, /*!< 4:2:0 chroma subsampling */
} esp_video_codec_chroma_subsampling_t;

#ifdef __cplusplus
}
#endif
