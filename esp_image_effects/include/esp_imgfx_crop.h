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

#include <string.h>
#include <stdint.h>
#include "esp_imgfx_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Cropping is the process of extracting a target area from an original image by setting the boundaries of a rectangular region
 *         (starting position, width, and height of the rectangle). The cropped area is then output as a new image. The resolution of the
 *         cropped image is determined by the width and height of the rectangular. It supports various image formats and resolutions, and
 *         can be used to perform cropping operations on images in real-time
 *
 *         1. Please note that the functions in this module are not thread-safe. Users must implement external locking mechanisms
 *         when using them in multi-threaded environments to maintain data consistency
 *         2. For optimal performance, it is recommended that data addresses be aligned with the chip’s cache line
 */

/**
 * @brief  Handle for image cropping operations
 */
typedef void *esp_imgfx_crop_handle_t;

/**
 * @brief  Image cropping configuration
 *         Visual Representation:
 *         Input Image Dimensions
 *         --------------------------------------------------------------
 *         |           ↑                                                |
 *         |           | y_pos                                          |
 *         |           ↓                                                |
 *         | ← x_pos → --------------------                             |
 *         |           |                  | cropped_res.height          |
 *         |           |                  |                             |
 *         |           --------------------                             |
 *         |              cropped_res.width                             |
 *         --------------------------------------------------------------
 */
typedef struct {
    esp_imgfx_resolution_t in_res;        /*!< Source image resolution */
    esp_imgfx_pixel_fmt_t  in_pixel_fmt;  /*!< Source pixel format */
    esp_imgfx_resolution_t cropped_res;   /*!< Cropped output resolution */
    uint16_t               x_pos;         /*!< Horizontal start position (pixels) */
    uint16_t               y_pos;         /*!< Vertical start position (pixels) */
} esp_imgfx_crop_cfg_t;

/**
 * @brief  Initialize cropping processor instance
 *         Returns error if crop area exceeds image boundaries
 *
 * @param[in]   cfg     Cropping configuration parameters
 * @param[out]  handle  Output cropping handle
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid configuration
 *       - ESP_IMGFX_ERR_NOT_SUPPORTED      Unsupported pixel format
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory
 */
esp_imgfx_err_t esp_imgfx_crop_open(esp_imgfx_crop_cfg_t *cfg, esp_imgfx_crop_handle_t *handle);

/**
 * @brief  Get current cropping configuration
 *
 * @param[in]   handle  Initialized crop handle
 * @param[out]  cfg     Destination configuration buffer
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid handle or NULL pointer
 */
esp_imgfx_err_t esp_imgfx_crop_get_cfg(esp_imgfx_crop_handle_t handle, esp_imgfx_crop_cfg_t *cfg);

/**
 * @brief  Update cropping configuration
 *
 * @note  This function is not thread-safe and must be externally synchronized in multi-threaded environments
 *
 * @code{c}
 *
 *           // Example configuration update:
 *           esp_imgfx_crop_cfg_t cfg;
 *           esp_imgfx_crop_get_cfg(handle, &cfg);
 *           cfg.cropped_res.width = 100; // Modify crop width
 *           ESP_ERROR_CHECK(esp_imgfx_crop_set_cfg(handle, &cfg));
 *           esp_imgfx_data_t in = {.data = input_buf, .data_len = in_size};
 *           esp_imgfx_data_t out = {.data = output_buf, .data_len = out_size};
 *           ESP_ERROR_CHECK(esp_imgfx_crop_process(handle, &in, &out));
 *
 * @endcode
 *
 * @param[in]  handle  Initialized crop handle
 * @param[in]  cfg     New configuration parameters
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory for configuration
 */
esp_imgfx_err_t esp_imgfx_crop_set_cfg(esp_imgfx_crop_handle_t handle, esp_imgfx_crop_cfg_t *cfg);

/**
 * @brief  Execute image cropping operation
 *         It will process the input image and store the result in the output buffer by the last configuration
 *
 * @note  1.This function is not thread-safe and must be externally synchronized in multi-threaded environments
 *        2.For better performance, it is recommended that the input and output image addresses be aligned with the cache line
 *        3.It is recommended to use `esp_imgfx_get_image_size()` to determine the required buffer size
 *
 * @code{c}
 *
 *           // Example processing with error handling:
 *           esp_imgfx_data_t in = {.data = input_buf, .data_len = in_size};
 *           esp_imgfx_data_t out = {.data = output_buf, .data_len = out_size};
 *           esp_imgfx_err_t ret = esp_imgfx_crop_process(handle, &in, &out);
 *           if (ret == ESP_IMGFX_ERR_BUFF_NOT_ENOUGH) {
 *               // Please check output buffer and its size or configuration
 *           } else if (ret == ESP_IMGFX_ERR_DATA_LACK) {
 *                // Please check input buffer and its size or configuration
 *           }
 *
 * @endcode
 *
 * @param[in]   handle     Initialized crop handle
 * @param[in]   in_image   Input image data (cache-aligned recommended)
 * @param[out]  out_image  Output buffer (cache-aligned recommended)
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_DATA_LACK          Input buffer size mismatch
 *       - ESP_IMGFX_ERR_BUFF_NOT_ENOUGH    Output buffer size mismatch
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 */
esp_imgfx_err_t esp_imgfx_crop_process(esp_imgfx_crop_handle_t handle, esp_imgfx_data_t *in_image, esp_imgfx_data_t *out_image);

/**
 * @brief  Release cropping resources
 *         If handle is NULL, it does nothing, and returns ESP_IMGFX_ERR_OK
 *
 * @param[in]  handle  Crop handle to destroy
 *
 * @return
 *       - ESP_IMGFX_ERR_OK  Success
 */
esp_imgfx_err_t esp_imgfx_crop_close(esp_imgfx_crop_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
