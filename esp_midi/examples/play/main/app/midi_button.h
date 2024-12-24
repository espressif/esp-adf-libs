/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Initialize the MIDI button
 */
void midi_button_init(void);

/**
 * @brief  MIDI button task
 *
 * @param  arg  Task argument
 */
void midi_button_task(void *arg);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
