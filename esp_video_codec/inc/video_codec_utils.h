/*
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

#include <string.h>
#include "esp_video_codec_utils.h"
#include "esp_video_codec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VIDEO_CODEC_HW_CACHE_LINE_ALIGNMENT    (0x10000)
#define SAFE_STR(str)                          ((str) ? (str) : "")
#define ALIGN_UP(num, align)                   (((num) + ((align) - 1)) & ~((align) - 1))
#define ELEMENTS_OF(array)                     (sizeof(array) / sizeof(array[0]))
#define video_codec_calloc(n, size)            calloc(n, size)
#define video_codec_calloc_struct(struct_name) (struct_name*)video_codec_calloc(1, sizeof(struct_name))

#define VIDEO_CODEC_ARG_CHECK(cond)                         \
    if (cond) {                                             \
        ESP_LOGD(TAG, "Invalid argument for %s", __func__); \
        return ESP_VC_ERR_INVALID_ARG;                      \
    }

#define VIDEO_CODEC_MEM_CHECK(ptr)                   \
    if (ptr == NULL) {                               \
        ESP_LOGD(TAG, "No memory for %s", __func__); \
        return ESP_VC_ERR_NO_MEMORY;                 \
    }

#define IS_SUPPORTED_VIDEO_FMT(chk_fmt, fmts) video_codec_is_fmt_support(chk_fmt, fmts, ELEMENTS_OF(fmts))

/**
 * @brief  Get alignment size according alignment type
 *
 * @param[in]  align_size_type  Alignment size type
 *
 * @return
 *       - Actual alignment size
 */
uint8_t video_codec_get_align_size(uint32_t align_size_type);

/**
 * @brief  Check whether pixel format is supported or not
 *
 * @param[in]  chk_fmt  Pixel format to be checked
 * @param[in]  fmts     Supported pixel formats
 * @param[in]  num      Number of supported pixel formats
 *
 * @return
 *       - true   Pixel format is supported
 *       - false  Pixel format is not supported
 */
bool video_codec_is_fmt_support(esp_video_codec_pixel_fmt_t chk_fmt, esp_video_codec_pixel_fmt_t *fmts, int num);

/**
 * @brief  Calculate typical frame rate according real cpu frequency
 *
 * @param[in]  refer_fps   Reference fps based on reference frequency
 * @param[in]  refer_freq  Reference frequency
 *
 * @return
 *       - Calculated typical frame rate
 */
uint8_t video_codec_calc_typical_fps(uint8_t refer_fps, uint32_t refer_freq);

#ifdef __cplusplus
}
#endif
