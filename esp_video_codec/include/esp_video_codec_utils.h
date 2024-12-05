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
#include "esp_video_codec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Malloc memory for video codec
 *
 * @param[in]  size  Size to be allocated
 *
 * @return
 *       - NULL    Fail to allocate
 *       - Others  On success
 */
void *esp_video_codec_malloc(uint32_t size);

/**
 * @brief  Malloc aligned memory for video codec
 *
 * @note  Most of the hardware codec require special alignment for input or output frame memory
 *        This API can be used to allocate aligned memory according to the requirement
 *        Actual allocated size may be more than the requested size, it's also returned to user
 *
 * @param[in]   align        Alignment requirement
 * @param[in]   size         Size to be allocated
 * @param[out]  actual_size  Actual allocated size
 *
 * @return
 *       - NULL    Fail to allocate
 *       - Others  On success
 */
void *esp_video_codec_align_alloc(uint8_t align, uint32_t size, uint32_t *actual_size);

/**
 * @brief  Free allocated memory for video codec
 *
 * @param[in]  ptr  Memory to be freed
 */
void esp_video_codec_free(void *ptr);

/**
 * @brief  Get string representing of pixel format
 *
 * @param[in]  fmt  Pixel format enumeration
 *
 * @return
 *       - "none"  Invalid pixel format
 *       - Others  Pixel format string representing
 */
const char *esp_video_codec_get_pixel_fmt_str(esp_video_codec_pixel_fmt_t fmt);

/**
 * @brief  Get string representing of codec type
 *
 * @param[in]  codec  Video codec type
 *
 * @return
 *       - "none"  Invalid video codec type
 *       - Others  Video codec string representing
 */
const char *esp_video_codec_get_codec_str(esp_video_codec_type_t codec);

/**
 * @brief  Get image size of given pixel format and resolution
 *
 * @param[in]  fmt         Image pixel format
 * @param[in]  resolution  Image resolution
 *
 * @return
 *       - 0       Invalid pixel format or resolution
 *       - Others  Image size
 */
uint32_t esp_video_codec_get_image_size(esp_video_codec_pixel_fmt_t fmt, esp_video_codec_resolution_t *resolution);

#ifdef __cplusplus
}
#endif
