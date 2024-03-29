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
#include <stdbool.h>
#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  An equalizer (EQ) is an audio processing tool used to adjust the
 *         balance between different frequency components in an audio signal.
 *         It allows users to control the balance and tone of different frequency ranges
 *         to achieve a desired tonal balance and improve the overall quality of the sound.
 *
 *         In the initialization configuration of EQ, user need to configure the
 *         audio basic information, filter number and filter parameter for each filter
 *         which including the filter type, center frequency, q value, and gain value
 *         to achieve the required equalization processing.
 *
 *         Note:
 *         1. Filter banks only support having one high pass filter.
 *         2. Filter banks only support having one low pass filter.
 *         3. The `fc` of a high pass filter is the minimum `fc` in the filter bank.
 *         4. The `fc` of a low pass filter is the maximum `fc` in the filter bank.
 *         5. In the initialization state, all bands are enable state.
 *
 *         During the EQ processing, user can enable or disable certain filter from the filter banks
 *         by calling `esp_ae_eq_enable_filter` and `esp_ae_eq_disable_filter`.
 *         Users can also modify the parameters of a certain filter by calling
 *         `esp_ae_eq_set_filter_para`.
 *
 *         EQ processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         EQ also offers two processing interfaces to fulfill the needs of interleaved
 *         and deinterleaved input data layouts.
 */

/**
 * @brief  The handle of equalizer
 */
typedef void *esp_ae_eq_handle_t;

/**
 * @brief  The type of equalizer filter
 */
typedef enum {
    ESP_AE_EQ_FILTER_INVALID    = 0,  /*!< Invalid status */
    ESP_AE_EQ_FILTER_HIGH_PASS  = 1,  /*!< High pass filter */
    ESP_AE_EQ_FILTER_LOW_PASS   = 2,  /*!< Low pass filter */
    ESP_AE_EQ_FILTER_PEAK       = 3,  /*!< Peak filter */
    ESP_AE_EQ_FILTER_HIGH_SHELF = 4,  /*!< High shelf pass filter */
    ESP_AE_EQ_FILTER_LOW_SHELF  = 5,  /*!< Low shelf pass filter */
    ESP_AE_EQ_FILTER_MAX        = 6,  /*!< The maximum value */
} esp_ae_eq_filter_type_t;

/**
 * @brief  The structure of equalizer filter set-up parameter
 */
typedef struct {
    esp_ae_eq_filter_type_t filter_type;  /*!< The type of equalizer filter */
    uint32_t                fc;           /*!< For high shelf, low shelf, and peak filter, it represents the center frequency.
                                               For high pass and low pass filter, it represents the cut-off frequency.
                                               The value should be less than half of the sampling rate */
    float                   q;            /*!< Q value of filter. Used to describe the bandwidth of the filter.
                                               The higher the Q value, the narrower the bandwidth;
                                               the lower the Q value, the wider the bandwidth.
                                               The range of q is [0.1, 20.0] */
    float                   gain;         /*!< Gain value of filter. Only valid for high shelf, low shelf, and peak filter.
                                               The range of the gain is [-15, 15], unit: dB */
} esp_ae_eq_filter_para_t;

/**
 * @brief  Configuration structure for equalizer
 */
typedef struct {
    uint32_t                 sample_rate;      /*!< The audio sample rate */
    uint8_t                  channel;          /*!< The audio channel number */
    uint8_t                  bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    uint8_t                  filter_num;       /*!< The equalizer's filter number */
    esp_ae_eq_filter_para_t *para;             /*!< Array of structure pointers for filter configuration parameters.
                                                    The size of 'para' is equal to filter number */
} esp_ae_eq_cfg_t;

/**
 * @brief  Create an equalizer handle based on the provided configuration
 *
 * @param[in]   cfg     Equalizer configuration
 * @param[out]  handle  The eq handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_eq_open(esp_ae_eq_cfg_t *cfg, esp_ae_eq_handle_t *handle);

/**
 * @brief  Process equalization for an audio stream with interleaved input data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The eq handle
 * @param[in]   sample_num   Number of sampling points processed by the equalizer
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
esp_ae_err_t esp_ae_eq_process(esp_ae_eq_handle_t handle, uint32_t sample_num,
                               esp_ae_sample_t in_samples,
                               esp_ae_sample_t out_samples);

/**
 * @brief  Process equalization for an audio stream with deinterleaved input data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]   handle       The eq handle
 * @param[in]   sample_num   Number of sampling points processed by the equalizer
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
esp_ae_err_t esp_ae_eq_deintlv_process(esp_ae_eq_handle_t handle, uint32_t sample_num,
                                       esp_ae_sample_t in_samples[],
                                       esp_ae_sample_t out_samples[]);

/**
 * @brief  Set the filter parameters for a specific filter identified by 'filter_idx'
 *
 * @param[in]  handle      The eq handle
 * @param[in]  filter_idx  The index of a specific filter for which the parameters are to be set.
 *                         eg: 0 refers to the first filter
 * @param[in]  para        The filter setup parameter
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_eq_set_filter_para(esp_ae_eq_handle_t handle, uint8_t filter_idx,
                                       esp_ae_eq_filter_para_t *para);

/**
 * @brief  Get the filter parameters for a specific filter identified by 'filter_idx'
 *
 * @param[in]   handle      The eq handle
 * @param[in]   filter_idx  The index of a specific filter for which the parameters are to be retrieved.
 *                          eg: 0 refers to first filter
 * @param[out]  para        The filter setup parameter
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_eq_get_filter_para(esp_ae_eq_handle_t handle, uint8_t filter_idx,
                                       esp_ae_eq_filter_para_t *para);

/**
 * @brief  Enable processing for a specific filter identified by 'filter_idx' in the equalizer
 *
 * @param[in]  handle      The eq handle
 * @param[in]  filter_idx  The index of a specific filter to be enabled
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_eq_enable_filter(esp_ae_eq_handle_t handle, uint8_t filter_idx);

/**
 * @brief  Disable processing for a specific filter identified by 'filter_idx' in the equalizer.
 *
 * @param[in]  handle      The eq handle
 * @param[in]  filter_idx  The index of a specific filter to be disabled
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_eq_disable_filter(esp_ae_eq_handle_t handle, uint8_t filter_idx);

/**
 * @brief  Deinitialize the eq handle
 *
 * @param  handle  The eq handle
 */
void esp_ae_eq_close(esp_ae_eq_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
