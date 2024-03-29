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
 * @brief  The data weaver module is used to change the layout of data
 */

/**
 * @brief  Split interleaved audio data and save the data of each channel to its corresponding buffer.
 *         eg: A two channel audio with interleaved data, arranged as follow in `in_samples`:
 *         01010101..., where 0 represents the first channel data, 1 represents the second channel data.
 *         After deinterleaved process, all the first channel data is stored in `out_samples[0]` and
 *         all the second channel data is stored in `out_samples[1]`.
 *
 * @param[in]   channel          The audio stream channel number
 * @param[in]   bits_per_sample  The audio bits per sample, supports 16, 24, 32 bits
 * @param[in]   sample_num       Number of sampling points to be processed during deinterleaving
 * @param[in]   in_samples       Input sample buffer. The size of `in_samples` must be greater than
 *                               or equal to `sample_num * channel * bit >> 3`
 * @param[out]  out_samples      Output samples buffer array. The size of every `out_samples buffer`
 *                               must be greater than or equal to `sample_num * channel * bit >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_deintlv_process(uint8_t channel, uint8_t bits_per_sample,
                                    uint32_t sample_num, esp_ae_sample_t in_samples,
                                    esp_ae_sample_t out_samples[]);

/**
 * @brief  Interleave the data of each independent channel together into one buffer
 *         eg: A two channel audio with deinterleaved data, stored as follow in `in_samples`:
 *         all the first channel data is stored in `in_samples[0]` and
 *         all the second channel data is stored in `in_samples[1]`.
 *         After interleaved process, the data is stored as `01010101...` into `out_samples`.
 *
 * @param[in]   channel          The audio stream channel number
 * @param[in]   bits_per_sample  The audio bits per sample, supports 16 and 32 bits
 * @param[in]   sample_num       Number of sampling points processed during interleaving
 * @param[in]   in_samples       Input sample buffer array. The size of every `in_samples` must
 *                               be greater than or equal to `sample_num * channel * bit >> 3`
 * @param[out]  out_samples      Output samples buffer. The size of `out_samples` must
 *                               be greater than or equal to `sample_num * channel * bit >> 3`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_intlv_process(uint8_t channel, uint8_t bits_per_sample,
                                  uint32_t sample_num, esp_ae_sample_t in_samples[],
                                  esp_ae_sample_t out_samples);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
