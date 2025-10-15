/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unity.h"
#include "esp_log.h"
#include "ae_common.h"
#include "esp_ae_types.h"
#include "esp_dsp.h"
#include "esp_heap_caps.h"

#define TAG "TEST_COMMON"
#define AE_TEST_MAX_S32 ((int32_t)0x7fffffffL)
#define AE_TEST_MIN_S32 ((int32_t)0x80000000L)
#define AE_TEST_MAX_S24 ((int32_t)0x7fffff)
#define AE_TEST_MIN_S24 ((int32_t)0xff800000)
#define AE_TEST_MAX_S16 ((int16_t)0x7fff)
#define AE_TEST_MIN_S16 ((int16_t)0x8000)
#define AE_TEST_GET_MAX_VAL(bits) ((bits) == ESP_AE_BIT16 ? AE_TEST_MAX_S16 :  \
                                   ((bits) == ESP_AE_BIT24 ? AE_TEST_MAX_S24 : \
                                                             AE_TEST_MAX_S32))

static void ae_test_calculate_final_audio_stats(ae_test_stats_accumulator_t *acc, float *rms, float *peak, float *dc_offset)
{
    if (acc->total_samples > 0) {
        *rms = sqrt(acc->sum_squares / acc->total_samples);
        *peak = acc->max_abs;
        *dc_offset = acc->sum_dc / acc->total_samples;
    } else {
        *rms = 0.0f;
        *peak = 0.0f;
        *dc_offset = 0.0f;
    }
}

float ae_test_normalize_value(uint8_t *data, int bits_per_sample)
{
    switch (bits_per_sample) {
        case 8: {
            uint8_t sample = data[0];
            return (float)(sample - 128) / 128.0f;
        }
        case 16: {
            int16_t sample = *(int16_t *)&data[0];
            return (float)sample / AE_TEST_GET_MAX_VAL(bits_per_sample);
        }
        case 24: {
            int32_t sample = data[0] | (data[1] << 8) | (data[2] << 16);
            if (sample & 0x800000) {
                sample |= 0xFF000000;
            }
            return (float)sample / AE_TEST_GET_MAX_VAL(bits_per_sample);
        }
        case 32: {
            int32_t sample = *(int32_t *)&data[0];
            return (float)sample / AE_TEST_GET_MAX_VAL(bits_per_sample);
        }
        default:
            return 0.0f;
    }
}

void ae_test_accumulate_audio_stats(ae_test_stats_accumulator_t *acc, uint8_t *data, int data_size, uint8_t bits_per_sample)
{
    int bytes_per_sample = bits_per_sample / 8;
    int total_samples = data_size / bytes_per_sample;
    for (int i = 0; i < total_samples; i++) {
        int byte_offset = i * bytes_per_sample;
        uint8_t *sample_data = data + byte_offset;
        float sample_value = ae_test_normalize_value(sample_data, bits_per_sample);
        acc->sum_squares += sample_value * sample_value;
        acc->sum_dc += sample_value;
        float abs_value = fabs(sample_value);
        if (abs_value > acc->max_abs) {
            acc->max_abs = abs_value;
        }
    }
    acc->total_samples += total_samples;
}

bool ae_test_analyze_audio_quality(ae_test_stats_accumulator_t *orig_acc, ae_test_stats_accumulator_t *proc_acc)
{
    float orig_rms, orig_peak, orig_dc;
    float proc_rms, proc_peak, proc_dc;
    ae_test_calculate_final_audio_stats(orig_acc, &orig_rms, &orig_peak, &orig_dc);
    ae_test_calculate_final_audio_stats(proc_acc, &proc_rms, &proc_peak, &proc_dc);
    float rms_diff_percent = fabs(proc_rms - orig_rms) / orig_rms;
    float peak_diff_percent = fabs(proc_peak - orig_peak) / orig_peak;
    float dc_offset_change = fabs(proc_dc - orig_dc);
    bool rms_pass = rms_diff_percent <= MAX_RMS_DIFF_DIFF;
    bool peak_pass = peak_diff_percent <= MAX_PEAK_DIFF_DIFF;
    bool dc_offset_pass = dc_offset_change <= MAX_DC_OFFSET_CHANGE;
    ESP_LOGD(TAG, "orig_rms: %f, orig_peak: %f, orig_dc: %f", orig_rms, orig_peak, orig_dc);
    ESP_LOGD(TAG, "proc_rms: %f, proc_peak: %f, proc_dc: %f", proc_rms, proc_peak, proc_dc);
    ESP_LOGD(TAG, "rms_pass: %d, peak_pass: %d, dc_offset_pass: %d", rms_pass, peak_pass, dc_offset_pass);
    return rms_pass && peak_pass && dc_offset_pass;
}

int ae_test_generate_sine_signal(void *buffer, int duration_ms, int sample_rate,
                                 float amplitude_db, int bits_per_sample, int channels, float frequency)
{
    int frame_num = (duration_ms * sample_rate) / 1000;
    int total_samples = frame_num * channels;
    int64_t max_amplitude = AE_TEST_GET_MAX_VAL(bits_per_sample);
    double amplitude_linear = pow(10.0, amplitude_db / 20.0);
    int64_t amplitude = (int64_t)(amplitude_linear * max_amplitude);
    if (amplitude > max_amplitude) {
        amplitude = max_amplitude;
    }
    for (int frame = 0; frame < frame_num; frame++) {
        double t = (double)frame / sample_rate;
        double sample = sin(2.0 * M_PI * frequency * t);
        int64_t sample_value = (int64_t)(amplitude * sample);

        if (bits_per_sample == 16) {
            int16_t *buf16 = (int16_t *)buffer;
            int16_t val16 = (int16_t)(sample_value);
            for (int ch = 0; ch < channels; ch++) {
                buf16[frame * channels + ch] = val16;
            }
        } else if (bits_per_sample == 24) {
            uint8_t *buf24 = (uint8_t *)buffer;
            for (int ch = 0; ch < channels; ch++) {
                int sample_index = (frame * channels + ch) * 3;
                buf24[sample_index + 0] = (uint8_t)(sample_value & 0xFF);
                buf24[sample_index + 1] = (uint8_t)((sample_value >> 8) & 0xFF);
                buf24[sample_index + 2] = (uint8_t)((sample_value >> 16) & 0xFF);
            }
        } else if (bits_per_sample == 32) {
            int32_t *buf32 = (int32_t *)buffer;
            int32_t val32 = (int32_t)(sample_value);
            for (int ch = 0; ch < channels; ch++) {
                buf32[frame * channels + ch] = val32;
            }
        }
    }
    return total_samples;
}

int ae_test_generate_burst_signal(int16_t *buffer, int total_samples, int sample_rate,
                                  float frequency, int16_t amplitude,
                                  int burst_start_sample, int burst_end_sample)
{
    for (int i = 0; i < total_samples; i++) {
        if (i >= burst_start_sample && i < burst_end_sample) {
            buffer[i] = amplitude;
        } else {
            buffer[i] = 5;
        }
    }
    return 0;
}

int ae_test_generate_sweep_signal(void *buffer, int duration_ms, int sample_rate,
                                  float amplitude_db, int bits_per_sample, int channels)
{
    int num_samples = (duration_ms * sample_rate) / 1000;
    float amplitude = powf(10.0f, amplitude_db / 20.0f);
    float f1 = 20.0f;
    float f2 = (float)sample_rate / 2.0f;
    float T = (float)duration_ms / 1000.0f;

    if (f2 > sample_rate / 2.0f) {
        f2 = sample_rate / 2.0f;
    }

    float freq_delta = f2 - f1;
    float freq_rate = freq_delta / T;

    for (int i = 0; i < num_samples; i++) {
        float t = (float)i / sample_rate;
        if (t > T) {
            t = T;
        }
        float phase = 2.0f * M_PI * (f1 * t + freq_rate * t * t * 0.5f);
        float sample_value = amplitude * sinf(phase);

        for (int ch = 0; ch < channels; ch++) {
            if (bits_per_sample == 16) {
                int16_t sample_16 = (int16_t)lrintf(sample_value * AE_TEST_MAX_S16);
                ((int16_t *)buffer)[i * channels + ch] = sample_16;
            } else if (bits_per_sample == 24) {
                int32_t sample_24 = (int32_t)lrintf(sample_value * AE_TEST_MAX_S24);
                uint8_t *p = (uint8_t *)buffer + (i * channels + ch) * 3;
                p[0] = (uint8_t)(sample_24 & 0xFF);
                p[1] = (uint8_t)((sample_24 >> 8) & 0xFF);
                p[2] = (uint8_t)((sample_24 >> 16) & 0xFF);
            } else if (bits_per_sample == 32) {
                int32_t sample_32 = (int32_t)lrintf(sample_value * AE_TEST_MAX_S32);
                ((int32_t *)buffer)[i * channels + ch] = sample_32;
            }
        }
    }
    return num_samples;
}

float ae_test_calculate_rms_dbfs(const void *buffer, int total_samples, int bits_per_sample, int channels)
{
    double sum = 0.0;
    int frame_num = total_samples;
    double max_value = AE_TEST_GET_MAX_VAL(bits_per_sample);
    if (bits_per_sample == 16) {
        const int16_t *buf16 = (const int16_t *)buffer;
        for (int frame = 0; frame < frame_num; frame++) {
            double frame_energy = 0.0;
            for (int ch = 0; ch < channels; ch++) {
                double sample = buf16[frame * channels + ch];
                frame_energy += sample * sample;
            }
            sum += frame_energy / channels;
        }
    } else if (bits_per_sample == 24) {
        const uint8_t *buf24 = (const uint8_t *)buffer;
        for (int frame = 0; frame < frame_num; frame++) {
            double frame_energy = 0.0;
            for (int ch = 0; ch < channels; ch++) {
                int sample_index = (frame * channels + ch) * 3;
                int32_t sample = (int32_t)(buf24[sample_index] | (buf24[sample_index + 1] << 8) | (buf24[sample_index + 2] << 16));
                if (sample & 0x800000) {
                    sample |= 0xFF000000;
                }
                frame_energy += (double)sample * sample;
            }
            sum += frame_energy / channels;
        }
    } else if (bits_per_sample == 32) {
        const int32_t *buf32 = (const int32_t *)buffer;
        for (int frame = 0; frame < frame_num; frame++) {
            double frame_energy = 0.0;
            for (int ch = 0; ch < channels; ch++) {
                double sample = buf32[frame * channels + ch];
                frame_energy += sample * sample;
            }
            sum += frame_energy / channels;
        }
    }
    double mean = sum / frame_num;
    double rms = sqrt(mean);
    return 20.0 * log10(rms / max_value + 1e-20);
}

void ae_test_analyze_frequency_response(uint8_t *input_buffer, int num_samples, uint32_t fft_size,
                                        uint8_t bits, uint8_t skip, float *spectrum)
{
    dsps_fft2r_init_fc32(NULL, fft_size);
    float *fft_data = (float *)heap_caps_aligned_calloc(fft_size, 2 * fft_size, sizeof(float),
                                                        MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    TEST_ASSERT_NOT_NULL(fft_data);
    for (int k = 0; k < fft_size / 2; k++) {
        spectrum[k] = -200.0f;
    }
    for (int start = 0; start + fft_size <= num_samples; start += fft_size / 2) {
        for (int i = 0; i < fft_size; i++) {
            int sample_idx = start + i;
            int offset = sample_idx * skip * (bits >> 3);
            uint8_t *sample_data = input_buffer + offset;
            float sample = ae_test_normalize_value(sample_data, bits);
            fft_data[2 * i] = sample;
            fft_data[2 * i + 1] = 0.0f;
        }
        dsps_fft2r_fc32(fft_data, fft_size);
        dsps_bit_rev_fc32(fft_data, fft_size);
        dsps_cplx2reC_fc32(fft_data, fft_size);
        for (int k = 0; k < fft_size / 2; k++) {
            float re = fft_data[2 * k];
            float im = fft_data[2 * k + 1];
            float magnitude = sqrtf(re * re + im * im);
            float normalized_magnitude = magnitude / (fft_size / 2);
            float magnitude_db = 20.0f * log10f(normalized_magnitude + 1e-12f) + 3.0f;
            if (magnitude_db > spectrum[k]) {
                spectrum[k] = magnitude_db;
            }
        }
    }
    free(fft_data);
    fft_data = NULL;
    dsps_fft2r_deinit_fc32();
}

void ae_test_write_sample(void *ptr, int value, uint8_t bits)
{
    switch (bits) {
        case ESP_AE_BIT16:
            *(int16_t *)ptr = value;
            break;
        case ESP_AE_BIT24:
            uint8_t *bytes = (uint8_t *)ptr;
            bytes[0] = 0;
            bytes[1] = value & 0xFF;
            bytes[2] = (value >> 8) & 0xFF;
            break;
        case ESP_AE_BIT32:
            *(int32_t *)ptr = value;
            break;
    }
}

int32_t ae_test_read_sample(void *sample_ptr, uint8_t bits)
{
    int32_t value = 0;
    uint8_t *bytes;
    switch (bits) {
        case ESP_AE_BIT8:
            value = *(uint8_t *)sample_ptr;
            break;
        case ESP_AE_BIT16:
            value = *(int16_t *)sample_ptr;
            break;
        case ESP_AE_BIT24:
            bytes = (uint8_t *)sample_ptr;
            value = (int32_t)((int32_t)bytes[0] | ((int32_t)bytes[1] << 8) | ((int32_t)bytes[2] << 16));
            if (value & 0x800000) {
                value |= 0xFF000000;
            }
            break;
        case ESP_AE_BIT32:
            value = *(int32_t *)sample_ptr;
            break;
    }
    return value;
}

int16_t ae_test_saturate_16(int32_t val)
{
    if (val > AE_TEST_MAX_S16) {
        val = AE_TEST_MAX_S16;
    } else if (val < AE_TEST_MIN_S16) {
        val = AE_TEST_MIN_S16;
    }
    return ((int16_t)val);
}

int32_t ae_test_saturate_24(int32_t val)
{
    if (val > AE_TEST_MAX_S24) {
        val = AE_TEST_MAX_S24;
    } else if (val < AE_TEST_MIN_S24) {
        val = AE_TEST_MIN_S24;
    }
    return ((int32_t)val);
}

int32_t ae_test_saturate_32(int64_t val)
{
    if (val > AE_TEST_MAX_S32) {
        val = AE_TEST_MAX_S32;
    } else if (val < AE_TEST_MIN_S32) {
        val = AE_TEST_MIN_S32;
    }
    return ((int32_t)val);
}
