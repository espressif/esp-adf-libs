/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_midi_lib_loader_if.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Loads the specified sound font file and returns a handle for subsequent operations
 *
 * @param  cfg         Currently it isn't use
 * @param  lib_handle  A pointer to store the handle of the loaded library
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_flash_loader_new(esp_midi_lib_loader_cfg_t cfg, esp_midi_lib_loader_handle_t *lib_handle);

/**
 * @brief  Deletes (unloads) the sound font library loaded via esp_midi_files_loader_new
 *
 * @param  lib_handle  MIDI loader handle to delete
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_flash_loader_delete(esp_midi_lib_loader_handle_t lib_handle);

/**
 * @brief  Triggers the playback of a sample (Note On)
 *         This typically involves starting the playback of the sample, possibly in response to a MIDI Note On message
 *
 * @param  lib_handle   MIDI loader handle
 * @param  sample_desc  A pointer to the sample description structure containing the sample information to play
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_flash_loader_noteon(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
