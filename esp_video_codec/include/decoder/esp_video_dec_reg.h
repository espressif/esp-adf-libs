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
#include "esp_video_dec.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Video decoder operations
 */
typedef struct {
    /**
     * @brief  Callback to get video decoder capabilities
     *
     * @param[out]  caps   Video decoder capabilities to be queried
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*get_caps)(esp_video_dec_caps_t *caps);

    /**
     * @brief  Callback to open video decoder
     *
     * @param[in]   cfg  Video decoder configuration
     * @param[out]  h    Video decoder handle to store
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*open)(esp_video_dec_cfg_t *cfg, esp_video_dec_handle_t *h);

    /**
     * @brief  Callback to do video decode process
     *
     * @note  When one input frame can be decoded to multiple output frame, decoder should use `consumed` field,
     *        to indicate how many bytes have been consumed by decoder.
     *        To flush pending output frame, user can set `size` to 0 of `in_frame` and wait return `ESP_VC_ERR_NO_MORE_DATA` to indicate flush done.
     *
     * @param[in]      h          Video decoder handle
     * @param[in,out]  in_frame   Input frame
     * @param[in,out]  out_frame  Output frame
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*decode)(esp_video_dec_handle_t h, esp_video_dec_in_frame_t *in_frame, esp_video_dec_out_frame_t *out_frame);

    /**
     * @brief  Callback to get decoded frame information
     *
     * @param[in]   h     Video decoder handle
     * @param[out]  info  Video frame information to store
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*get_frame_info)(esp_video_dec_handle_t h, esp_video_codec_frame_info_t *info);

    /**
     * @brief  Callback to do video decoder setting
     *
     * @param[in]  h     Video decoder handle
     * @param[in]  type  Video decoder setting type
     * @param[in]  data  Setting data
     * @param[in]  size  Setting data size
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*set)(esp_video_dec_handle_t h, esp_video_dec_set_type_t type, void *data, uint32_t size);

    /**
     * @brief  Callback to close video decoder
     *
     * @param[in]  h  Video decoder handle
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
    */
    esp_vc_err_t (*close)(esp_video_dec_handle_t h);
} esp_video_dec_ops_t;

/**
 * @brief  Register video decoder
 *
 * @param[in]  desc  Video decoder description
 * @param[in]  ops   Video decoder operations
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NO_MEMORY    Out of memory
 */
esp_vc_err_t esp_video_dec_register(esp_video_codec_desc_t *desc, const esp_video_dec_ops_t *ops);

/**
 * @brief  Get video decoder operations by query information
 *
 * @param[in]  query  Query information
 *
 * @return
 *       - NULL    Video decoder for such codec is not registered
 *       - Others  Video decoder operations
 */
const esp_video_dec_ops_t *esp_video_dec_get_ops(esp_video_codec_query_t *query);

/**
 * @brief  Get video decoder description by index
 *
 * @param[in]   index  Index of the video decoders
 * @param[out]  desc   Video decoder description
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  Index overflow
 */
esp_vc_err_t esp_video_dec_get_desc(uint8_t index, esp_video_codec_desc_t *desc);

/**
 * @brief  Unregister video decoder by description
 *
 * @param[in]  desc  Video decoder description
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  Not found such codec
 */
esp_vc_err_t esp_video_dec_unregister(esp_video_codec_desc_t *desc);

#ifdef __cplusplus
}
#endif
