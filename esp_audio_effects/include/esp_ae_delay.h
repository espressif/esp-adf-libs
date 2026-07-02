/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Delay is an audio effect that produces one or more delayed copies of the input signal,
 *         creating an echo effect. This implementation uses a simple feedback delay line based on
 *         a circular buffer. The delayed signal is fed back into the buffer with a configurable
 *         feedback coefficient, producing repeated echoes that gradually decay.
 *
 *         Key parameters:
 *         - `delay_time_ms` controls the time interval between echoes (0 = bypass)
 *         - `feedback` controls how many times the echo repeats (higher = more repeats).
 *           Set to 0.0 for a single delay copy with no repeated echoes
 *         - `mix` controls wet/dry balance via equal-power crossfade (0.0 = dry only, 1.0 = wet only)
 *
 *         Note: This module does not apply built-in defaults to `esp_ae_delay_para_t` fields.
 *         All parameter fields must be set explicitly before calling `esp_ae_delay_open()`.
 *         See the `ESP_AE_DELAY_DEFAULT_*` macros for recommended starting values.
 *         Setting `max_delay_ms` to 0 selects `ESP_AE_DELAY_DEFAULT_MAX_MS` (1000 ms).
 *
 *         Delay processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Delay also offers two processing interfaces to fulfill the needs of interleaved and
 *         deinterleaved input data layouts.
 */

/**
 * @brief  The handle of delay
 */
typedef void *esp_ae_delay_handle_t;

/**
 * @brief  Structure of delay parameters
 */
typedef struct {
    uint16_t  delay_time_ms;  /*!< Delay time in milliseconds. Range: [0, max_delay_ms].
                                   0 = bypass (output equals input, no delay applied) */
    float     feedback;       /*!< Feedback coefficient controlling echo repetitions. Range: [0.0, 0.95].
                                   0.0 = single delay (no echo repetition); higher values produce
                                   more repeated echoes that decay more slowly */
    float     mix;            /*!< Wet/dry mix ratio via equal-power crossfade. Range: [0.0, 1.0].
                                   0.0 = dry (original) only, 1.0 = wet (delayed) only, 0.5 ≈ balanced */
} esp_ae_delay_para_t;

/**
 * @brief  Structure of delay configuration
 */
typedef struct {
    uint32_t             sample_rate;      /*!< The audio sample rate */
    uint8_t              channel;          /*!< The audio channel number */
    uint8_t              bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    uint16_t             max_delay_ms;     /*!< Maximum delay time in milliseconds, determines the
                                                per-channel delay buffer size
                                                (buffer_samples = sample_rate * max_delay_ms / 1000).
                                                Larger values require more memory. Range: [0, 1000].
                                                0 = use ESP_AE_DELAY_DEFAULT_MAX_MS (1000 ms) */
    esp_ae_delay_para_t  delay_para;       /*!< The delay parameters */
} esp_ae_delay_cfg_t;

/**
 * @brief  Create a delay handle based on the provided configuration
 *
 * @param[in]   cfg     Delay configuration
 * @param[out]  handle  The delay handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_open(esp_ae_delay_cfg_t *cfg, esp_ae_delay_handle_t *handle);

/**
 * @brief  Process delay for an audio stream with interleaved input data
 *
 * @note  1. The interleaved data is shown in the example:
 *           sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The delay handle
 * @param[in]   sample_num   Number of sampling points processed by the delay
 * @param[in]   in_samples   Input samples buffer with sample_num audio samples. The size of 'in_samples' must be greater
 *                           than or equal to sample_num * channel * bit >> 3
 * @param[out]  out_samples  Output samples buffer with sample_num audio samples. The size of 'out_samples' must be greater
 *                           than or equal to sample_num * channel * bit >> 3. The pointer address can be
 *                           set equal to 'in_samples' or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_process(esp_ae_delay_handle_t handle, uint32_t sample_num,
                                  esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Process delay for an audio stream with uninterleaved input data
 *
 * @note  1. The deinterleaved data is shown in the example:
 *           sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The delay handle
 * @param[in]   sample_num   Number of sampling points processed by the delay
 * @param[in]   in_samples   Input samples buffer array. The size of every 'in_samples buffer' must
 *                           be greater than or equal to sample_num * bit >> 3
 * @param[out]  out_samples  Output samples buffer array. The size of every 'out_samples buffer'
 *                           must be greater than or equal to sample_num * bit >> 3
 *                           The pointer address can be set equal to 'in_samples' or not
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_deintlv_process(esp_ae_delay_handle_t handle, uint32_t sample_num,
                                          esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Set the delay time
 *
 * @param[in]  handle         The delay handle
 * @param[in]  delay_time_ms  The delay time in milliseconds, range: [0, max_delay_ms].
 *                            0 = bypass (no delay applied)
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_set_delay_time(esp_ae_delay_handle_t handle, uint16_t delay_time_ms);

/**
 * @brief  Get the delay time
 *
 * @param[in]   handle         The delay handle
 * @param[out]  delay_time_ms  The delay time in milliseconds
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_get_delay_time(esp_ae_delay_handle_t handle, uint16_t *delay_time_ms);

/**
 * @brief  Set the feedback coefficient
 *
 * @param[in]  handle    The delay handle
 * @param[in]  feedback  The feedback coefficient, range: [0.0, 0.95].
 *                       0.0 = single delay with no echo repetition
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_set_feedback(esp_ae_delay_handle_t handle, float feedback);

/**
 * @brief  Get the feedback coefficient
 *
 * @param[in]   handle    The delay handle
 * @param[out]  feedback  The feedback coefficient
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_get_feedback(esp_ae_delay_handle_t handle, float *feedback);

/**
 * @brief  Set wet/dry mix ratio (equal-power crossfade)
 *
 * - mix = 0.0 → dry only
 * - mix = 1.0 → wet only
 * - mix = 0.5 → equal-power blend (both ≈ -3 dB)
 *
 * @param[in]  handle  The delay handle
 * @param[in]  mix     Mix ratio, range: [0.0, 1.0]. 0.0 = full dry, 1.0 = full wet
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_set_mix(esp_ae_delay_handle_t handle, float mix);

/**
 * @brief  Get the current wet/dry mix ratio
 *
 * @param[in]   handle  The delay handle
 * @param[out]  mix     Mix ratio, range: [0.0, 1.0]
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_get_mix(esp_ae_delay_handle_t handle, float *mix);

/**
 * @brief  Reset the internal processing state of the delay handle while preserving user-configured settings.
 *         This function clears the delay buffer, allowing efficient reuse of the handle
 *         without the overhead of destroying and recreating it when audio information remains unchanged.
 *
 * @note  This function is not thread-safe, and users must ensure correct call sequencing
 *         and avoid invoking this function while the process is running
 *
 * @param[in]  handle  The delay handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_delay_reset(esp_ae_delay_handle_t handle);

/**
 * @brief  Deinitialize the delay handle
 *
 * @param  handle  The delay handle
 */
void esp_ae_delay_close(esp_ae_delay_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
