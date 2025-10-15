/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
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
 * @brief  Dynamic Range Control (DRC) is a technique used in audio signal processing that adjusts the dynamic range of
 *         an audio signal to adapt to different environments and devices. The dynamic range of an audio signal refers to
 *         the difference between the loudest and quietest parts of the signal. DRC includes compressors, limiters, and extenders,
 *         which process signals of different amplitudes by designing gain adjustment curves. Gain smoothing and compensation gain
 *         are key steps in DRC, ensuring signal quality while avoiding overload and noise.
 *
 *         DRC first needs to confirm the dynamic range curve, which describes the rule of how the output signal should change
 *         under different input signal logarithmic amplitudes. The setting of `point` and `knee_width` forms the
 *         dynamic range curve, where `point` determine where to start applying different control curves,
 *         and `knee_width` determines the smoothness of the curve's inflection point.
 *         For example, if the user wants to implement a compressor，can set three points (0.0, -10.0), (-50.0, -50.0), (-100.0, 100.0),
 *         it stated a compression curve with a threshold of -50 and a ratio of 1.25.
 *         `attack_time`, `release_time`, and `hold_time` are used to adjust the time parameters during compression or expansion processes, affecting the
 *         response speed and smoothness of the signal. `makeup_gain` is applied to lift or reduce the overall signal.
 *
 *         DRC processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         DRC also offers two processing interfaces to fulfill the needs of interleaved and deinterleaved input data layouts.
 */

/**
 * @brief  The handle of dynamic range control
 */
typedef void *esp_ae_drc_handle_t;

/**
 * @brief  Dynamic range control curve segmentation points
 */
typedef struct {
    float  x;  /*!< X-axis coordinate value which means logarithmic amplitude of input. The range is [-100.0, 0.0], unit: dB */
    float  y;  /*!< Y-axis coordinate value which means logarithmic amplitude of output. The range is [-100.0, 0.0], unit: dB */
} esp_ae_drc_curve_point;

/**
 * @brief  Structure of dynamic range control parameter
 */
typedef struct {
    esp_ae_drc_curve_point  *point;         /*!< Segmentation points for dynamic range control. The value of the x-axis in the segmentation point must have 0.0 and -100.0 dB,
                                                 And the value of x cannot be equal */
    uint8_t                  point_num;     /*!< Number of segmentation points. The maximum number of segmentation points is 6 */
    float                    makeup_gain;   /*!< Makeup gain for dynamic range control. The range of makeup gain is [-10.0, 10.0], unit: dB */
    float                    knee_width;    /*!< The width that makes the corners of the dynamic range curve smooth. The range of knee width is [0.0, 10.0]
                                                 If knee width is equal to 0, it means hard knee; otherwise means soft knee */
    uint16_t                 attack_time;   /*!< Response time of control gain attenuation. The range of attack time is [0, 500], unit: ms */
    uint16_t                 release_time;  /*!< Response time of recovery with increased gain. The range of release time is [0, 500], unit: ms */
    uint16_t                 hold_time;     /*!< The delay time to control gain change. The hold_time better set less than attack_time and release_time.
                                                 The range of release time is [0, 100], unit: ms */
} esp_ae_drc_para_t;

/**
 * @brief  Structure of dynamic range control configuration
 */
typedef struct {
    uint32_t           sample_rate;      /*!< The audio sample rate */
    uint8_t            channel;          /*!< The audio channel number */
    uint8_t            bits_per_sample;  /*!< Support bits per sample: 16, 24, 32 bit */
    esp_ae_drc_para_t  drc_para;         /*!< The dynamic range control parameter */
} esp_ae_drc_cfg_t;

/**
 * @brief  Create a DRC handle based on the provided configuration
 *
 * @param[in]   cfg     DRC configuration
 * @param[out]  handle  The DRC handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_open(esp_ae_drc_cfg_t *drc_cfg, esp_ae_drc_handle_t *handle);

/**
 * @brief  Process dynamic range control for an audio stream with interleaved input data
 *
 * @note  1. The interleaved data is shown in the example:
 *           sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The DRC handle
 * @param[in]   sample_num   Number of sampling points processed by the DRC
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
esp_ae_err_t esp_ae_drc_process(esp_ae_drc_handle_t handle, uint32_t sample_num,
                                esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Process dynamic range control for an audio stream with uninterleaved input data
 *
 * @note  1. The deinterleaved data is shown in the example:
 *           sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *        2. Support inplace processing
 *
 * @param[in]   handle       The DRC handle
 * @param[in]   sample_num   Number of sampling points processed by the DRC
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
esp_ae_err_t esp_ae_drc_deintlv_process(esp_ae_drc_handle_t handle, uint32_t sample_num,
                                        esp_ae_sample_t in_samples[], esp_ae_sample_t out_samples[]);

/**
 * @brief  Set the value of the attack time
 *
 * @param[in]  handle       The DRC handle
 * @param[in]  attack_time  The attack time, range: [0, 500], unit: ms
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_attack_time(esp_ae_drc_handle_t handle, uint16_t attack_time);

/**
 * @brief  Get the value of the attack time
 *
 * @param[in]   handle       The DRC handle
 * @param[out]  attack_time  The attack time
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_attack_time(esp_ae_drc_handle_t handle, uint16_t *attack_time);

/**
 * @brief  Set the value of the release time
 *
 * @param[in]  handle        The DRC handle
 * @param[in]  release_time  The release time, range: [0, 500], unit: ms
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_release_time(esp_ae_drc_handle_t handle, uint16_t release_time);

/**
 * @brief  Get the value of the release time
 *
 * @param[in]   handle        The DRC handle
 * @param[out]  release_time  The release time
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_release_time(esp_ae_drc_handle_t handle, uint16_t *release_time);

/**
 * @brief  Set the value of the hold time
 *
 * @param[in]  handle     The DRC handle
 * @param[in]  hold_time  The hold time, range: [0, 100], unit: ms
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_hold_time(esp_ae_drc_handle_t handle, uint16_t hold_time);

/**
 * @brief  Get the value of the hold time
 *
 * @param[in]   handle     The DRC handle
 * @param[out]  hold_time  The hold time
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_hold_time(esp_ae_drc_handle_t handle, uint16_t *hold_time);

/**
 * @brief  Set the value of the makeup gain
 *
 * @param[in]  handle       The DRC handle
 * @param[in]  makeup_gain  The makeup gain, range: [-10.0, 10.0], unit: dB
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_makeup_gain(esp_ae_drc_handle_t handle, float makeup_gain);

/**
 * @brief  Get the value of the makeup gain
 *
 * @param[in]   handle       The DRC handle
 * @param[out]  makeup_gain  The makeup gain
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_makeup_gain(esp_ae_drc_handle_t handle, float *makeup_gain);

/**
 * @brief  Set the value of the knee width
 *
 * @param[in]  handle      The DRC handle
 * @param[in]  knee_width  The knee width, range: [0.0, 10.0]
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_knee_width(esp_ae_drc_handle_t handle, float knee_width);

/**
 * @brief  Get the value of the knee width
 *
 * @param[in]   handle       The DRC handle
 * @param[out]  makeup_gain  The knee width
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_knee_width(esp_ae_drc_handle_t handle, float *knee_width);

/**
 * @brief  Set the value of the curve point
 *
 * @note  1. The point_num must be greater than or equal to 2 and less than or equal to 6
 *        2. The x-axis value and y-axis value of the point must be range of [-100.0, 0.0]
 *        3. The x-axis value of the different point must be unique
 *        4. The x-axis value of the point must have value of 0.0 and -100.0
 *
 * @param[in]  handle     The DRC handle
 * @param[in]  point      Dynamic range control curve segmentation points
 * @param[in]  point_num  Number of segmentation points
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_set_curve_points(esp_ae_drc_handle_t handle, esp_ae_drc_curve_point *point, uint8_t point_num);

/**
 * @brief  Get the number of the curve point
 *
 * @param[in]   handle     The DRC handle
 * @param[out]  point_num  Number of segmentation points
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_curve_point_num(esp_ae_drc_handle_t handle, uint8_t *point_num);

/**
 * @brief  Get the value of the curve point
 *
 * @note  The size of the `point` must be `sizeof(esp_ae_drc_curve_point) * point_num`,
 *        which `point_num` is returned by `esp_ae_drc_get_curve_point_num`
 *
 * @param[in]   handle  The DRC handle
 * @param[out]  point   Dynamic range control curve segmentation points
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_get_curve_points(esp_ae_drc_handle_t handle, esp_ae_drc_curve_point *point);

/**
 * @brief  Reset the internal processing state of the DRC handle while preserving user-configured settings
 *         This function allows efficient reuse of the handle without the overhead of destroying and recreating
 *         it when audio information (such as sample rate, channel, bits per sample) remains unchanged
 *         Typical use cases include:
 *         - Seek operations within the same audio stream
 *         - Starting playback of a new audio stream with identical audio information
 * 
 * @note   This function is not thread-safe, and users must ensure correct call sequencing 
 *         and avoid invoking this function while the process is running
 * 
 * @param[in]  handle  The DRC handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_drc_reset(esp_ae_drc_handle_t handle);

/**
 * @brief  Deinitialize the DRC handle
 *
 * @param  handle  The DRC handle
 */
void esp_ae_drc_close(esp_ae_drc_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
