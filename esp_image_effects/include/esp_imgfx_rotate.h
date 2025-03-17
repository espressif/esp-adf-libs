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
 * @brief  Image rotation refers to the process of transforming an image by rotating it around its center point by a specified angle.
 *         This adjusts the display orientation of the image by altering the spatial positions of its pixels. The rotated image may
 *         result in blank areas (which retain their original values) or exceed the boundaries of the original image (necessitating
 *         adjustments to the output size). Standard angles(0°, 90°, 180°, 270°, 360° and so on) allow for lossless rotation. For standard
 *         angles, it optimized with assembly code and algorithm optimization. This design is based on clockwise rotation. If you want to
 *         rotate counterclockwise, the rotation angle is 360° minus the rotation angle. For example, if you need to counterclockwise 60°,
 *         you can set the `degree` in `esp_imgfx_rotate_cfg_t` equal to 360-60
 *
 *         1. Please note that the functions in this module are not thread-safe. Users must implement external locking mechanisms
 *         when using them in multi-threaded environments to maintain data consistency
 *         2. For optimal performance, it is recommended that data addresses be aligned with the chip’s cache line
 *         3. Accelerated rotation for standard angles when:
 *               * Image dimensions are multiples of 8
 *               * Pixel format is Y/RGB565 variants
 *         4. ESP32P4-specific optimizations:
 *               * 0°: Bypass mode with 16-pixel aligned dimensions (assembly optimized)
 *               * 90°/270°: 64-pixel aligned dimensions (assembly optimized)
 *               * 180°: 16-pixel aligned dimensions (assembly optimized)
 */

/**
 * @brief  Handle for image rotation operations
 */
typedef void *esp_imgfx_rotate_handle_t;

/**
 * @brief  Image rotation configuration
 */
typedef struct {
    esp_imgfx_resolution_t in_res;        /*!< Source image resolution */
    esp_imgfx_pixel_fmt_t  in_pixel_fmt;  /*!< Source image pixel format */
    uint16_t               degree;        /*!< Clockwise rotation angle in degrees. It range is from 0° to maximun value of uint16_t,
                                               (one degree per unit) */
} esp_imgfx_rotate_cfg_t;

/**
 * @brief  Initialize rotation processor instance
 *
 * @param[in]   cfg     Rotation configuration parameters
 * @param[out]  handle  Output rotation handle
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid configuration
 *       - ESP_IMGFX_ERR_NOT_SUPPORTED      Unsupported pixel format
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory
 */
esp_imgfx_err_t esp_imgfx_rotate_open(esp_imgfx_rotate_cfg_t *cfg, esp_imgfx_rotate_handle_t *handle);

/**
 * @brief  Get current rotation configuration
 *
 * @param[in]   handle  Initialized rotation handle
 * @param[out]  cfg     Destination configuration buffer
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid handle or NULL pointer
 */
esp_imgfx_err_t esp_imgfx_rotate_get_cfg(esp_imgfx_rotate_handle_t handle, esp_imgfx_rotate_cfg_t *cfg);

/**
 * @brief  Update rotation configuration
 *
 * @note  This function is not thread-safe and must be externally synchronized in multi-threaded environments
 *
 * @code{c}
 *
 *           //Example configuration update:
 *           esp_imgfx_rotate_cfg_t cfg;
 *           esp_imgfx_rotate_get_cfg(handle, &cfg);
 *           cfg.degree = 90; // Modify rotation angle
 *           ESP_ERROR_CHECK(esp_imgfx_rotate_set_cfg(handle, &cfg));
 *           esp_imgfx_data_t in = {.data = input_buf, .data_len = in_size};
 *           esp_imgfx_data_t out = {.data = output_buf, .data_len = out_size};
 *           ESP_ERROR_CHECK(esp_imgfx_rotate_process(handle, &in, &out));
 *
 * @endcode
 *
 * @param[in]  handle  Initialized rotation handle
 * @param[in]  cfg     New configuration parameters
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 *       - ESP_IMGFX_ERR_MEM_LACK           Insufficient memory for configuration
 */
esp_imgfx_err_t esp_imgfx_rotate_set_cfg(esp_imgfx_rotate_handle_t handle, esp_imgfx_rotate_cfg_t *cfg);

/**
 * @brief  Calculate post-rotation output resolution
 *         eg: if input resolution is 1920x1080 and rotation angle is 90°, the output resolution will be 1080x1920
 *
 * @param[in]   handle  Initialized rotation handle
 * @param[out]  res     Calculated output resolution
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid handle or pointer
 */
esp_imgfx_err_t esp_imgfx_rotate_get_rotated_resolution(esp_imgfx_rotate_handle_t handle, esp_imgfx_resolution_t *res);

/**
 * @brief  Execute image rotation
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
 *           esp_imgfx_err_t ret = esp_imgfx_rotate_process(handle, &in, &out);
 *           if (ret == ESP_IMGFX_ERR_BUFF_NOT_ENOUGH) {
 *               // Please check output buffer and its size or configuration
 *           } else if (ret == ESP_IMGFX_ERR_DATA_LACK) {
 *                // Please check input buffer and its size or configuration
 *           }
 *
 * @endcode
 *
 * @param[in]   handle     Initialized scaling handle
 * @param[in]   in_image   Input image data (cache-aligned recommended)
 * @param[out]  out_image  Output image buffer (cache-aligned recommended)
 *
 * @return
 *       - ESP_IMGFX_ERR_OK                 Success
 *       - ESP_IMGFX_ERR_DATA_LACK          Input buffer size mismatch
 *       - ESP_IMGFX_ERR_BUFF_NOT_ENOUGH    Output buffer size mismatch
 *       - ESP_IMGFX_ERR_INVALID_PARAMETER  Invalid parameters
 */
esp_imgfx_err_t esp_imgfx_rotate_process(esp_imgfx_rotate_handle_t handle, esp_imgfx_data_t *in_image, esp_imgfx_data_t *out_image);

/**
 * @brief  Release rotation resources
 *         If handle is NULL, it does nothing, and returns ESP_IMGFX_ERR_OK
 *
 * @param[in]  handle  Rotation handle to destroy
 *
 * @return
 *       - ESP_IMGFX_ERR_OK  Success
 */
esp_imgfx_err_t esp_imgfx_rotate_close(esp_imgfx_rotate_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
