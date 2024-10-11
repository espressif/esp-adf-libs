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
#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  The bit conversion module is used to change the audio bit depth from src_bits to dst_bit.
 *         Supported formats:
 *         - Little endian only
 *         - Unsigned 8 bits, signed 16, 24, 32 bits
 *
 *         Bit conversion processing is based on sampling points as processing units.
 *         The relationship between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Bit conversion also offers two processing interfaces to fulfill the needs of
 *         interleaved and deinterleaved input data layouts.
 */

/**
 * @brief  Handle for bit conversion
 */
typedef void *esp_ae_bit_cvt_handle_t;

/**
 * @brief  Configuration structure for bit conversion
 */
typedef struct {
    uint32_t sample_rate;  /*!< The audio sample rate */
    uint8_t  channel;      /*!< The audio channel number */
    uint8_t  src_bits;     /*!< Audio sample bits of source stream, supports unsigned 8 bits and
                                signed 16, 24, 32 bits */
    uint8_t  dest_bits;    /*!< Audio sample bits of dest stream, supports unsigned 8 bits and
                                signed 16, 24, 32 bits */
} esp_ae_bit_cvt_cfg_t;

/**
 * @brief  Create a bit conversion handle based on the provided configuration
 *
 * @param[in]   cfg     Pointer to the bit conversion configuration
 * @param[out]  handle  The bit conversion handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_bit_cvt_open(esp_ae_bit_cvt_cfg_t *cfg, esp_ae_bit_cvt_handle_t *handle);

/**
 * @brief  Perform bit conversion on interleaved audio data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The bit conversion handle
 * @param[in]   sample_num   Number of sampling points processed by bit conversion
 * @param[in]   in_samples   Input sample buffer. The size of `in_samples` must be greater than
 *                           or equal to `sample_num * channel * src_bits >> 3`
 * @param[out]  out_samples  Output samples buffer. The size of `out_samples` must be greater than
 *                           or equal to `sample_num * channel * dest_bits >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_bit_cvt_process(esp_ae_bit_cvt_handle_t handle, uint32_t sample_num,
                                    esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Perform bit conversion on deinterleaved audio data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]   handle       The bit conversion handle
 * @param[in]   sample_num   Number of sampling points processed by bit conversion
 * @param[in]   in_samples   Input sample buffer array. The size of every `in_samples buffer` must
 *                           be greater than or equal to `sample_num * src_bits >> 3`
 * @param[out]  out_samples  Output samples buffer array. The size of every `out_samples buffer` must
 *                           be greater than or equal to `sample_num * dest_bits >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_bit_cvt_deintlv_process(esp_ae_bit_cvt_handle_t handle, uint32_t sample_num,
                                            esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Deinitialize bit conversion handle
 *
 * @param  handle  The bit conversion handle
 */
void esp_ae_bit_cvt_close(esp_ae_bit_cvt_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
