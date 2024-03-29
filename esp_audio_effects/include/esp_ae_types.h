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

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_AE_BIT8  (8)
#define ESP_AE_BIT16 (16)
#define ESP_AE_BIT24 (24)
#define ESP_AE_BIT32 (32)

/**
 * @brief  Type definition for audio data samples
 */
typedef void *esp_ae_sample_t;

/**
 * @brief  Error type definition of audio effects
 */
typedef enum {
    ESP_AE_ERR_OK                = 0,   /*!< Operation succeeded */
    ESP_AE_ERR_FAIL              = -1,  /*!< Operation failed */
    ESP_AE_ERR_MEM_LACK          = -2,  /*!< Memory allocation failure */
    ESP_AE_ERR_INVALID_PARAMETER = -3,  /*!< Invalid input parameter */
    ESP_AE_ERR_NOT_SUPPORT       = -4,  /*!< Unsupported type */
} esp_ae_err_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
