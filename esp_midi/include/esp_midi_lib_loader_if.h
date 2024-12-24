/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_midi_lib_loader_defines.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  MIDI sound font loader handle
 */
typedef void *esp_midi_lib_handle_t;

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_lib_loader(esp_midi_lib_cfg_t *cfg, esp_midi_lib_handle_t *handle);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_lib_note_on(esp_midi_lib_handle_t handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_lib_set_bank(esp_midi_lib_handle_t handle, int32_t bank);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_lib_set_preset(esp_midi_lib_handle_t handle, int32_t preset);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_lib_set_data(esp_midi_lib_handle_t handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_lib_free_data(esp_midi_lib_handle_t handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Creates a MIDI SoundFont loader handle using the provided configuration
 *
 * @param  cfg     MIDI player configure information
 * @param  handle  MIDI player handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Succeeded
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_lib_delete(esp_midi_lib_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
