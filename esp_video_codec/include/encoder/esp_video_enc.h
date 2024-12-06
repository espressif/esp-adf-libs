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

#include "esp_video_codec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Definition for video encoder capabilities mask
 */
#define ESP_VIDEO_ENC_CAPS(type) ((uint32_t)1 << (type))

/**
 * @brief  Video encoder handle
 */
typedef void *esp_video_enc_handle_t;

/**
 * @brief  Configuration for video encoder
 */
typedef struct {
    esp_video_codec_type_t       codec_type; /*!< Video codec type */
    uint32_t                     codec_cc;   /*!< Codec fourcc representing
                                                  If set to 0 it will try to prefer hardware implement,
                                                  otherwise it will match codec with the fourcc exactly */
    esp_video_codec_pixel_fmt_t  in_fmt;     /*!< Encoder input pixel format */
    esp_video_codec_resolution_t resolution; /*!< Resolution of video to be encoded */
    uint32_t                     fps;        /*!< Video framerate */
} esp_video_enc_cfg_t;

/**
 * @brief  Video encoder setting type
 */
typedef enum {
    ESP_VIDEO_ENC_SET_TYPE_BITRATE            = 0,  /*!< Bitrate setting type */
    ESP_VIDEO_ENC_SET_TYPE_GOP                = 1,  /*!< GOP size setting type */
    ESP_VIDEO_ENC_SET_TYPE_QP                 = 2,  /*!< QP (quantization parameter) setting type */
    ESP_VIDEO_ENC_SET_TYPE_QUALITY            = 3,  /*!< QP (quantization parameter) setting type */
    ESP_VIDEO_ENC_SET_TYPE_FPS                = 4,  /*!< Frame rate setting type */
    ESP_VIDEO_ENC_SET_TYPE_FORCE_IDR          = 5,  /*!< Force to output IDR on next frame setting type */
    ESP_VIDEO_ENC_SET_TYPE_SPS_PPS_RESEND     = 6,  /*!< Control to resend SPS and PPS */
    ESP_VIDEO_ENC_SET_TYPE_ROTATE             = 7,  /*!< Rotate setting type */
    ESP_VIDEO_ENC_SET_TYPE_RESIZE             = 8,  /*!< Resize setting type */
    ESP_VIDEO_ENC_SET_TYPE_CROP               = 9,  /*!< Crop setting type */
    ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING = 10, /*!< Chroma subsampling setting type */
} esp_video_enc_set_type_t;

/**
 * @brief  Video encoder capabilities
 */
typedef struct {
    uint8_t                            in_frame_align;  /*!< Input frame alignment */
    uint8_t                            out_frame_align; /*!< Output frame alignment */
    uint32_t                           set_caps;        /*!< Setting capabilities */
    const esp_video_codec_pixel_fmt_t *in_fmts;         /*!< Supported input formats */
    uint8_t                            in_fmt_num;      /*!< Number of supported input formats */
    uint8_t                            typical_fps;     /*!< Typical frame rate */
    esp_video_codec_resolution_t       typical_res;     /*!< Typical resolution */
    esp_video_codec_resolution_t       max_res;         /*!< Maximum supported resolution
                                                             If set to 0, it is limited by system memory or CPU */
} esp_video_enc_caps_t;

/**
 * @brief  Video encoder input frame information
 */
typedef struct {
    uint32_t  pts;      /*!< Presentation timestamp (unit ms) */
    uint8_t  *data;     /*!< Input data to be encoded */
    uint32_t  size;     /*!< Input data size */
    uint32_t  consumed; /*!< Input data consumed by encoder */
} esp_video_enc_in_frame_t;

/**
 * @brief  Video encoder output frame information
 */
typedef struct {
    esp_video_codec_frame_type_t frame_type;   /*!< Video frame type */
    uint32_t                     pts;          /*!< Presentation timestamp (unit ms) */
    uint32_t                     dts;          /*!< Decoder timestamp (unit ms) */
    uint8_t                     *data;         /*!< Memory to put encoded data */
    uint32_t                     size;         /*!< Memory size to put encoded data */
    uint32_t                     encoded_size; /*!< Actual encoded data size */
} esp_video_enc_out_frame_t;

/**
 * @brief  Query video encoder capabilities by codec type
 *
 * @param[in]   query  Query information
 * @param[out]  caps   Encoder capabilities
 *
 * @return
 *       - ESP_VC_ERR_OK           Query success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  The codec type is not registered yet
 */
esp_vc_err_t esp_video_enc_query_caps(esp_video_codec_query_t *query, esp_video_enc_caps_t *caps);

/**
 * @brief  Open video encoder
 *
 * @param[in]   cfg     Video encoder configuration
 * @param[out]  handle  Video encoder handle to store
 *
 * @return
 *       - ESP_VC_ERR_OK           Query success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NO_MEMORY    Out of memory
 *       - ESP_VC_ERR_NOT_EXISTED  Encoder is not registered yet
 */
esp_vc_err_t esp_video_enc_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle);

/**
 * @brief  Get video encoder input and output frame alignment
 *
 * @param[in]   handle           Video encoder handle
 * @param[out]  in_frame_align   Input frame alignment to stored
 * @param[out]  out_frame_align  Output frame alignment to stored
 *
 * @return
 *       - ESP_VC_ERR_OK             Query success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported to get alignment
 */
esp_vc_err_t esp_video_enc_get_frame_align(esp_video_enc_handle_t handle, uint8_t *in_frame_align, uint8_t *out_frame_align);

/**
 * @brief  Set bitrate for video encoder
 *
 * @param[in]  handle   Video encoder handle
 * @param[in]  bitrate  Target bitrate
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_bitrate(esp_video_enc_handle_t handle, uint32_t bitrate);

/**
 * @brief  Set picture quality for video encoder
 *
 * @param[in]  handle  Video encoder handle
 * @param[in]  min_qp  Minimum QP
 * @param[in]  max_qp  Maximum QP
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_qp(esp_video_enc_handle_t handle, uint32_t min_qp, uint32_t max_qp);

/**
 * @brief  Set picture quality for video encoder
 *
 * @param[in]  handle   Video encoder handle
 * @param[in]  quality  Video encoder quality (for JPEG range from [0, 100])
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_quality(esp_video_enc_handle_t handle, uint8_t quality);

/**
 * @brief  Set framerate for video encoder
 *
 * @param[in]  handle  Video encoder handle
 * @param[in]  fps     Frame rate unit (frame per second)
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_fps(esp_video_enc_handle_t handle, uint32_t fps);

/**
 * @brief  Set GOP for video encoder
 *
 * @param[in]  handle      Video encoder handle
 * @param[in]  gop_frames  Frame count in one GOP
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_gop(esp_video_enc_handle_t handle, uint32_t gop_frames);

/**
 * @brief  Set resend SPS/PPS for video encoder
 *
 * @note  All frames sharing the same SPS/PPS can reduce the size of the encoded data.
 *        However, for real-time streaming where instant decoding is required, the SPS/PPS should be resent.
 *        This API controls the behavior of resending the SPS/PPS when GOP number exceeds the `gop_num`.
 *
 * @param[in]  handle   Video encoder handle
 * @param[in]  gop_num  Resend when processing GOP number over gop_num
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_resend_sps_pps(esp_video_enc_handle_t handle, uint32_t gop_num);

/**
 * @brief  Set force IDR for video encoder
 *
 * @note  After set force IDR, next frame force to be a IDR, this action will reset the GOP.
 *        Also it will trigger only once.
 *
 * @param[in]  handle  Video encoder handle
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_force_idr(esp_video_enc_handle_t handle);

/**
 * @brief  Set video encoder subsampling
 *
 * @param[in]  handle       Video encoder handle
 * @param[in]  subsampling  Chroma subsampling to set
 *
 * @return
 *       - ESP_VC_ERR_OK           Set success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - Others                  Fail to set
 */
esp_vc_err_t esp_video_enc_set_chroma_subsampling(esp_video_enc_handle_t handle, esp_video_codec_chroma_subsampling_t subsampling);

/**
 * @brief  Do video encoder process
 *
 * @note  Input frame have a `consumed` field, which indicate how many bytes have been consumed by encoder.
 *        User need repeat call this API until all bytes have been consumed.
 *        For there is no special flush API, in order to flush pending output frame.
 *        User can set `size` to 0 of `in_frame` and wait return `ESP_VC_ERR_NO_MORE_DATA` to indicate flush done.
 *
 * @note  After set force IDR, next frame force to be a IDR, this action will reset the GOP.
 *         Also it will trigger only once.
 *
 * @param[in]      handle     Video encoder handle
 * @param[in,out]  in_frame   Information for input frame to be encoded
 * @param[in,out]  out_frame  Information for output encoded frame
 *
 * @return
 *       - ESP_VC_ERR_OK              Process success
 *       - ESP_VC_ERR_INVALID_ARG     Invalid argument
 *       - ESP_VC_ERR_BUF_NOT_ENOUGH  Buffer for output frame not enough
 */
esp_vc_err_t esp_video_enc_process(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame,
                                   esp_video_enc_out_frame_t *out_frame);

/**
 * @brief  Close video process
 *
 * @param[in]  handle  Video encoder handle
 *
 * @return
 *       - ESP_VC_ERR_OK           Process success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 */
esp_vc_err_t esp_video_enc_close(esp_video_enc_handle_t handle);

#ifdef __cplusplus
}
#endif
