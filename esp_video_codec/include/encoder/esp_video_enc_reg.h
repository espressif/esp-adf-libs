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

#include "esp_video_enc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Video encoder QP(quantization parameter) range setting
 */
typedef struct {
    uint32_t min_qp; /*!< Minimum QP */
    uint32_t max_qp; /*!< Maximum QP */
} esp_video_enc_qp_set_t;

/**
 * @brief  Video encoder operations
 */
typedef struct {
    /**
     * @brief  Callback to get video encoder capabilities
     *
     * @param[out]  caps   Video encoder capabilities to be queried
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*get_caps)(esp_video_enc_caps_t *caps);

    /**
     * @brief  Callback to open video encoder
     *
     * @param[in]   cfg  Video encoder configuration
     * @param[out]  h    Video encoder handle to store
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*open)(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *h);

    /**
     * @brief  Callback to do video encoder setting
     *
     * @param[in]  h     Video encoder handle
     * @param[in]  type  Video encoder setting type
     * @param[in]  data  Setting data
     * @param[in]  size  Setting data size
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*set)(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size);

    /**
     * @brief  Callback to do video encode process
     *
     * @note  When one input frame can be encoded to multiple output frame, encoder should use `consumed` field,
     *        to indicate how many bytes have been consumed by encoder.
     *        If encode have pending frames, it should allow input frame `size` to be 0,
     *        to flush all pending frames and report `ESP_VC_ERR_NO_MORE_DATA` when all frames have been flushed.
     *
     * @param[in]      h          Video encoder handle
     * @param[in,out]  in_frame   Input frame
     * @param[in,out]  out_frame  Output frame
     *
     * @return
     *       - ESP_VC_ERR_OK            On success
     *       - ESP_VC_ERR_NO_MORE_DATA  All frame processed
     *       - Others                   Fail
     */
    esp_vc_err_t (*encode)(esp_video_enc_handle_t h, esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame);

    /**
     * @brief  Callback to close video encoder
     *
     * @param[in]  h  Video encoder handle
     *
     * @return
     *       - ESP_VC_ERR_OK  On success
     *       - Others         Fail
     */
    esp_vc_err_t (*close)(esp_video_enc_handle_t h);
} esp_video_enc_ops_t;

/**
 * @brief  Register video encoder
 *
 * @param[in]  desc  Codec description
 * @param[in]  ops   Video encoder operations
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NO_MEMORY    Out of memory
 */
esp_vc_err_t esp_video_enc_register(esp_video_codec_desc_t *desc, const esp_video_enc_ops_t *ops);

/**
 * @brief  Get video encoder operations by query information
 *
 * @param[in]  query  Video encoder query information
 *
 * @return
 *       - NULL    Video encoder for such codec is not registered
 *       - Others  Video encoder operations
 */
const esp_video_enc_ops_t *esp_video_enc_get_ops(esp_video_codec_query_t *query);

/**
 * @brief  Get video encoder codec type by index
 *
 * @param[in]   index  Index of the video encoders
 * @param[out]  desc   Video encoder description
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  Index overflow
 */
esp_vc_err_t esp_video_enc_get_desc(uint8_t index, esp_video_codec_desc_t *desc);

/**
 * @brief  Unregister video encoder by description
 *
 * @param[in]  desc  Video encoder description
 *
 * @return
 *       - ESP_VC_ERR_OK           On success
 *       - ESP_VC_ERR_INVALID_ARG  Invalid argument
 *       - ESP_VC_ERR_NOT_EXISTED  Not found such codec
 */
esp_vc_err_t esp_video_enc_unregister(esp_video_codec_desc_t *desc);

#ifdef __cplusplus
}
#endif
