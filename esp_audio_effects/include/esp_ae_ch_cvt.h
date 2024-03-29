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
 * @brief  The channel conversion module is used to convert the original audio channel into
 *         the target channel. The implementation algorithm is to multiply the input data
 *         by a given weight coefficient array to get the output data.
 *
 *         Following example shows the conversion process of a 3-channel audio input to a 2-channel audio output:
 *         The A1, A2, and A3 means three-channel input data, the B1 and B2 means two-channel output data,
 *         Wx means weight coefficient.
 *         The algorithm representation is
 *         B1 = W1 * A1 + W2 * A2 + W3 * A3;
 *         B2 = W4 * A1 + W5 * A2 + W6 * A3;
 *         E.g. The weight coefficient is {0.5, 0, 0.5, 0, 0.6, 0.4}
 *         B1 = 0.5 * A1 + 0 * A2 + 0.5 * A3;
 *         B2 = 0 * A1 + 0.6 * A2 + 0.4 * A3;
 *         Note:
 *         1. If user not set weight coefficient array, all the weight value will be set to 1/src_ch_num.
 *
 *         Channel conversion processing is based on sampling points as processing units.
 *         The relationship between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Channel conversion also offers two processing interfaces to fulfill the needs of
 *         interleaved and deinterleaved input data layouts.
 */

/**
 * @brief  The handle of channel conversion
 */
typedef void *esp_ae_ch_cvt_handle_t;

/**
 * @brief  Configuration structure for channel conversion
 */
typedef struct {
    uint32_t sample_rate;      /*!< The audio sample rate */
    uint8_t  bits_per_sample;  /*!< The audio bits per sample, supports 16, 24, 32 bits */
    uint8_t  src_ch;           /*!< Channel number for input source stream */
    uint8_t  dest_ch;          /*!< Channel number for output source stream */
    float   *weight;           /*!< The weight coefficient array
                                    Note:
                                    1. When set to NULL, the default setting of weight is 1/src_ch_num for each channel.
                                    2. If the src_ch is equal to dest_ch, channel conversion processing
                                       will be in the form of bypass.
                               */
    uint32_t weight_len;       /*!< The length of weight coefficient array. If weight not set to null,
                                    `weight_len` will set to `src_ch * dest_ch`, otherwise set to 0 */
} esp_ae_ch_cvt_cfg_t;

/**
 * @brief  Create an channel conversion handle based on the provided configuration
 *
 * @param[in]   cfg     Pointer to the channel conversion configuration
 * @param[out]  handle  The channel conversion handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_ch_cvt_open(esp_ae_ch_cvt_cfg_t *cfg, esp_ae_ch_cvt_handle_t *handle);

/**
 * @brief  Perform channel conversion on interleaved audio data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The channel conversion handle
 * @param[in]   sample_num   Number of sampling points processed by channel conversion
 * @param[in]   in_samples   Input sample buffer. The size of `in_samples` must be greater than
 *                           or equal to `sample_num * src_channel * bit >> 3`
 * @param[out]  out_samples  Output samples buffer. The size of `out_samples` must be greater than
 *                           or equal to `sample_num * dest_channel * bit >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_ch_cvt_process(esp_ae_ch_cvt_handle_t handle, uint32_t sample_num,
                                   esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Perform channel conversion on deinterleaved audio data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]   handle       The channel convert handle
 * @param[in]   sample_num   Number of sampling points processed by channel conversion
 * @param[in]   in_samples   Input sample buffer array. The size of every `in_samples buffer`
 *                           must be greater than or equal to `sample_num * src_channel * bit >> 3`
 * @param[out]  out_samples  Output samples buffer array. The size of every `out_samples buffer`
 *                           must be greater than or equal to `sample_num * dest_channel * bit >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_ch_cvt_deintlv_process(esp_ae_ch_cvt_handle_t handle, uint32_t sample_num,
                                           esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Deinitialize channel conversion handle
 *
 * @param  handle  The channel convert handle
 */
void esp_ae_ch_cvt_close(esp_ae_ch_cvt_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
