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
 * @brief  Automatic Level Control (ALC) is an audio processing technology designed
 *         to automatically adjust the volume level of audio signals, ensuring a
 *         stable output volume across different input signal levels. The way ALC works
 *         is by continuously monitoring the audio signal's amplitude and dynamically
 *         adjusting the gain to keep the signal within a predetermined range.
 *
 *         If the user wants to adjust the volume of a certain channel, can call the
 *         `esp_ae_alc_set_gain` interface. Audio gain refers to the amplification or
 *         reduction of an audio signal to adjust the volume level of the audio.
 *
 *         Gain is usually expressed in decibels (dB), with positive gain indicating
 *         amplification, negative gain indicating reduction and 0 gain indicating
 *         volume remains unchanged.
 *
 *         ALC processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3)).
 *
 *         ALC also offers two processing interfaces to fulfill the needs of interleaved
 *         and deinterleaved input data layouts.
 */

/**
 * @brief  Handle for Automatic Level Control (ALC)
 */
typedef void *esp_ae_alc_handle_t;

/**
 * @brief  Configuration structure for Automatic Level Control (ALC)
 */
typedef struct {
    uint32_t sample_rate;      /*!< The audio sample rate */
    uint8_t  channel;          /*!< The audio channel number */
    uint8_t  bits_per_sample;  /*!< The audio bits per sample, supports 16, 24, 32 bits */
} esp_ae_alc_cfg_t;

/**
 * @brief  Create an ALC handle based on the provided configuration
 *
 * @param[in]   cfg     ALC configuration
 * @param[out]  handle  The ALC handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Failed to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_alc_open(esp_ae_alc_cfg_t *cfg, esp_ae_alc_handle_t *handle);

/**
 * @brief  Set the gain for a specific channel in the ALC handle.
 *         Positive gain indicates an increase in volume,
 *         negative gain indicates a decrease in volume.
 *         0 gain indicates the volume level remains unchanged.
 *
 * @param[in]  handle  The ALC handle
 * @param[in]  ch_idx  The channel index of the gain to retrieve. eg: 0 refers to the first channel
 * @param[in]  gain    The gain value needs to conform to the following conditions:
 *                     - Supported range [-64, 63]
 *                     - Below -64 will set to mute
 *                     - Higher than 63 not supported
 *                     Unit: dB
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_alc_set_gain(esp_ae_alc_handle_t handle, uint8_t ch_idx, int8_t gain);

/**
 * @brief  Get the gain for a specific channel from the ALC handle
 *
 * @param[in]   handle  The ALC handle
 * @param[in]   ch_idx  The channel index of the gain to retrieve. eg: 0 refers to the first channel
 * @param[out]  gain    Pointer to store the retrieved gain. Unit: dB
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_alc_get_gain(esp_ae_alc_handle_t handle, uint8_t ch_idx, int8_t *gain);

/**
 * @brief  Modify audio volume by adjusting gain with interleaved input data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The ALC handle
 * @param[in]   sample_num   Number of sampling points processed by ALC
 * @param[in]   in_samples   Input samples buffer. The size of `in_samples` must be greater
 *                           than or equal to `sample_num * channel * bit >> 3`
 * @param[out]  out_samples  Output samples buffer. The size of `out_samples` must be greater
 *                           than or equal to `sample_num * channel * bit >> 3`.
 *                           The pointer address can be set equal to `in_samples` or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_alc_process(esp_ae_alc_handle_t handle, uint32_t sample_num,
                                esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Modify audio volume by adjusting gain with deinterleaved input data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10].
 *
 * @param[in]   handle       The ALC handle
 * @param[in]   sample_num   Number of sampling points processed by ALC
 * @param[in]   in_samples   Input samples buffer array. The size of every `in_samples buffer` must
 *                           larger than `sample_num * bit >> 3`
 * @param[out]  out_samples  Output samples buffer array. The size of every `out_samples buffer`
 *                           must larger than `sample_num * bit >> 3`. The pointer address can be set
 *                           equal to `in_samples` or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_alc_deintlv_process(esp_ae_alc_handle_t handle, uint32_t sample_num,
                                        esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Deinitialize ALC handle
 *
 * @param  handle  The ALC handle
 */
void esp_ae_alc_close(esp_ae_alc_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
