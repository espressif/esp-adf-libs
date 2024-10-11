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

#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  This module realize fade in and fade out effect for audio stream.
 *         Fade processing involves multiplying audio data with a smooth transition weight.
 *
 *         In the initialization configuration of fades, users need to configure
 *         the audio basic information which is `mode`, `curve`, and `transit_time`.
 *         `mode` is mode for fade, provided two mode:
 *         The `ESP_AE_FADE_MODE_FADE_IN` refers to the gradual increase in volume, which means the weight changes from 0 to 1.
 *         The `ESP_AE_FADE_MODE_FADE_OUT` refers to the gradual decrease in volume, which means the weight changes from 1 to 0.
 *         The effect of fading in or out depends on the choice of response curve which supports three response
 *         curve in `esp_ae_fade_curve_t` and response time.
 *
 *         If user want to adjust the fade mode during processing, can call the `esp_ae_fade_set_mode` function.
 *
 *         If the user wants to restart the current fade mode, can call the `esp_ae_fade_reset_weight` function.
 *
 *         Fade processing is based on sampling points as processing units. The relationship between
 *         processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3)).
 *
 *         Fade also offers two processing interfaces to fulfill the needs of interleaved
 *         and deinterleaved input data layouts.
 */

/**
 * @brief  The handle of fade
 */
typedef void *esp_ae_fade_handle_t;

/**
 * @brief  Enum for fade curve types
 */
typedef enum {
    ESP_AE_FADE_CURVE_INVALID = 0,  /*!< Invalid status */
    ESP_AE_FADE_CURVE_LINE    = 1,  /*!< Linear fade variation curve */
    ESP_AE_FADE_CURVE_QUAD    = 2,  /*!< Quadratic fade variation curve */
    ESP_AE_FADE_CURVE_SQRT    = 3,  /*!< Square root fade variation curve */
    ESP_AE_FADE_CURVE_MAX     = 4,  /*!< The maximum value */
} esp_ae_fade_curve_t;

/**
 * @brief  Enum for fade modes
 */
typedef enum {
    ESP_AE_FADE_MODE_INVALID  = 0,  /*!< Invalid status */
    ESP_AE_FADE_MODE_FADE_IN  = 1,  /*!< Fade-in mode */
    ESP_AE_FADE_MODE_FADE_OUT = 2,  /*!< Fade-out mode */
    ESP_AE_FADE_MODE_MAX      = 3,  /*!< The maximum value */
} esp_ae_fade_mode_t;

/**
 * @brief  Configuration structure for fade
 */
typedef struct {
    esp_ae_fade_mode_t  mode;             /*!< The fade mode */
    esp_ae_fade_curve_t curve;            /*!< The curve used when the weight changes from 0 to 1 in fade-in mode
                                               and from 1 to 0 in fade-out mode */
    uint32_t            transit_time;     /*!< The transition time (ms) when the weight changes from 0 to 1 in
                                               fade-in mode and from 1 to 0 in fade-out mode */
    uint32_t            sample_rate;      /*!< The audio sample rate */
    uint8_t             channel;          /*!< The audio channel num */
    uint8_t             bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
} esp_ae_fade_cfg_t;

/**
 * @brief  Initialize a fade handle based on the provided configuration
 *
 * @param[in]   cfg     Fade configuration
 * @param[out]  handle  The fade handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_open(esp_ae_fade_cfg_t *cfg, esp_ae_fade_handle_t *handle);

/**
 * @brief  Apply fade effect to an audio stream, where the input data is interleaved
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The fade handle
 * @param[in]   sample_num   Number of sampling points processed by the fade
 * @param[in]   in_samples   Input samples buffer. The size of 'in_samples' must be greater
 *                           than or equal to sample_num * channel * bit >> 3
 * @param[out]  out_samples  Output samples buffer. The size of 'out_samples' must be greater
 *                           than or equal to sample_num * channel * bit >> 3. The pointer address can be
 *                           set equal to 'in_samples' or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_process(esp_ae_fade_handle_t handle, uint32_t sample_num,
                                 esp_ae_sample_t in_samples,
                                 esp_ae_sample_t out_samples);

/**
 * @brief  Apply fade effect to an audio stream, where the input data is deinterleaved.
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]   handle       The fade handle
 * @param[in]   sample_num   Number of sampling points processed by the fade
 * @param[in]   in_samples   Input samples buffer array. The size of every 'in_samples buffer' must
 *                           be greater than or equal to sample_num * bit >> 3
 * @param[out]  out_samples  Output samples buffer array. The size of every 'out_samples buffer'
 *                           must be greater than or equal to sample_num * bit >> 3.
 *                           The pointer address can be set equal to 'in_samples' or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_deintlv_process(esp_ae_fade_handle_t handle, uint32_t sample_num,
                                         esp_ae_sample_t in_samples[],
                                         esp_ae_sample_t out_samples[]);

/**
 * @brief  Set the fade process mode
 *
 * @param[in]  handle  The fade handle
 * @param[in]  mode    The mode of fade
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_set_mode(esp_ae_fade_handle_t handle, esp_ae_fade_mode_t mode);

/**
 * @brief  Get the fade process mode
 *
 * @param[in]   handle  The fade handle
 * @param[out]  mode    The mode of fade
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_get_mode(esp_ae_fade_handle_t handle, esp_ae_fade_mode_t *mode);

/**
 * @brief  Reset the fade process to the initial configuration state.
 *         If the initial configuration mode is fade in, the current weight is set to 0.
 *         If the initial configuration mode is fade out, the current weight is set to 1.
 *         This function helps the user to restart fade in or fade out without
 *         reopening and closing.
 *
 * @param[in]  handle  The fade handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_fade_reset_weight(esp_ae_fade_handle_t handle);

/**
 * @brief  Deinitialize  the fade handle
 *
 * @param[in]  handle  The fade handle
 */
void esp_ae_fade_close(esp_ae_fade_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
