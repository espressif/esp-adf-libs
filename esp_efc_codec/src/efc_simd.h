/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * @file  efc_simd.h
 *
 * @brief  Bulk pixel move used by the EFC decoder
 *
 *         On ESP32-S3 and ESP32-P4 the copy is a 128-bit vector memcpy. Other
 *         targets use a portable C copy.
 */

#pragma once

#include <stdint.h>
#include "efc_codec.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Copy bytes from `src` to `dst`
 *
 * @param[out]  dst           Destination buffer
 * @param[in]   src           Source buffer
 * @param[in]   length_bytes  Number of bytes to copy
 */
void esp_efc_memcpy_simd(void *dst, const void *src, int length_bytes);

/**
 * @brief  Bilinear-scale one RGB565 output row using precomputed X maps
 */
void esp_efc_bilinear_row_rgb565(const efc_bilinear_row_args_t *args);

/**
 * @brief  Nearest-neighbor RGB565 row using precomputed source indices
 */
void esp_efc_nn_row_rgb565(uint16_t *dst, const uint16_t *src,
                           const uint16_t *x_idx, int dst_w);

/**
 * @brief  Copy RGB565 pixels
 *
 * @param[out]  dst  Destination pixels
 * @param[in]   src  Source pixels
 * @param[in]   n    Number of pixels
 */
static inline void esp_efc_copy_u16(uint16_t *dst, const uint16_t *src, int n)
{
    esp_efc_memcpy_simd(dst, src, n * 2);
}

/**
 * @brief  Fill RGB565 pixels with one value
 *
 * @param[out]  dst  Destination pixels
 * @param[in]   v    RGB565 value
 * @param[in]   n    Number of pixels
 */
static inline void esp_efc_fill_u16(uint16_t *dst, uint16_t v, int n)
{
    for (int i = 0; i < n; i++) {
        dst[i] = v;
    }
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
