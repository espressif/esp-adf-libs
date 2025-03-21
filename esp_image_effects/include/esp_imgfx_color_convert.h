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
 */

#pragma once

#include <stdint.h>
#include "esp_imgfx_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  This module provides conversion capabilities between various color formats. For example, it supports converting
 *         RGB to planar YUV, RGB to packet YUV, and YUV to RGB. It also handles color space conversions for different color
 *         formats, with current support for the BT601, BT709, and BT2020 color spaces
 *
 *         1. Functions in this module are not thread-safe. Users must implement external locking mechanisms
 *         when using them in multi-threaded environments to maintain data consistency
 *         2. For optimal performance, data addresses should be aligned with the chip’s cache line
 *         3. The ESP32P4 offers assembly-optimized paths for certain conversion routines:
 *               * I420, YUYV, UYVY, YUV_PACKET, and O_UYY_E_VYY formats (requires 16-pixel alignment)
 *               * General cases (requires 32-pixel alignment)
 */

/**
 * @brief  Handle for color conversion operations
 */
typedef void *esp_imgfx_color_convert_handle_t;

/**
 * @brief  Color space standards
 */
typedef enum {
    ESP_IMGFX_COLOR_SPACE_STD_BT601,   /*!< ITU-R BT.601 color space (SDTV) */
    ESP_IMGFX_COLOR_SPACE_STD_BT709,   /*!< ITU-R BT.709 color space (HDTV) */
    ESP_IMGFX_COLOR_SPACE_STD_BT2020,  /*!< ITU-R BT.2020 color space (UHDTV) */
} esp_imgfx_color_space_std_t;

/**
 * @brief  Color conversion configuration
 */
typedef struct {
    esp_imgfx_resolution_t      in_res;           /*!< Source image resolution */
    esp_imgfx_pixel_fmt_t       in_pixel_fmt;     /*!< Source pixel format (see format matrix) */
    esp_imgfx_pixel_fmt_t       out_pixel_fmt;    /*!< Target pixel format (see format matrix) */
    esp_imgfx_color_space_std_t color_space_std;  /*!< Color space conversion standard */
} esp_imgfx_color_convert_cfg_t;

/**
 * @brief  Initialize color conversion processor
 *
 * @param[in]   cfg     Conversion configuration parameters
 * @param[out]  handle  Output conversion handle
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid configuration
 *       - ESP_IMGFX_ERR_NOT_SUPPORTED      Unsupported format combination or color space standard
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory
 */
esp_imgfx_err_t esp_imgfx_color_convert_open(esp_imgfx_color_convert_cfg_t *cfg, esp_imgfx_color_convert_handle_t *handle);

/**
 * @brief  Get current conversion configuration
 *
 * @param[in]   handle  Initialized conversion handle
 * @param[out]  cfg     Destination configuration buffer
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid handle or NULL pointer
 */
esp_imgfx_err_t esp_imgfx_color_convert_get_cfg(esp_imgfx_color_convert_handle_t handle, esp_imgfx_color_convert_cfg_t *cfg);

/**
 * @brief  Update conversion configuration
 *
 * @note  This function is not thread-safe and must be externally synchronized in multi-threaded environments
 *
 * @code{c}
 *
 *           // Example configuration update:
 *           esp_imgfx_color_convert_cfg_t cfg;
 *           esp_imgfx_color_convert_get_cfg(handle, &cfg);
 *           cfg.out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888;
 *           ESP_ERROR_CHECK(esp_imgfx_color_convert_set_cfg(handle, &cfg));
 *           esp_imgfx_data_t in = {.data = input_buf, .data_len = in_size};
 *           esp_imgfx_data_t out = {.data = output_buf, .data_len = out_size};
 *           ESP_ERROR_CHECK(esp_imgfx_color_convert_process(handle, &in, &out));
 *
 * @endcode
 *
 * @param[in]  handle  Initialized conversion handle
 * @param[in]  cfg     New configuration parameters
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory for reconfiguration
 */
esp_imgfx_err_t esp_imgfx_color_convert_set_cfg(esp_imgfx_color_convert_handle_t handle, esp_imgfx_color_convert_cfg_t *cfg);

/**
 * @brief  Execute color conversion
 *         It will process the input image and store the result in the output buffer by the last configuration
 *
 * @note  1.This function is not thread-safe and must be externally synchronized in multi-threaded environments
 *        2.For better performance, it is recommended that the input and output image addresses be aligned with the cache line
 *        3.It is recommended to use `esp_imgfx_get_image_size()` to determine the required buffer size
 *
 * @code{c}
 *
 *           // Example processing:
 *           esp_imgfx_data_t in = {.data = input_buf, .data_len = in_size};
 *           esp_imgfx_data_t out = {.data = output_buf, .data_len = out_size};
 *           esp_imgfx_err_t ret = esp_imgfx_color_convert_process(handle, &in, &out);
 *           if (ret == ESP_IMGFX_ERR_BUFF_NOT_ENOUGH) {
 *               // Please check output buffer and its size or configuration
 *           } else if (ret == ESP_IMGFX_ERR_DATA_LACK) {
 *                // Please check input buffer and its size or configuration
 *           }
 *
 * @endcode
 *
 * @param[in]   handle     Initialized conversion handle
 * @param[in]   in_image   Input image data (cache-aligned recommended)
 * @param[out]  out_image  Output buffer (cache-aligned recommended)
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_DATA_LACK          Input buffer size mismatch
 *       - ESP_IMGFX_ERR_BUFF_NOT_ENOUGH    Output buffer size mismatch
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 */
esp_imgfx_err_t esp_imgfx_color_convert_process(esp_imgfx_color_convert_handle_t handle, esp_imgfx_data_t *in_image, esp_imgfx_data_t *out_image);

/**
 * @brief  Release conversion resources
 *         If handle is NULL, it does nothing, and returns ESP_IMGFX_ERR_OK
 *
 * @param[in]  handle  Conversion handle to destroy
 *
 * @return
 *       - ESP_IMGFX_ERR_OK  Success
 */
esp_imgfx_err_t esp_imgfx_color_convert_close(esp_imgfx_color_convert_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
