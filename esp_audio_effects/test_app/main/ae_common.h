/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define AE_TEST_CLAMPS(val, bits) \
    ((bits) == ESP_AE_BIT16 ? ae_test_saturate_16(val) : ((bits) == ESP_AE_BIT24 ? ae_test_saturate_24(val) : ae_test_saturate_32(val)))

#define MAX_RMS_DIFF_DIFF    0.2f
#define MAX_PEAK_DIFF_DIFF   0.2f
#define MAX_DC_OFFSET_CHANGE 0.3f
#define AE_TEST_PARAM_NUM(p) (sizeof(p) / sizeof(p[0]))

/**
 * @brief  Statistics accumulator structure for audio analysis
 */
typedef struct {
    double  sum_squares;    /*!< Sum of squared samples for RMS calculation */
    double  sum_dc;         /*!< Sum of samples for DC offset calculation */
    double  max_abs;        /*!< Maximum absolute sample value */
    int     total_samples;  /*!< Total number of samples processed */
} ae_test_stats_accumulator_t;

/**
 * @brief  Saturate a 32-bit value to 16-bit range
 *
 * @param[in]  val  Input 32-bit value
 * @return
 */
int16_t ae_test_saturate_16(int32_t val);

/**
 * @brief  Saturate a 32-bit value to 24-bit range
 *
 * @param[in]  val  Input 32-bit value
 * @return
 */
int32_t ae_test_saturate_24(int32_t val);

/**
 * @brief  Saturate a 64-bit value to 32-bit range
 *
 * @param[in]  val  Input 64-bit value
 * @return
 */
int32_t ae_test_saturate_32(int64_t val);

/**
 * @brief  Normalize audio sample value based on bits per sample
 *
 * @param[in]  data             Pointer to sample data
 * @param[in]  bits_per_sample  Number of bits per sample
 * @return
 */
float ae_test_normalize_value(uint8_t *data, int bits_per_sample);

/**
 * @brief  Accumulate audio statistics for analysis
 *
 * @param[in,out]  acc              Statistics accumulator
 * @param[in]      data             Audio data buffer
 * @param[in]      data_size        Size of audio data in bytes
 * @param[in]      bits_per_sample  Number of bits per sample
 */
void ae_test_accumulate_audio_stats(ae_test_stats_accumulator_t *acc, uint8_t *data,
                                    int data_size, uint8_t bits_per_sample);

/**
 * @brief  Analyze audio quality by comparing original and processed statistics
 *
 * @param[in]  orig_acc  Original audio statistics
 * @param[in]  proc_acc  Processed audio statistics
 * @return
 */
bool ae_test_analyze_audio_quality(ae_test_stats_accumulator_t *orig_acc, ae_test_stats_accumulator_t *proc_acc);

/**
 * @brief  Generate sine wave test signal at specified frequency
 *
 * @param[out]  buffer           Output buffer
 * @param[in]   duration_ms      Duration in milliseconds
 * @param[in]   sample_rate      Sample rate in Hz
 * @param[in]   amplitude_db     Amplitude in dB
 * @param[in]   bits_per_sample  Number of bits per sample
 * @param[in]   channels         Number of channels
 * @param[in]   frequency        Frequency in Hz
 * @return
 */
int ae_test_generate_sine_signal(void *buffer, int duration_ms, int sample_rate,
                                 float amplitude_db, int bits_per_sample, int channels, float frequency);

/**
 * @brief  Generate burst test signal
 *
 * @param[out]  buffer              Output buffer
 * @param[in]   total_samples       Total number of samples
 * @param[in]   sample_rate         Sample rate in Hz
 * @param[in]   frequency           Frequency in Hz
 * @param[in]   amplitude           Signal amplitude
 * @param[in]   burst_start_sample  Start sample of burst
 * @param[in]   burst_end_sample    End sample of burst
 * @return
 */
int ae_test_generate_burst_signal(int16_t *buffer, int total_samples, int sample_rate,
                                  float frequency, int16_t amplitude,
                                  int burst_start_sample, int burst_end_sample);

/**
 * @brief  Generate frequency sweep test signal
 *
 * @param[out]  buffer           Output buffer
 * @param[in]   duration_ms      Duration in milliseconds
 * @param[in]   sample_rate      Sample rate in Hz
 * @param[in]   amplitude_db     Amplitude in dB
 * @param[in]   bits_per_sample  Number of bits per sample
 * @param[in]   channels         Number of channels
 * @return
 */
int ae_test_generate_sweep_signal(void *buffer, int duration_ms, int sample_rate,
                                  float amplitude_db, int bits_per_sample, int channels);

/**
 * @brief  Calculate RMS value in dBFS
 *
 * @param[in]  buffer           Audio buffer
 * @param[in]  total_samples    Total number of samples
 * @param[in]  bits_per_sample  Number of bits per sample
 * @param[in]  channels         Number of channels
 * @return
 */
float ae_test_calculate_rms_dbfs(const void *buffer, int total_samples, int bits_per_sample, int channels);

/**
 * @brief  Analyze frequency response of audio signal
 *
 * @param[in]   input_buffer  Input audio buffer
 * @param[in]   num_samples   Number of samples
 * @param[in]   fft_size      FFT size
 * @param[in]   bits          Bits per sample
 * @param[in]   skip          Skip factor for analysis
 * @param[out]  spectrum      Output spectrum array
 */
void ae_test_analyze_frequency_response(uint8_t *input_buffer, int num_samples, uint32_t fft_size,
                                        uint8_t bits, uint8_t skip, float *spectrum);

/**
 * @brief  Read audio sample based on bit depth
 *
 * @param[in]  sample_ptr  Pointer to sample data
 * @param[in]  bits        Number of bits per sample
 * @return
 */
int32_t ae_test_read_sample(void *sample_ptr, uint8_t bits);

/**
 * @brief  Write audio sample based on bit depth
 *
 * @param[out]  ptr    Pointer to output buffer
 * @param[in]   value  Sample value
 * @param[in]   bits   Number of bits per sample
 */
void ae_test_write_sample(void *ptr, int value, uint8_t bits);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
