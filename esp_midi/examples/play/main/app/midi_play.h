/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Initialize the MIDI player
 */
void midi_player_init(void);

/**
 * @brief  Increase the MIDI volume
 */
void midi_volume_increase(void);

/**
 * @brief  Decrease the MIDI volume
 */
void midi_volume_decrease(void);

/**
 * @brief  Increase the MIDI file index
 */
void midi_file_index_increase(void);

/**
 * @brief  MIDI play task
 *
 * @param  arg  Task argument
 */
void midi_play_task(void *arg);

/**
 * @brief  Trigger play event
 */
void midi_event_play(void);

/**
 * @brief  Trigger mute event
 */
void midi_event_mute(void);

/**
 * @brief  Trigger record event
 */
void midi_event_rec(void);

/**
 * @brief  Pause MIDI playback
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t midi_play_pause(void);

/**
 * @brief  Set MIDI playback BPM
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t midi_play_set_bpm(void);

/**
 * @brief  Close MIDI playback
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t midi_play_close(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
