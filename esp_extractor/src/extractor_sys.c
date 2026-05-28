/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include <stdatomic.h>

void extractor_atomic_inc(atomic_int *v)
{
    atomic_fetch_add(v, 1);
}

int extractor_atomic_load(atomic_int *v)
{
    return atomic_load(v);
}

int extractor_atomic_dec(atomic_int *v)
{
    return atomic_fetch_sub(v, 1);
}
