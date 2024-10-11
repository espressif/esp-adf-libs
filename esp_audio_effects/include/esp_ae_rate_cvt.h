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
 * @brief  Rate convertion is used to change the sample rate of an audio signal.
 *
 *         In the initialization configuration of Rate convertion, users need to configure
 *         the audio basic information, dest sample rate and complexity.
 *         The configuration of complexity affects the audio quality and processing
 *         speed of resampling.
 *
 *         Rate convertion processing is based on sampling points as processing units.
 *         The relationship between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         The `esp_ae_rate_cvt_get_max_out_sample_num` function allows users to obtain the recommended
 *         sample point size for the output buffer by providing the number of processed sample points.
 *
 *         Rate convertion also offers two interfaces to process interleaved and deinterleaved data seperately.
 */

typedef enum {
    ESP_AE_RATE_CVT_PERF_TYPE_MEMORY = 0,  /*!< INRAM usage lesser and CPU usage higher than `ESP_AE_RATE_CVT_PERF_TYPE_SPEED` */
    ESP_AE_RATE_CVT_PERF_TYPE_SPEED  = 1,  /*!< INRAM usage higher and CPU usage lower than `ESP_AE_RATE_CVT_PERF_TYPE_MEMORY` */
} esp_ae_rate_cvt_perf_type_t;

/**
 * @brief  The handle of rate convertion
 */
typedef void *esp_ae_rate_cvt_handle_t;

/**
 * @brief  Rate convertion configuration
 */
typedef struct {
    uint32_t                    src_rate;         /*!< The sample rate of input audio stream which is change to should be multiple of 4000 or 11025 */
    uint32_t                    dest_rate;        /*!< The sample rate of output audio stream which is change to should be multiple of 4000 or 11025 */
    uint8_t                     channel;          /*!< The audio channel number */
    uint8_t                     bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    uint8_t                     complexity;       /*!< Indicates the complexity of the resampling.
                                                       Range: 1~3;
                                                       1: lowest complexity (worst audio quality while fastest speed);
                                                       3: highest complexity (best audio quality while slowest speed). */
    esp_ae_rate_cvt_perf_type_t perf_type;        /*!< The select type about lesser CPU usage or lower INRAM usage, refer to `esp_ae_rate_cvt.h` */
} esp_ae_rate_cvt_cfg_t;

/**
 * @brief  Create rate convertion handle through configuration
 *
 * @param[in]   cfg     Rate convertion configuration
 * @param[out]  handle  The rate convertion handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_rate_cvt_open(esp_ae_rate_cvt_cfg_t *cfg, esp_ae_rate_cvt_handle_t *handle);

/**
 * @brief  Get the minimum required sample number for the output frame buffer
 *
 * @param[in]   handle          The rate convertion handle
 * @param[in]   in_sample_num   The number of input sampling points for resampling processing
 * @param[out]  out_sample_num  Minimum number of sampling points for the output buffer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_FAIL               On fail procss
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_rate_cvt_get_max_out_sample_num(esp_ae_rate_cvt_handle_t handle, uint32_t in_sample_num,
                                                    uint32_t *out_sample_num);

/**
 * @brief  Perform rate conversion on interleaved audio data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]      handle          The rate convertion handle
 * @param[in]      in_samples      The input samples buffer
 * @param[in]      in_sample_num   The input samples number
 * @param[out]     out_samples     The output samples buffer
 * @param[in,out]  out_sample_num  For the input parameter, it represents the maximum number of samples in the output buffer.
 *                                 For the output parameter, it represents the actual number of output samples processed by rate convertion
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_rate_cvt_process(esp_ae_rate_cvt_handle_t handle, esp_ae_sample_t in_samples,
                                     uint32_t in_sample_num, esp_ae_sample_t out_samples,
                                     uint32_t *out_sample_num);

/**
 * @brief  Perform bit conversion on deinterleaved audio data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]      handle          The rate convertion handle
 * @param[in]      in_samples      Array of input buffer pointers with each channel
 * @param[in]      in_sample_num   Input samples for resampling processing
 * @param[out]     out_samples     Array of output buffer pointers with each channel
 * @param[in,out]  out_sample_num  For the input parameter, it represents the maximum number of samples in the output buffer.
 *                                 For the output parameter, it represents the actual number of output samples processed by rate convertion
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_rate_cvt_deintlv_process(esp_ae_rate_cvt_handle_t handle, esp_ae_sample_t in_samples[],
                                             uint32_t in_sample_num, esp_ae_sample_t out_samples[],
                                             uint32_t *out_sample_num);

/**
 * @brief  Deinitialize the rate convertion handle
 *
 * @param  handle  The rate convertion handle
 */
void esp_ae_rate_cvt_close(esp_ae_rate_cvt_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
