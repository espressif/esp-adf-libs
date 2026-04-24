/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Audio information structure
 */
typedef struct {
    int  sample_rate;      /*!< The audio sample rate */
    int  channels;         /*!< The number of audio channels */
    int  bits_per_sample;  /*!< The audio bits per sample */
} ae_audio_info_t;

/**
 * @brief  Ensure wifi_fs is connected and mounted
 *
 * @param[in]  mount_point  VFS mount point (e.g. "/sdcard")
 *
 * @return
 *       - ESP_OK    Already ready or just mounted
 *       - ESP_FAIL  Wi-Fi connect or mount failed
 */
esp_err_t ae_test_ensure_wifi_fs_ready(const char *mount_point);

/**
 * @brief  Unmount wifi_fs and disconnect Wi-Fi
 *
 * @param[in]  mount_point  VFS mount point used in ae_test_ensure_wifi_fs_ready
 *
 * @return
 *       - ESP_OK    Unmounted successfully
 *       - ESP_FAIL  Unmount failed
 */
esp_err_t ae_test_wifi_fs_cleanup(const char *mount_point);

/**
 * @brief  Parse a WAV file header and extract audio info
 *
 * @param[in]   fp          Opened file pointer (positioned at start)
 * @param[out]  audio_info  Parsed audio parameters
 * @param[out]  data_size   Size of the "data" chunk in bytes
 *
 * @return
 *       - true   Success
 *       - false  Parse error
 */
bool ae_test_parse_wav_header(FILE *fp, ae_audio_info_t *audio_info, uint32_t *data_size);

/**
 * @brief  Write (or overwrite) a WAV file header
 *
 * @param[in]  fp          Opened file pointer
 * @param[in]  audio_info  Audio parameters to write
 * @param[in]  data_size   Size of the PCM data that follows
 *
 * @return
 *       - true   Success
 *       - false  Write error
 */
bool ae_test_write_wav_header(FILE *fp, const ae_audio_info_t *audio_info, uint32_t data_size);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
