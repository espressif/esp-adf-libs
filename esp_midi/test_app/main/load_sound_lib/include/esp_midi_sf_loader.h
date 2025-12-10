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

// #define USE_MEMOEY_STORE_SAMPLE_DATA

/**
 * @brief  Loads the sound font file and returns a handle for subsequent operations
 *
 * @param  sf_name     The name or path of the sound font file
 * @param  lib_handle  A pointer to store the handle of the loaded library
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sf_loader_new(esp_midi_lib_loader_cfg_t sf_name, esp_midi_lib_loader_handle_t *lib_handle);

/**
 * @brief  Deletes (unloads) the sound font library loaded via esp_midi_sf_loader_new
 *
 * @param  lib_handle  MIDI loader handle to delete
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_loader_delete(esp_midi_lib_loader_handle_t lib_handle);

/**
 * @brief  Sets the sample data for playback
 *         This may involve configuring certain properties of the sample, such as the start point, length, etc.
 *
 * @param  lib_handle   MIDI loader handle
 * @param  sample_desc  A pointer to the sample description structure containing detailed information about the sample
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_loader_set_data(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Frees the previously set sample data
 *         When the sample is no longer needed for playback, this function should be called to release related resources
 *
 * @param  lib_handle   MIDI loader handle
 * @param  sample_desc  A pointer to the sample description structure that was previously set
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_loader_free_data(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

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
esp_midi_err_t esp_midi_sf_loader_noteon(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Sets the currently used sound bank
 *         MIDI sound banks typically contain multiple sound sets (banks), each containing multiple programs (sounds)
 *
 * @param  lib_handle  MIDI loader handle
 * @param  bank        The number of the sound bank to set
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_loader_set_bank(esp_midi_lib_loader_handle_t lib_handle, int32_t bank);

/**
 * @brief  Sets the currently used program (sound)
 *         Selects a specific program (sound) within the specified sound bank for playback
 *
 * @param  lib_handle  MIDI loader handle.
 * @param  prenum      The number of the program to set.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_loader_set_program(esp_midi_lib_loader_handle_t lib_handle, int32_t prenum);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
