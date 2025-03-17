/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Macro to create FourCC (Four Character Code) from characters
 */
#define ESP_IMGFX_FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/**
 * @brief  Error codes for image processing operations
 */
typedef enum {
    ESP_IMGFX_ERR_OK                = 0,   /*!< Operation successful */
    ESP_IMGFX_ERR_FAIL              = -1,  /*!< General failure */
    ESP_IMGFX_ERR_MEM_LACK          = -2,  /*!< Insufficient memory */
    ESP_IMGFX_ERR_INVALID_PARAMETER = -3,  /*!< Invalid parameters provided */
    ESP_IMGFX_ERR_NOT_SUPPORTED     = -4,  /*!< Operation/format not supported */
    ESP_IMGFX_ERR_BUFF_NOT_ENOUGH   = -5,  /*!< Insufficient output buffer size */
    ESP_IMGFX_ERR_DATA_LACK         = -6,  /*!< Insufficient input buffer size */
} esp_imgfx_err_t;

/**
 * @brief  Image pixel formats
 * @note  Detailed pixel arrangement reference:
 *        https://github.com/espressif/esp-gmf/blob/main/gmf_core/helpers/include/esp_fourcc.h
 */
typedef enum {
    ESP_IMGFX_PIXEL_FMT_Y           = ESP_IMGFX_FOURCC('G', 'R', 'E', 'Y'),  /*!< 8 bpp Greyscale */
    ESP_IMGFX_PIXEL_FMT_I420        = ESP_IMGFX_FOURCC('Y', 'U', '1', '2'),  /*!< 12 bpp YUV 4:2:0 planar, 1 UV pair per 2x2 pixel block */
    ESP_IMGFX_PIXEL_FMT_O_UYY_E_VYY = ESP_IMGFX_FOURCC('O', 'U', 'E', 'V'),  /*!< 12 bpp Espressif Y-U-V 4:2:0 format */
    ESP_IMGFX_PIXEL_FMT_RGB565_LE   = ESP_IMGFX_FOURCC('R', 'G', 'B', 'L'),  /*!< 16 bpp RGB-565 little-endian */
    ESP_IMGFX_PIXEL_FMT_BGR565_LE   = ESP_IMGFX_FOURCC('B', 'G', 'R', 'L'),  /*!< 16 bpp BGR-565 little-endian */
    ESP_IMGFX_PIXEL_FMT_RGB565_BE   = ESP_IMGFX_FOURCC('R', 'G', 'B', 'B'),  /*!< 16 bpp RGB-565 big-endian */
    ESP_IMGFX_PIXEL_FMT_BGR565_BE   = ESP_IMGFX_FOURCC('B', 'G', 'R', 'B'),  /*!< 16 bpp BGR-565 big-endian */
    ESP_IMGFX_PIXEL_FMT_YUYV        = ESP_IMGFX_FOURCC('Y', 'U', 'Y', 'V'),  /*!< 16 bpp YUYV 4:2:2 packed */
    ESP_IMGFX_PIXEL_FMT_UYVY        = ESP_IMGFX_FOURCC('U', 'Y', 'V', 'Y'),  /*!< 16 bpp UYVY 4:2:2 packed */
    ESP_IMGFX_PIXEL_FMT_RGB888      = ESP_IMGFX_FOURCC('R', 'G', 'B', '3'),  /*!< 24 bpp RGB888 */
    ESP_IMGFX_PIXEL_FMT_BGR888      = ESP_IMGFX_FOURCC('B', 'G', 'R', '3'),  /*!< 24 bpp BGR888 */
    ESP_IMGFX_PIXEL_FMT_YUV_PLANNER = ESP_IMGFX_FOURCC('4', '4', '4', 'P'),  /*!< 24 bpp YUV444 planar */
    ESP_IMGFX_PIXEL_FMT_YUV_PACKET  = ESP_IMGFX_FOURCC('V', '3', '0', '8'),  /*!< 24 bpp YUV444 packed */
    ESP_IMGFX_PIXEL_FMT_ARGB888     = ESP_IMGFX_FOURCC('A', 'B', '2', '4'),  /*!< 32 bpp ARGB8888 */
    ESP_IMGFX_PIXEL_FMT_ABGR888     = ESP_IMGFX_FOURCC('A', 'R', '2', '4'),  /*!< 32 bpp ABGR8888 */
} esp_imgfx_pixel_fmt_t;

/**
 * @brief  Image resolution configuration
 */
typedef struct {
    int16_t width;   /*!< Image width in pixels */
    int16_t height;  /*!< Image height in pixels */
} esp_imgfx_resolution_t;

/**
 * @brief  Structure to hold image data and its length
 */
typedef struct {
    uint8_t *data;      /*!< Pointer to the image data buffer */
    uint32_t data_len;  /*!< Length of the image data in bytes */
} esp_imgfx_data_t;

/**
 * @brief  Get bits per pixel for specified pixel format
 *
 * @param[in]   pixel_fmt       Target pixel format
 * @param[out]  bits_per_pixel  Output bits per pixel value
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Operation successful
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid pixel format or bits_per_pixel is NULL
 */
esp_imgfx_err_t esp_imgfx_get_bits_per_pixel(esp_imgfx_pixel_fmt_t pixel_fmt, uint16_t *bits_per_pixel);

/**
 * @brief  Get image size in byte for specified pixel format and resolution
 *
 * @param[in]   pixel_fmt   Target pixel format
 * @param[in]   res         Image resolution
 * @param[out]  image_size  Output image size value. unit byte
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Operation successful
 *       - ESP_IMGFX_ERR_NOT_SUPPORTED      Unsupported format combination
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid pixel format or image_size is NULL
 */
esp_imgfx_err_t esp_imgfx_get_image_size(esp_imgfx_pixel_fmt_t pixel_fmt, const esp_imgfx_resolution_t *res, uint32_t *image_size);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
