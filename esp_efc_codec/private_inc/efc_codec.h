/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */
#pragma once

#include "esp_efc_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*efc_memcpy_t)(void *dst, const void *src, int length_bytes);

typedef struct {
    uint16_t       *dst;
    const uint16_t *row0;
    const uint16_t *row1;
    const uint16_t *xbase;
    const uint16_t *xweight;
    int             count;
    int             src_width;
    unsigned        yweight;
    uint16_t        color_key;
    uint16_t        has_key;
} efc_bilinear_row_args_t;

typedef void (*efc_bilinear_row_t)(const efc_bilinear_row_args_t *args);
typedef void (*efc_nn_row_t)(uint16_t *dst, const uint16_t *src,
                             const uint16_t *x_idx, int dst_w);
esp_efc_err_t efc_dec_open(const uint8_t *blob, size_t blob_len,
                           esp_efc_dec_handle_t *handle);
esp_efc_err_t efc_dec_get_info(esp_efc_dec_handle_t handle, esp_efc_dec_info_t *info);
esp_efc_err_t efc_dec_frame(esp_efc_dec_handle_t handle, int index, uint16_t *rgb565_out);
esp_efc_err_t efc_dec_frame_fixed4(esp_efc_dec_handle_t handle, int index,
                                   uint16_t *rgb565_out);
esp_efc_err_t efc_dec_frame_fixed4_direct(esp_efc_dec_handle_t handle, int index,
                                          const uint16_t **pixels, int *stride);
esp_efc_err_t efc_dec_apply_alpha(esp_efc_dec_handle_t handle, uint8_t *rgb565a8);
void efc_dec_close(esp_efc_dec_handle_t handle);

bool efc_dec_is_fixed4(esp_efc_dec_handle_t handle);
bool efc_dec_is_fixed4_stream(esp_efc_dec_handle_t handle);
void efc_dec_set_memcpy(esp_efc_dec_handle_t handle, efc_memcpy_t memcpy_fn);
void efc_dec_set_bilinear_row(esp_efc_dec_handle_t handle, efc_bilinear_row_t row_fn);
void efc_dec_set_nn_row(esp_efc_dec_handle_t handle, efc_nn_row_t row_fn);

#ifdef __cplusplus
}
#endif
