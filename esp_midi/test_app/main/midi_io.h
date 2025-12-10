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
 * @brief  Mount the SD card
 */
void mount_sd(void);

/**
 * @brief  Unmount the SD card
 */
void unmount_sd(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
