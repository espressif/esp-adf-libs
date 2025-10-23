/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "media_lib_common.h"

bool media_lib_verify(void *lib, int size)
{
    int i;
    void **check = (void **)lib;
    if (lib == NULL) {
        return false;
    }
    for (i = 0; i < size / sizeof(void *); i++) {
        if (check[i] == NULL) {
            return false;
        }
    }
    return true;
}