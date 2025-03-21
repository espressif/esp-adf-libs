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

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 *  Features:
 *     Rotation
 *         - Supported bypass
 *         - Supported arbitrary input resolutions
 *         - Supported clockwise rotation at any degree
 *         - Supported ESP_IMG_PIXEL_FMT_Y/RGB565/BGR565/RGB888/BGR888/YUV_PACKET
 *         - Supported faster clockwise rotation algorithm for specific degrees formats resolutions
 *
 *     Scale
 *        - Supported bypass
 *        - Supported arbitrary input resolutions
 *        - Supported both up-scaling and down-scaling operations
 *        - Supported ESP_IMG_PIXEL_FMT_RGB565/BGR565/RGB888/BGR888/YUV_PACKET
 *        - Supported multiple filter algorithms: optimized down-sampling and bilinear interpolation
 *
 *     Crop
 *        - Supported bypass
 *        - Supported arbitrary input resolutions
 *        - Supported flexible region selection
 *        - Supported ESP_IMG_PIXEL_FMT_Y/RGB565/BGR565/RGB888/BGR888/YUV_PACKET
 *
 *     Color convert
 *        - Supported arbitrary input resolutions
 *        - Supported bypass mode for identical input/output formats
 *        - Supports BT.601/BT.709/BT.2020 color space standards
 *        - Supported faster color convert algorithm for formats and resolutions
 *        - Comprehensive format support matrix:
 *          ------------------------------------------------------------------------------------------
 *          | Input Format               | Supported Output Formats                                  |
 *          ------------------------------------------------------------------------------------------
 *          | RGB565_LE/BGR565_LE        | RGB565_LE/BGR565_LE/RGB565_BE/BGR565_BE/RGB888/BGR888/    |
 *          | RGB565_BE/BGR565_BE        | YUV_PLANAR/YUV_PACKET/YUYV/UYVY/O_UYY_E_VYY/I420          |
 *          | RGB888/BGR888              |                                                           |
 *          ------------------------------------------------------------------------------------------
 *          | ARGB888/ABGR888            | RGB565_LE/BGR565_LE/RGB565_BE/BGR565_BE/RGB888/BGR888/    |
 *          |                            | YUV_PLANAR/O_UYY_E_VYY/I420                               |
 *          ------------------------------------------------------------------------------------------
 *          | YUV_PACKET/UYVY/YUYV       | RGB565_LE/BGR565_LE/RGB565_BE/BGR565_BE/RGB888/BGR888/    |
 *          |                            | O_UYY_E_VYY/I420                                          |
 *          ------------------------------------------------------------------------------------------
 *          | O_UYY_E_VYY/I420           | RGB565_LE/BGR565_LE/RGB565_BE/BGR565_BE/RGB888/BGR888/    |
 *          |                            | O_UYY_E_VYY                                               |
 *          ------------------------------------------------------------------------------------------
 *
 *  Release Notes:
 *     v1.0.0:
 *     - Add rotation, scale, crop, color convert module
 */

/**
 * @brief  Get image process version string
 *
 */
const char *esp_imgfx_get_version(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
