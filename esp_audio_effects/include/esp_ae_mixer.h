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
 * @brief  Mixer is the process of combining multiple audio signals which have the same sample_rate,
 *         channel and bits_per_sample into one single audio signal, where each audio signal has a weight.
 *         These weight values are multiplied with the corresponding channel's audio data,
 *         and the multiplied data from each channel is then summed to form the final mixed audio signal.
 *         The formula is as follows:
 *         output = w0 * input0 + w1 * input1 + w2 * input2 + ...
 *
 *         In the initialization configuration of Mixer, users need to configure the
 *         audio basic information, source number and source information on each channel
 *         which including `weight1`, `weight2`, and `transit_time`. The `weight1` is
 *         the initial weight of each channel. If user use `esp_ae_mixer_set_mode` to set the mode
 *         of a channel to `ESP_AE_MIXER_MODE_FADE_UPWARD`, the weight for that channel will change
 *         from weight1 to weight2 over the specified transit time. If the user sets the mode
 *         of a channel to `ESP_AE_MIXER_MODE_FADE_DOWNWARD`, the weight for that channel
 *         will eventually become weight1.
 *
 *         Mixer processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Mixer also offers two processing interfaces to fulfill the needs of interleaved
 *         and deinterleaved input data layouts.
 */

/**
 * @brief  The handle of mixer
 */
typedef void *esp_ae_mixer_handle_t;

/**
 * @brief  The stream transit mode of mixer
 */
typedef enum {
    ESP_AE_MIXER_MODE_INVALID       = 0,  /*!< Invalid status */
    ESP_AE_MIXER_MODE_FADE_UPWARD   = 1,  /*!< Mixer fade upward, audio weight
                                               changes to weight2 */
    ESP_AE_MIXER_MODE_FADE_DOWNWARD = 2,  /*!< Mixer fade downward, audio weight
                                               changes to weight1 */
    ESP_AE_MIXER_MODE_MAX           = 3,  /*!< The maximum value */
} esp_ae_mixer_mode_t;

/**
 * @brief  Structure of input stream information for the mixer
 */
typedef struct {
    float    weight1;       /*!< The initial weight of the audio stream.
                                 The weight value maintained under stable conditions
                                 when MIXER is set to 'ESP_AE_MIXER_MODE_FADE_DOWNWARD' mode.
                                 The data range of weight1 is [0.0, 1.0] */
    float    weight2;       /*!< The weight value maintained under stable conditions
                                 when MIXER is set to 'ESP_AE_MIXER_MODE_FADE_UPWARD' mode.
                                 The data range of weight1 is [0.0, 1.0] */
    uint32_t transit_time;  /*!< Time of change from weight1 to weight2 or weight2 to weight1.
                                 unit: ms */
} esp_ae_mixer_info_t;

/**
 * @brief  Configuration structure for the mixer
 */
typedef struct {
    uint32_t             sample_rate;      /*!< The audio sample rate */
    uint8_t              channel;          /*!< The channel number of the input stream */
    uint8_t              bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    uint8_t              src_num;          /*!< The number of input streams to the mixer */
    esp_ae_mixer_info_t *src_info;         /*!< Array for structure pointer of
                                                each streams's set information */
} esp_ae_mixer_cfg_t;

/**
 * @brief  Create the mixer handle through configuration
 *
 * @param[in]   cfg     Mixer configuration
 * @param[out]  handle  The mixer handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mixer_open(esp_ae_mixer_cfg_t *cfg, esp_ae_mixer_handle_t *handle);

/**
 * @brief  Set the transit mode of a certain stream according to src_idx
 *
 * @param[in]  handle   The mixer handle
 * @param[in]  src_idx  The index of a certain source stream which want to set transit mode.
 *                      eg: 0 refer to first source stream
 * @param[in]  mode     The transit mode of source stream
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mixer_set_mode(esp_ae_mixer_handle_t handle, uint8_t src_idx, esp_ae_mixer_mode_t mode);

/**
 * @brief  Mix multiple interleaved audio data streams
 *
 * @param[in]   handle       The mixer handle
 * @param[in]   sample_num   Number of sampling points processed by mixer
 * @param[in]   in_samples   An array that stores pointers to the interleaved input streams
 * @param[out]  out_samples  The output samples buffer must be 16 bytes aligned
 *
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mixer_process(esp_ae_mixer_handle_t handle, uint32_t sample_num,
                                  esp_ae_sample_t in_samples[],
                                  esp_ae_sample_t out_samples);

/**
 * @brief  Mix multiple deinterleaved audio data streams
 *
 * @param[in]   handle       The mixer handle
 * @param[in]   sample_num   Number of sampling points processed by mixer
 * @param[in]   in_samples   An array that stores pointers to the input stream buffer pointer arrays.
 *                           Note: `esp_ae_sample_t *in_samples[]` equal to `in_samples[src_num][ch_num]`
 * @param[out]  out_samples  Array for output samples buffer pointer with each channel.
 *                           The output samples buffer must be 16 bytes aligned.
 *                           Note: `esp_ae_sample_t out_samples[]` equal to `out_samples[ch_num]`
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mixer_deintlv_process(esp_ae_mixer_handle_t handle, uint32_t sample_num,
                                          esp_ae_sample_t *in_samples[],
                                          esp_ae_sample_t  out_samples[]);

/**
 * @brief  Deinitialize the mixer handle
 *
 * @param  handle  The mixer handle
 */
void esp_ae_mixer_close(esp_ae_mixer_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
