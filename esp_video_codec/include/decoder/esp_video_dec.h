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
 * @brief  Definition for video decode capabilities mask
 */
#define ESP_VIDEO_DEC_CAPS(type) ((uint32_t)1 << (type))

/**
 * @brief  Video decoder handle
 */
typedef void *esp_video_dec_handle_t;

/**
 * @brief  Configuration of video decoder
 */
typedef struct {
    esp_video_codec_type_t      codec_type;           /*!< Decoder type */
    uint32_t                    codec_cc;             /*!< Codec fourcc representing
                                                           If set to 0 it will try to prefer hardware implement,
                                                           otherwise it will match codec with the fourcc exactly */
    esp_video_codec_pixel_fmt_t out_fmt;              /*!< Decoder output format */
    uint8_t                    *codec_spec_info;      /*!< Codec specific info
                                                           ex: when H264 data not contain SPS and PPS, need provide it use `esp_video_codec_h264_spec_info_t` */
    uint32_t                    codec_spec_info_size; /*!< Codec specific info size */
} esp_video_dec_cfg_t;

/**
 * @brief  Video decoder input frame information
 */
typedef struct {
    uint32_t pts;      /*!< Presentation timestamp (unit ms) */
    uint32_t dts;      /*!< Decoding timestamp (unit ms) */
    uint8_t *data;     /*!< Input data to be decoded */
    uint32_t size;     /*!< Input data size */
    uint32_t consumed; /*!< Input data consumed by decoder */
} esp_video_dec_in_frame_t;

/**
 * @brief  Video decoder output frame information
 */
typedef struct {
    esp_video_codec_frame_type_t frame_type;   /*!< Video frame type */
    uint32_t                     pts;          /*!< Presentation timestamp (unit ms) */
    uint32_t                     dts;          /*!< Decoding timestamp (unit ms) */
    uint8_t                     *data;         /*!< Store decoded output data */
    uint32_t                     size;         /*!< Total size of output data */
    uint32_t                     decoded_size; /*!< Actual size of decoded data */
} esp_video_dec_out_frame_t;

/**
 * @brief  Video decoder setting type
 */
typedef enum {
    ESP_VIDEO_DEC_SET_TYPE_ROTATE       = 0, /*!< Rotate setting type */
    ESP_VIDEO_DEC_SET_TYPE_CROP         = 1, /*!< Crop setting type */
    ESP_VIDEO_DEC_SET_TYPE_RESIZE       = 2, /*!< Resize setting type */
    ESP_VIDEO_DEC_SET_TYPE_BLOCK_DECODE = 3, /*!< Block decode setting type
                                                  Enable block decode allow decode by lines to use less memory
                                                  While intermediate data store into RAM speed up the decode speed */
} esp_video_dec_set_type_t;

/**
 * @brief  Video decoder capabilities
 */
typedef struct {
    uint8_t                            in_frame_align;  /*!< Input frame alignment */
    uint8_t                            out_frame_align; /*!< Output frame alignment */
    uint32_t                           set_caps;        /*!< Setting capabilities */
    const esp_video_codec_pixel_fmt_t *out_fmts;        /*!< Supported output formats */
    uint8_t                            out_fmt_num;     /*!< Number of supported output formats */
    uint8_t                            typical_fps;     /*!< Typical used frame rate */
    esp_video_codec_resolution_t       typical_res;     /*!< Typical used resolution */
    esp_video_codec_resolution_t       max_res;         /*!< Maximum supported resolution */
} esp_video_dec_caps_t;

/**
 * @brief  Query video decoder capabilities
 *
 * @param[in]   query  Query setting
 * @param[out]  caps   Decoder capabilities
 *
 * @return
 *       - ESP_VC_ERR_OK           Query success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  The codec type is not registered yet
 */
esp_vc_err_t esp_video_dec_query_caps(esp_video_codec_query_t *query, esp_video_dec_caps_t *caps);

/**
 * @brief  Open video decoder
 *
 * @param[in]   cfg     Video decoder configuration
 * @param[out]  handle  Video decoder handle to store
 *
 * @return
 *       - ESP_VC_ERR_OK           Open success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NO_MEMORY    Out of memory
 *       - ESP_VC_ERR_NOT_EXISTED  The codec type is not registered yet
 */
esp_vc_err_t esp_video_dec_open(esp_video_dec_cfg_t *cfg, esp_video_dec_handle_t *handle);

/**
 * @brief  Get video decoder input and output frame alignment
 *
 * @param[in]   handle           Video decoder handle
 * @param[out]  in_frame_align   Input frame alignment to store
 * @param[out]  out_frame_align  Output frame alignment to store
 *
 * @return
 *       - ESP_VC_ERR_OK             Get success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported for this codec
 */
esp_vc_err_t esp_video_dec_get_frame_align(esp_video_dec_handle_t handle, uint8_t *in_frame_align, uint8_t *out_frame_align);

/**
 * @brief  Video decoder processing
 *
 * @param[in]      handle     Video decoder handle
 * @param[in,out]  in_frame   Input frame to be decoded
 * @param[in,out]  out_frame  Output decoded frame information
 *
 * @return
 *       - ESP_VC_ERR_OK              Process success
 *       - ESP_VC_ERR_INVALID_ARG     Invalid argument
 *       - ESP_VC_ERR_NO_MEMORY       Out of memory
 *       - ESP_VC_ERR_BUF_NOT_ENOUGH  Out frame buffer not enough, can reallocate and retry
 *       - ESP_VC_ERR_NO_MORE_DATA    If set `size` to 0 of `in_frame` means flush done
 */
esp_vc_err_t esp_video_dec_process(esp_video_dec_handle_t handle, esp_video_dec_in_frame_t *in_frame,
                                   esp_video_dec_out_frame_t *out_frame);

/**
 * @brief  Get video decoder frame information
 *
 * @param[in]   handle  Video decoder handle
 * @param[out]  info    Frame information be stored
 *
 * @return
 *       - ESP_VC_ERR_OK             Get success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_INVALID_STATE  Wrong state, frame information is not ready yet
 */
esp_vc_err_t esp_video_dec_get_frame_info(esp_video_dec_handle_t handle, esp_video_codec_frame_info_t *info);

/**
 * @brief  Set rotate for video decoder
 *
 * @note  Only some of the decoder have rotate capability, query and check `set_caps` in `esp_video_dec_caps_t` firstly
 *        When supported its bitwise contain `ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_ROTATE)`
 *
 * @param[in]  handle  Video decoder handle
 * @param[in]  degree  Degree to be rotated
 *
 * @return
 *       - ESP_VC_ERR_OK             Set success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported
 */
esp_vc_err_t esp_video_dec_set_rotate(esp_video_dec_handle_t handle, uint32_t degree);

/**
 * @brief  Set crop for video decoder
 *
 * @note  Only some of the decoder have rotate capability, query and check `set_caps` in `esp_video_dec_caps_t` firstly.
 *        When supported its bitwise contain `ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_CROP)`
 *
 * @param[in]  handle  Video decoder handle
 * @param[in]  rect    Region to be cropped
 *
 * @return
 *       - ESP_VC_ERR_OK             Set success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported
 */
esp_vc_err_t esp_video_dec_set_crop(esp_video_dec_handle_t handle, esp_video_codec_rect_t *rect);

/**
 * @brief  Set resize for video decoder
 *
 * @note  Only some of the decoder have rotate capability, query and check `set_caps` in `esp_video_dec_caps_t` firstly
 *        When supported its bitwise contain `ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_RESIZE)`
 *
 * @param[in]  handle   Video decoder handle
 * @param[in]  dst_res  Resolution to be resized
 *
 * @return
 *       - ESP_VC_ERR_OK             Set success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported
 */
esp_vc_err_t esp_video_dec_set_resize(esp_video_dec_handle_t handle, esp_video_codec_resolution_t *dst_res);

/**
 * @brief  Set block decode for video decoder
 *
 * @note  Some decoder support block decoder, it can speed up the decoding process and put decoding buffer on chip memory
 *
 * @param[in]  handle             Video decoder handle
 * @param[in]  enable_block_mode  Whether enable block mode decoder
 *
 * @return
 *       - ESP_VC_ERR_OK             Set success
 *       - ESP_VC_ERR_INVALID_ARG    Invalid argument
 *       - ESP_VC_ERR_NOT_SUPPORTED  Not supported
 */
esp_vc_err_t esp_video_dec_set_block_decode(esp_video_dec_handle_t handle, bool enable_block_mode);

/**
 * @brief  Close video decoder
 *
 * @param[in]  handle  Video decoder handle
 *
 * @return
 *       - ESP_VC_ERR_OK           Close success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 */
esp_vc_err_t esp_video_dec_close(esp_video_dec_handle_t handle);

#ifdef __cplusplus
}
#endif
