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
#include "video_codec_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  MJPEG hardware decoder capabilities
 */
#define VIDEO_HW_DEC_MJPEG_IN_FRAME_ALIGN   1
#define VIDEO_HW_DEC_MJPEG_OUT_FRAME_ALIGN  VIDEO_CODEC_HW_CACHE_LINE_ALIGNMENT
#define VIDEO_HW_DEC_MJPEG_OUT_FMTS      \
{                                        \
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB888,    \
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE, \
    ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422,   \
}

#define VIDEO_HW_DEC_MJPEG_TYPICAL_WIDTH  1920
#define VIDEO_HW_DEC_MJPEG_TYPICAL_HEIGHT 1080
#define VIDEO_HW_DEC_MJPEG_TYPICAL_FPS    30

#define VIDEO_HW_DEC_MJPEG_MAX_WIDTH      3840
#define VIDEO_HW_DEC_MJPEG_MAX_HEIGHT     2160

#ifdef __cplusplus
}
#endif
