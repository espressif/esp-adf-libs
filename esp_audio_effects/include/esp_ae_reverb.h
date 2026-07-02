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
 * @brief  Reverb is an audio effect that simulates the natural reflections of sound in an enclosed space.
 *         This implementation uses the Freeverb (Schroeder-Moorer) algorithm, which consists of
 *         8 parallel comb filters with low-pass damping feedback followed by 4 series allpass filters
 *         for diffusion. The algorithm is lightweight and suitable for ESP32 embedded platforms.
 *
 *         Key parameters:
 *         - `room_size` controls the feedback gain of comb filters, affecting reverb tail length
 *         - `damping` controls high-frequency absorption rate in the feedback loop
 *         - `wet_level` and `dry_level` control the mix between processed and original signal
 *         - `pre_delay_ms` adds initial delay before reverb onset
 *
 *         Reverb processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         Reverb also offers two processing interfaces to fulfill the needs of interleaved and
 *         deinterleaved input data layouts.
 */

/**
 * @brief  The handle of reverb
 */
typedef void *esp_ae_reverb_handle_t;

/**
 * @brief  Structure of reverb parameters
 */
typedef struct {
    float     room_size;     /*!< Room size factor controlling reverb tail length. Range: [0.0, 1.0] */
    float     damping;       /*!< High-frequency damping factor. Range: [0.0, 1.0].
                                  Higher values cause faster high-frequency decay */
    float     wet_level;     /*!< Wet (reverb) signal level. Range: [-96.0, 0.0], unit: dB */
    float     dry_level;     /*!< Dry (original) signal level. Range: [-96.0, 0.0], unit: dB */
    uint16_t  pre_delay_ms;  /*!< Pre-delay time before reverb onset. Range: [0, 200], unit: ms */
} esp_ae_reverb_para_t;

/**
 * @brief  Structure of reverb configuration
 */
typedef struct {
    uint32_t              sample_rate;      /*!< The audio sample rate */
    uint8_t               channel;          /*!< The audio channel number */
    uint8_t               bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    esp_ae_reverb_para_t  reverb_para;      /*!< The reverb parameters */
} esp_ae_reverb_cfg_t;

/**
 * @brief  Create a reverb handle based on the provided configuration
 *
 * @param[in]   cfg     Reverb configuration
 * @param[out]  handle  The reverb handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_open(esp_ae_reverb_cfg_t *cfg, esp_ae_reverb_handle_t *handle);

/**
 * @brief  Process reverb for an audio stream with interleaved input data
 *
 * @note  1. The interleaved data is shown in the example:
 *           sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The reverb handle
 * @param[in]   sample_num   Number of sampling points processed by the reverb
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
esp_ae_err_t esp_ae_reverb_process(esp_ae_reverb_handle_t handle, uint32_t sample_num,
                                   esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Process reverb for an audio stream with uninterleaved input data
 *
 * @note  1. The deinterleaved data is shown in the example:
 *           sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The reverb handle
 * @param[in]   sample_num   Number of sampling points processed by the reverb
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
esp_ae_err_t esp_ae_reverb_deintlv_process(esp_ae_reverb_handle_t handle, uint32_t sample_num,
                                           esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Set the value of room size
 *
 * @param[in]  handle     The reverb handle
 * @param[in]  room_size  The room size factor, range: [0.0, 1.0]
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_set_room_size(esp_ae_reverb_handle_t handle, float room_size);

/**
 * @brief  Get the value of room size
 *
 * @param[in]   handle     The reverb handle
 * @param[out]  room_size  The room size factor
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_get_room_size(esp_ae_reverb_handle_t handle, float *room_size);

/**
 * @brief  Set the value of damping
 *
 * @param[in]  handle   The reverb handle
 * @param[in]  damping  The damping factor, range: [0.0, 1.0]
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_set_damping(esp_ae_reverb_handle_t handle, float damping);

/**
 * @brief  Get the value of damping
 *
 * @param[in]   handle   The reverb handle
 * @param[out]  damping  The damping factor
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_get_damping(esp_ae_reverb_handle_t handle, float *damping);

/**
 * @brief  Set the value of wet level
 *
 * @param[in]  handle     The reverb handle
 * @param[in]  wet_level  The wet signal level, range: [-96.0, 0.0], unit: dB
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_set_wet_level(esp_ae_reverb_handle_t handle, float wet_level);

/**
 * @brief  Get the value of wet level
 *
 * @param[in]   handle     The reverb handle
 * @param[out]  wet_level  The wet signal level
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_get_wet_level(esp_ae_reverb_handle_t handle, float *wet_level);

/**
 * @brief  Set the value of dry level
 *
 * @param[in]  handle     The reverb handle
 * @param[in]  dry_level  The dry signal level, range: [-96.0, 0.0], unit: dB
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_set_dry_level(esp_ae_reverb_handle_t handle, float dry_level);

/**
 * @brief  Get the value of dry level
 *
 * @param[in]   handle     The reverb handle
 * @param[out]  dry_level  The dry signal level
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_get_dry_level(esp_ae_reverb_handle_t handle, float *dry_level);

/**
 * @brief  Reset the internal processing state of the reverb handle while preserving user-configured settings.
 *         This function clears all delay lines and internal buffers, allowing efficient reuse of the handle
 *         without the overhead of destroying and recreating it when audio information remains unchanged.
 *         Typical use cases include:
 *         - Seek operations within the same audio stream
 *         - Starting playback of a new audio stream with identical audio information
 *
 * @note  This function is not thread-safe, and users must ensure correct call sequencing
 *         and avoid invoking this function while the process is running
 *
 * @param[in]  handle  The reverb handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_reverb_reset(esp_ae_reverb_handle_t handle);

/**
 * @brief  Deinitialize the reverb handle
 *
 * @param  handle  The reverb handle
 */
void esp_ae_reverb_close(esp_ae_reverb_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
