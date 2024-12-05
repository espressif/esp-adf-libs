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

#define ESP_VIDEO_DEC_SW_H264_TAG ESP_VIDEO_CODEC_FOURCC('S', '2', '6', '4')

/**
 * @brief  Register software H264 video decoder
 *
 * @return
 *       - ESP_VC_ERR_OK         Register success
 *       - ESP_VC_ERR_NO_MEMORY  Out of memory
 */
esp_vc_err_t esp_video_dec_register_sw_h264(void);

/**
 * @brief  Unregister software H264 video decoder
 *
 * @return
 *       - ESP_VC_ERR_OK           Unregister success
 *       - ESP_VC_ERR_NOT_EXISTED  Such codec not existed
 */
esp_vc_err_t esp_video_dec_unregister_sw_h264(void);

#ifdef __cplusplus
}
#endif
