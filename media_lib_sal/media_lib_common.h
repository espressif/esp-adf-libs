/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdbool.h>
#include <string.h>
#include "esp_err.h"

/**
 * @brief  Verify library functions pointer all set or not
 *
 * @param  lib   Wrapper function struct pointer
 * @param  size  Wrapper struct size
 * @return
 *       - true   Library verify OK
 *       - false  Library verify Fail
 */
bool media_lib_verify(void *lib, int size);

#define MEDIA_LIB_DEFAULT_INSTALLER(src, dst, type) if (media_lib_verify(src, sizeof(type)) == false) {  \
    return ESP_ERR_INVALID_ARG;                                                                          \
    }                                                                                                    \
    memcpy(dst, src, sizeof(type));                                                                      \
    return ESP_OK;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_COMMON_H */
