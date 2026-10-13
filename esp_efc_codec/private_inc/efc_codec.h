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

#ifndef ESP_PLATFORM
int efc_encode(const uint16_t *frames, const esp_efc_enc_info_t *info, uint8_t **out);
#endif

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
void efc_dec_set_memcpy(esp_efc_dec_handle_t handle, efc_memcpy_t memcpy_fn);

#ifdef __cplusplus
}
#endif
