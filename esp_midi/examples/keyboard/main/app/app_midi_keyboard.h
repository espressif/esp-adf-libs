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

#define KEYBOARD_INPUT_WAIT_TICK 10
#if CONFIG_INPUT_KEY_USB_KEYBOARD
#define KEYBOARD_INPUT_NOTE_NUMBER 24
#elif CONFIG_INPUT_KEY_MATRIX_KEYBOARD
#define KEYBOARD_INPUT_NOTE_NUMBER 25
#endif  /* CONFIG_INPUT_KEY_USB_KEYBOARD */

typedef enum {
    KEYBOARD_INPUT_BUTTON   = 0,
    KEYBOARD_INPUT_KNOB     = 1,
    KEYBOARD_INPUT_MATRIX   = 2,
    KEYBOARD_INPUT_USB_HID  = 3,
    KEYBOARD_INPUT_MAX      = 4,
    KEYBOARD_INPUT_TASK_DEL = -1,
} keyboard_input_type_t;

typedef struct {
    keyboard_input_type_t  type;
    int                    value1;
    int                    value2;
} keyboard_input_t;

/**
 * @brief  Initialize keyboard input
 *
 * @return
 */
void *keyboard_input_init(void);

/**
 * @brief  Deinitialize keyboard input
 */
void keyboard_input_deinit(void);

/**
 * @brief  Get current instrument index
 *
 * @return
 */
int midi_player_get_current_instr_index(void);

/**
 * @brief  Initialize MIDI player
 *
 * @return
 *       - ESP_OK    Succeeded
 *       - ESP_FAIL  Fail
 */
esp_err_t midi_player_init(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
