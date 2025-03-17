// Copyright 2024 Espressif Systems (Shanghai) CO., LTD.
// All rights reserved.

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
