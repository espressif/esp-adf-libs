/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Howling suppression (HS) detects and attenuates acoustic feedback (howl) in a loop formed by
 *         microphone, amplifier, and speaker. It uses FFT-based spectral analysis and multiple criteria
 *         to distinguish a narrow, sustained howl from normal speech or music.
 *
 *         Detection uses PAPR (Peak to Average Power Ratio), PHPR (Peak to Harmonic Power Ratio),
 *         PNPR (Peak to Noise Power Ratio), and optionally IMSD (Inter-Frame Magnitude Spectral Deviation).
 *         When a frequency bin exceeds the configured thresholds, it is treated as a howling candidate.
 *         The algorithm applies dynamic biquad notch filters at those frequencies and may reduce global
 *         gain to lower loop gain and limit new howl peaks.
 *
 *         howl processing is frame-based. The internal FFT block length depends on sample rate (e.g. 512
 *         samples at 16 kHz). The relationship between frame byte length and samples is:
 *         frame_size_bytes = esp_ae_howl_get_frame_size(...);
 *         samples_per_channel = frame_size_bytes / (channel * (bits_per_sample >> 3)).
 *         Each `esp_ae_howl_process` / `esp_ae_howl_deintlv_process` call must use exactly one such frame.
 *
 *         HS also offers two processing interfaces for interleaved and deinterleaved input layouts.
 */

/**
 * @brief  Handle for howling suppression
 */
typedef void *esp_ae_howl_handle_t;

/**
 * @brief  Configuration structure for howling suppression
 */
typedef struct {
    uint32_t  sample_rate;      /*!< The audio sample rate; supported: 8000, 16000, 24000, 32000, 44100, 48000 */
    uint8_t   channel;          /*!< The audio channel number */
    uint8_t   bits_per_sample;  /*!< The audio bits per sample; supports 16, 24, 32 bits */
    float     papr_th;          /*!< PAPR (dB) threshold: if measured PAPR of a bin exceeds papr_th, that bin
                                     tends toward howl under the PAPR criterion. Valid range: [-10.0, 20.0] */
    float     phpr_th;          /*!< PHPR (dB) threshold: if measured PHPR exceeds phpr_th, that bin tends
                                     toward howl under the PHPR criterion. Valid range: [0.0, 100.0] */
    float     pnpr_th;          /*!< PNPR (dB) threshold: if measured PNPR exceeds pnpr_th, that bin tends
                                     toward howl under the PNPR criterion. Valid range: [0.0, 100.0] */
    float     imsd_th;          /*!< Only valid when enable_imsd = true. IMSD threshold: if measured IMSD is below imsd_th,
                                     that bin tends toward howl under the IMSD criterion (only when enable_imsd is true).
                                     Valid range: [0.0, 20.0] */
    bool      enable_imsd;      /*!< Enable IMSD for spectral stability (higher memory and CPU cost),
                                     suitable for music or mixed content; set false for speech-only to save cost */
} esp_ae_howl_cfg_t;

/**
 * @brief  Create a howling suppression handle from the provided configuration
 *
 * @param[in]   cfg     howl configuration
 * @param[out]  handle  The howl handle. If an error occurs, the result will be a NULL pointer
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_MEM_LACK           Failed to allocate memory
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_howl_open(esp_ae_howl_cfg_t *cfg, esp_ae_howl_handle_t *handle);

/**
 * @brief  Get the frame size in bytes required for each process call
 *
 * @param[in]   handle      The howl handle
 * @param[out]  frame_size  Frame size in bytes: block_len * channel * (bits_per_sample / 8)
 *                          The block_len is the internal FFT length chosen automatically at open time
 *                          based on sample rate: 512 samples when sample_rate < 32000 Hz, otherwise
 *                          1024 samples. It determines the frequency resolution and per-frame latency
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_howl_get_frame_size(esp_ae_howl_handle_t handle, uint32_t *frame_size);

/**
 * @brief  Run howling suppression on interleaved PCM
 *
 * @note  1) The interleaved layout example: channel=2, one frame of samples_per_channel = N per channel,
 *           data layout like [L1,R1, L2,R2, ... Ln,Rn], total bytes = N * channel * (bits_per_sample >> 3),
 *           which must equal the value returned by esp_ae_howl_get_frame_size. If use want to support
 *           arbitary length of frame data, need cached at user layer
 *        2) Supports in-place processing: in_samples and out_samples may be the same pointer.
 *           Input byte length must be exactly the frame size from esp_ae_howl_get_frame_size
 *
 * @param[in]   handle       The howl handle
 * @param[in]   in_samples   Input audio buffer (interleaved)
 * @param[out]  out_samples  Output audio buffer (interleaved). May equal in_samples
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_howl_process(esp_ae_howl_handle_t handle, esp_ae_sample_t in_samples, esp_ae_sample_t out_samples);

/**
 * @brief  Run howling suppression on deinterleaved PCM (one buffer per channel)
 *
 * @note  1) The deinterleaved layout example: channel=2, arrays like [L1,...,Ln], [R1,...,Rn], each channel
 *           buffer length in bytes is samples_per_channel * (bits_per_sample >> 3), and the sum across channels
 *           matches one frame from esp_ae_howl_get_frame_size in sample-aligned terms. If use want to support
 *           arbitary length of frame data, need cached at user layer
 *        2) Supports in-place processing per channel: out_samples[ch] may equal in_samples[ch]
 *
 * @param[in]   handle       The howl handle
 * @param[in]   in_samples   Array of channel input buffers (one pointer per channel)
 * @param[out]  out_samples  Array of channel output buffers. May equal in_samples element-wise
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_howl_deintlv_process(esp_ae_howl_handle_t handle, esp_ae_sample_t *in_samples, esp_ae_sample_t *out_samples);

/**
 * @brief  Reset the internal processing state of howl handle while preserving user-configured gain settings
 *         It allows the handle to be reused efficiently when the audio information (sample rate, channel, bits per sample)
 *         remains unchanged, avoiding the overhead of closing and recreating the howl handle
 *         Typical use cases include:
 *         - Seek operations within the same audio stream
 *         - Starting playback of a new audio stream with identical audio information
 *
 * @note  This function is not thread-safe. The user must ensure proper call sequencing and avoid invoking
 *        this function while another context is using the same handle to process audio
 *
 * @param[in]  handle  The howl handle
 *
 * @return
 *       - ESP_AE_ERR_OK                 Operation succeeded
 *       - ESP_AE_ERR_INVALID_PARAMETER  Invalid input parameter
 */
esp_ae_err_t esp_ae_howl_reset(esp_ae_howl_handle_t handle);

/**
 * @brief  Deinitialize howl handle and release resources
 *
 * @param  handle  The howl handle
 */
void esp_ae_howl_close(esp_ae_howl_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
