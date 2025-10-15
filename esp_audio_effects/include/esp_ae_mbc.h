/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Multi-Band Compressor(MBC) is an advanced audio effect that divides the audio signal into
 *         several frequency bands and applies compression independently to each band. This allows for precise control
 *         of the dynamics in each frequency range, enabling tailored compression for different parts of the audio spectrum.
 *
 *         This MBC is a four-band compressor. Users can set three frequency division points (low_fc, mid_fc, high_fc)
 *         to divide the audio signal into four frequency ranges(low frequency, low-mid frequency, high-mid frequency, high frequency).
 *         Each band has its own independent compression parameters, including(`threshold`, `ratio`, `attack_time`, `release_time`,
 *         `hold_time`, `knee_width`, `gain`).
 *
 *         The MBC have the function of solo and bypass which are used to control and monitor different frequency bands.
 *         The Solo allows user to isolate and listen to a specific frequency band while muting all other bands.
 *         This helps in focusing on the sound of that individual band without the influence of other frequency ranges.
 *         User can call the function `esp_ae_mbc_set_solo` and set `true` to a certain band which want to monitor.
 *         The Bypass lets the signal of a selected frequency band pass through without being processed by the compressor.
 *         In Bypass mode, the band’s compression is temporarily disabled, but the audio signal still passes through.
 *
 *         MBC processing is based on sampling points as processing units. The relationship
 *         between processing data length and sampling points is as follows:
 *         sample_num = data_length / (channel * (bits_per_sample >> 3))
 *
 *         MBC also offers two processing interfaces to fulfill the needs of interleaved and deinterleaved input data layouts.
 */

/**
 * @brief  Handle for Multi-Band Compressor (MBC)
 */
typedef void *esp_ae_mbc_handle_t;

/**
 * @brief  Enum for crossover point indices in the Multi-Band Compressor (MBC)
 *
 *         The audio spectrum is divided into four bands by three crossover frequencies: low_fc, mid_fc, and high_fc
 *
 *         |<------ low band ------>|<------ low-mid band ------>|<------ high-mid band ------>|<------ high band ------>|
 *         0Hz                      ^                            ^                             ^                        Nyquist
 *                                  |                            |                             |
 *                              low_fc(fc[0])                mid_fc(fc[1])                high_fc(fc[2])
 */
typedef enum {
    ESP_AE_MBC_FC_IDX_INVALID = -1,  /*!< Invalid crossover point index */
    ESP_AE_MBC_FC_IDX_LOW     = 0,   /*!< Low crossover point: defines the boundary between the lowest and low-mid frequency bands */
    ESP_AE_MBC_FC_IDX_MID     = 1,   /*!< Mid crossover point: defines the boundary between the low-mid and high-mid frequency bands */
    ESP_AE_MBC_FC_IDX_HIGH    = 2,   /*!< High crossover point: defines the boundary between the high-mid and highest frequency bands */
    ESP_AE_MBC_FC_IDX_MAX     = 3,   /*!< Maximum crossover point (used for boundary checks) */
} esp_ae_mbc_fc_idx_t;

/**
 * @brief  Enum for band indices in the Multi-Band Compressor (MBC)
 */
typedef enum {
    ESP_AE_MBC_BAND_IDX_INVALID  = -1,  /*!< Invalid band index */
    ESP_AE_MBC_BAND_IDX_LOW      = 0,   /*!< Low frequency band */
    ESP_AE_MBC_BAND_IDX_LOW_MID  = 1,   /*!< Low-mid frequency band */
    ESP_AE_MBC_BAND_IDX_MID_HIGH = 2,   /*!< High-mid frequency band */
    ESP_AE_MBC_BAND_IDX_HIGH     = 3,   /*!< High frequency band */
    ESP_AE_MBC_BAND_IDX_MAX      = 4,   /*!< Maximum band index (for boundary checks) */
} esp_ae_mbc_band_idx_t;

/**
 * @brief  Structure of Multi-Band Compressor parameter
 */
typedef struct {
    float     threshold;     /*!< When the level of the input signal exceeds the threshold, the input signal will be compressed.
                                  The range is (-100, 0), unit: dB */
    float     ratio;         /*!< The amount of compression applied once the signal exceeds the threshold. The range is [1, +∞) */
    float     makeup_gain;   /*!< The makeup gain applied after compression to adjust the output level. The range is [-10, 10], unit: dB */
    uint16_t  attack_time;   /*!< Response time of control gain attenuation. The range of attack time is [0, 500], unit: ms */
    uint16_t  release_time;  /*!< Response time of control gain recovery. The range of release time is [0, 500], unit: ms */
    uint16_t  hold_time;     /*!< The delay time to control gain change. The hold_time better set less than attack_time and release_time
                                  The range of hold time is [0, 100], unit: ms */
    float     knee_width;    /*!< The width of the transition region where the compressor moves from "no effect" to "full effect"
                                  as the input signal approaches the threshold, in dB. Range: [0, 10]
                                  - If knee_width = 0: Hard knee. Compression is applied immediately at the threshold,
                                    resulting in a sharp corner in the dynamic range curve
                                  - If knee_width > 0: Soft knee. Compression is gradually applied over a region around the threshold,
                                    resulting in a smoother transition. The larger the knee_width, the wider the dB range over
                                    which compression is gradually introduced */
} esp_ae_mbc_para_t;

/**
 * @brief  Configuration structure for Multi-Band Compressor (MBC)
 */
typedef struct {
    uint32_t           sample_rate;                        /*!< The audio sample rate */
    uint8_t            channel;                            /*!< The audio channel number */
    uint8_t            bits_per_sample;                    /*!< The audio bits per sample, supports 16, 24, 32 bits */
    uint32_t           fc[ESP_AE_MBC_FC_IDX_MAX];          /*!< Frequency of crossover points */
    esp_ae_mbc_para_t  mbc_para[ESP_AE_MBC_BAND_IDX_MAX];  /*!< Array of MBC parameter */
} esp_ae_mbc_config_t;

/**
 * @brief  Create an MBC handle based on the provided configuration
 *
 * @param[in]   cfg     MBC configuration
 * @param[out]  handle  The MBC handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_open(esp_ae_mbc_config_t *cfg, esp_ae_mbc_handle_t *handle);

/**
 * @brief  Process Multi-Band Compressor (MBC) for an audio stream with interleaved input data
 *
 * @note  The interleaved data is shown in the example:
 *        sample_num=10, channel=2, the data layout like [L1,R1,...L10,R10]
 *
 * @param[in]   handle       The MBC handle
 * @param[in]   sample_num   Number of sampling points processed by the MBC
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
esp_ae_err_t esp_ae_mbc_process(esp_ae_mbc_handle_t handle, uint32_t sample_num,
                                esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Process Multi-Band Compressor (MBC) for an audio stream with uninterleaved input data
 *
 * @note  The deinterleaved data is shown in the example:
 *        sample_num=10, channel=2, the array layout like [L1,...,L10], [R1,...,R10]
 *
 * @param[in]   handle       The MBC handle
 * @param[in]   sample_num   Number of sampling points processed by the MBC
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
esp_ae_err_t esp_ae_mbc_deintlv_process(esp_ae_mbc_handle_t handle, uint32_t sample_num,
                                        esp_ae_sample_t *in_samples, esp_ae_sample_t *out_samples);

/**
 * @brief  Set the compressor parameter for a specific band identified by 'band_idx'
 *
 * @param[in]  handle    The MBC handle
 * @param[in]  band_idx  The index of a specific band for which the parameters are to be set
 *                       eg: `ESP_AE_MBC_BAND_IDX_LOW refers` to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[in]  para      The compressor setup parameter
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_set_para(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx,
                                 esp_ae_mbc_para_t *mbc_para);

/**
 * @brief  Get the compressor parameter for a specific band identified by 'band_idx'
 *
 * @param[in]   handle    The MBC handle
 * @param[in]   band_idx  The index of a specific band for which the parameters are to be set
 *                        eg: `ESP_AE_MBC_BAND_IDX_LOW` refers to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[out]  para      The compressor setup parameter
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_get_para(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx,
                                 esp_ae_mbc_para_t *mbc_para);

/**
 * @brief  Set the frequency of crossover point identified by 'fc_idx'
 *
 * @param[in]  handle  The MBC handle
 * @param[in]  fc_idx  The index of a crossover point for which the frequency are to be set
 *                     eg: `ESP_AE_MBC_FC_IDX_LOW` refers to the first point, `ESP_AE_MBC_FC_IDX_HIGH` refers to the last point
 * @param[in]  fc      The frequency of crossover point
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_set_fc(esp_ae_mbc_handle_t handle, esp_ae_mbc_fc_idx_t fc_idx, uint32_t fc);

/**
 * @brief  Get the frequency of crossover point identified by 'fc_idx'
 *
 * @param[in]   handle  The MBC handle
 * @param[in]   fc_idx  The index of a crossover point for which the frequency are to be set
 *                      eg: `ESP_AE_MBC_FC_IDX_LOW` refers to the first point, `ESP_AE_MBC_FC_IDX_HIGH` refers to the last point
 * @param[out]  fc      The pointer of frequency of crossover point
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_get_fc(esp_ae_mbc_handle_t handle, esp_ae_mbc_fc_idx_t fc_idx, uint32_t *fc);

/**
 * @brief  Set whether to turn on the function of solo for a specific band identified by 'band_idx'
 *
 * @param[in]  handle       The MBC handle
 * @param[in]  band_idx     The index of a specific band for which the parameters are to be set
 *                          eg: `ESP_AE_MBC_BAND_IDX_LOW` refers to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[in]  enable_solo  The flag to turn on the solo function
 *                          True means turn on the solo for a specific band
 *                          False means turn off the solo for a specific band
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_set_solo(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx, bool enable_solo);

/**
 * @brief  Get the solo state for a specific band identified by 'band_idx'
 *
 * @param[in]   handle       The MBC handle
 * @param[in]   band_idx     The index of a specific band for which the parameters are to be set
 *                           eg: `ESP_AE_MBC_BAND_IDX_LOW` refers to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[out]  enable_solo  Pointer to store the solo state
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_get_solo(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx, bool *enable_solo);

/**
 * @brief  Set whether to turn on/off the function of bypass for a specific band identified by 'band_idx'
 *
 * @param[in]  handle         The MBC handle
 * @param[in]  band_idx       The index of a specific band for which the parameters are to be set
 *                            eg: `ESP_AE_MBC_BAND_IDX_LOW` refers to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[in]  enable_bypass  The flag to turn on/off the bypass function
 *                            True means turn on the bypass for a specific band
 *                            False means turn off the bypass for a specific band
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_set_bypass(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx, bool enable_bypass);

/**
 * @brief  Get the bypass state for a specific band identified by 'band_idx'
 *
 * @param[in]   handle         The MBC handle
 * @param[in]   band_idx       The index of a specific band for which the parameters are to be set
 *                             eg: `ESP_AE_MBC_BAND_IDX_LOW` refers to the first band, `ESP_AE_MBC_BAND_IDX_HIGH` refers to the last band
 * @param[out]  enable_bypass  Pointer to store the bypass state
 *                             True indicates the bypass is enabled for the specified band
 *                             False indicates the bypass is disabled for the specified band
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_get_bypass(esp_ae_mbc_handle_t handle, esp_ae_mbc_band_idx_t band_idx, bool *enable_bypass);

/**
 * @brief  Reset the internal processing state of the MBC handle while preserving user-configured settings
 *         It allows for efficient reuse of the handle when the audio information (such as sample rate, channel, bits per sample)
 *         remains unchanged, avoiding the overhead of closing and reopening the MBC handle
 *         Typical use cases include:
 *         - Seek operations within the same audio stream
 *         - Starting playback of a new audio stream with identical audio information
 * 
 * @note   This function is not thread-safe, and the user must ensure proper call sequencing and avoid invoking this function
 *         while the process is running
 *
 * @param[in]  handle  The MBC handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_mbc_reset(esp_ae_mbc_handle_t handle);

/**
 * @brief  Deinitialize MBC handle
 *
 * @param  handle  The MBC handle
 */
void esp_ae_mbc_close(esp_ae_mbc_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
