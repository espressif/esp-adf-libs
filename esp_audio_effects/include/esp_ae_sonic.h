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

#include <stdbool.h>
#include <stdint.h>
#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Sonic is an audio processing tool that allows for the adjustment of pitch and playback speed of audio.
 *         By setting the speed parameter, can achieve variable speed without changing the pitch;
 *         by setting the pitch parameter, can achieve variable pitch without changing speed;
 *         by setting both speed and pitch parameters, can achieve the different multiple of speed and pitch.
 *
 *         Sonic processing is based on sampling points as processing units. The relationship between processing
 *         data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Sonic only offers the interleaved input data layout processing interfaces.
 *         If the input data layout is deinterleaved, user need to create a sonic handle for each channel
 *         and process independently.
 *
 *         In Sonic processing, users need to configure the `samples` and `num` parameters
 *         in the structure of `esp_ae_sonic_in_data_t`, where `samples` represents the input buffer
 *         and `num` represents the number of sampling points to be processed;
 *         and configure `samples` and `needed_num` in the structure of `esp_ae_sonic_out_data_t`,
 *         where `samples` represents the output buffer, `needed_num` represents the desired
 *         number of sample points after processing.
 *
 *         The Sonic processing process is as follows:
 *         1) When the output sampling points of the internal cache are greater than or equal to
 *         the `needed_num`, the sonic data will be directly copied to the output buffer,
 *         and `out_num = needed_num`, `consume_num = 0`.
 *         2) When the output sampling points of the internal cache are less than the 'needed_num',
 *         there are two case with sonic.
 *         Case 1, if all the input data is consumed, but the output sampling points of the
 *         internal cache are still less than the `needed_num`, the sonic data will be directly
 *         copied to the output buffer, and `out_num = out_cache_num`,  `consume_num = num`.
 *         Case 2, if the input data consumes a portion, but the output sampling points of the
 *         internal cache are greater than or equal to the `needed_num`, the sonic data will be
 *         directly copied to the output buffer, and `out_num = needed_num`, `consume_num = in_sonic_consume_num`.
 */

/**
 * @brief  The handle of sonic
 */
typedef void *esp_ae_sonic_handle_t;

/**
 * @brief  Configuration structure for Sonic
 */
typedef struct {
    uint32_t sample_rate;      /*!< The audio stream sample rate which is change to should be multiple of 4000 or 11025 */
    uint8_t  channel;          /*!< The audio stream channel */
    uint8_t  bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
} esp_ae_sonic_cfg_t;

/**
 * @brief  Sonic input data structure
 */
typedef struct {
    void    *samples;      /*!< The input samples buffer. The buffer size of samples need set larger
                                than `num * channel * (bit >> 3)` */
    uint32_t num;          /*!< Number of sampling points in the samples */
    uint32_t consume_num;  /*!< Number of sampling points that current sonic processing have consumed. (Output parameter) */
} esp_ae_sonic_in_data_t;

/**
 * @brief  Sonic output data structure
 */
typedef struct {
    void    *samples;     /*!< The output samples buffer. The buffer size of samples need set larger
                               than `needed_num * channel * (bit >> 3)` */
    uint32_t needed_num;  /*!< Number of sampling points that want to get from sonic processing */
    uint32_t out_num;     /*!< Number of sampling points for sonic processing output.
                               The `out_num` is less than or equal `needed_num`. (Output parameter) */
} esp_ae_sonic_out_data_t;

/**
 * @brief  Create a sonic handle through configuration
 *
 * @param[in]   cfg     Sonic configuration
 * @param[out]  handle  The sonic handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_open(esp_ae_sonic_cfg_t *cfg, esp_ae_sonic_handle_t *handle);

/**
 * @brief  Set the audio speed
 *
 * @param[in]  handle  The handle of the sonic
 * @param[in]  speed   The scaling factor of audio speed.
 *                     The range of speed is [0.5, 2.0]
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_set_speed(esp_ae_sonic_handle_t handle, float speed);

/**
 * @brief  Get the audio speed
 *
 * @param[in]   handle  The handle of the sonic
 * @param[out]  speed   The scaling factor of audio speed
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_get_speed(esp_ae_sonic_handle_t handle, float *speed);

/**
 * @brief  Set the audio pitch.
 *
 * @param[in]  handle  The handle of the sonic
 * @param[in]  pitch   The scaling factor of audio pitch.
 *                     The range of pitch is [0.5, 2.0].
 *                     If the pitch value is smaller than 1.0, the sound is deep voice;
 *                     if the pitch value is equal to 1.0, the sound is no change;
 *                     if the pitch value is gather than 1.0, the sound is sharp voice;
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_set_pitch(esp_ae_sonic_handle_t handle, float pitch);

/**
 * @brief  Get the audio pitch
 *
 * @param[in]   handle  The handle of the sonic
 * @param[out]  pitch   The scaling factor of audio pitch
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_get_pitch(esp_ae_sonic_handle_t handle, float *pitch);

/**
 * @brief  Do sonic process(Change speed without altering pitch; Change pitch without altering speed;
 *         Change pitch and change speed), and the input data is interleaved
 *
 * @param[in]  handle       The handle of the sonic
 * @param[in]  in_samples   The structure of `esp_ae_sonic_in_data_t`
 * @param[in]  out_samples  The structure of `esp_ae_sonic_out_data_t`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_sonic_process(esp_ae_sonic_handle_t handle, esp_ae_sonic_in_data_t *in_samples,
                                  esp_ae_sonic_out_data_t *out_samples);

/**
 * @brief  Deinitialize the sonic stream
 *
 * @param  handle  The handle of the sonic
 */
void esp_ae_sonic_close(esp_ae_sonic_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
