/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_efc_codec.h"

#include "efc_codec.h"
#include "esp_log.h"

#ifdef ESP_EFC_USE_SIMD
#include "efc_simd.h"
#endif

static const char *TAG = "EFC_CODEC";

#ifndef ESP_PLATFORM
int esp_efc_encode(const uint16_t *frames, const esp_efc_enc_info_t *info, uint8_t **out)
{
    int ret = efc_encode(frames, info, out);
    if (ret < 0) {
        ESP_LOGE(TAG, "esp_efc_encode failed: %d", ret);
    }
    return ret;
}
#endif

esp_efc_err_t esp_efc_dec_open(const uint8_t *blob, size_t blob_len,
                               esp_efc_dec_handle_t *handle)
{
    esp_efc_err_t ret = efc_dec_open(blob, blob_len, handle);
#ifdef ESP_EFC_DECODE_4X4_ONLY
    if (ret == ESP_EFC_OK && !efc_dec_is_fixed4(*handle)) {
        efc_dec_close(*handle);
        *handle = NULL;
        ret = ESP_EFC_ERR_INVALID_DATA;
    }
#endif
#ifdef ESP_EFC_USE_SIMD
    if (ret == ESP_EFC_OK) {
        efc_dec_set_memcpy(*handle, esp_efc_memcpy_simd);
    }
#endif
    if (ret != ESP_EFC_OK) {
        ESP_LOGE(TAG, "esp_efc_dec_open failed: %d", (int)ret);
    }
    return ret;
}

esp_efc_err_t esp_efc_dec_get_info(esp_efc_dec_handle_t handle, esp_efc_dec_info_t *info)
{
    esp_efc_err_t ret = efc_dec_get_info(handle, info);
    if (ret != ESP_EFC_OK) {
        ESP_LOGE(TAG, "esp_efc_dec_get_info failed: %d", (int)ret);
    }
    return ret;
}

esp_efc_err_t esp_efc_dec_frame(esp_efc_dec_handle_t handle, int index, uint16_t *rgb565_out)
{
#ifdef ESP_EFC_DECODE_4X4_ONLY
    esp_efc_err_t ret = efc_dec_frame_fixed4(handle, index, rgb565_out);
#else
    esp_efc_err_t ret = efc_dec_frame(handle, index, rgb565_out);
#endif
    if (ret != ESP_EFC_OK) {
        ESP_LOGE(TAG, "esp_efc_dec_frame failed: %d", (int)ret);
    }
    return ret;
}

esp_efc_err_t esp_efc_dec_frame_with_alpha(esp_efc_dec_handle_t handle, int index,
                                           uint8_t *rgb565a8)
{
    esp_efc_err_t ret;
#ifdef ESP_EFC_DECODE_4X4_ONLY
    ret = efc_dec_frame_fixed4(handle, index, (uint16_t *)rgb565a8);
#else
    ret = efc_dec_frame(handle, index, (uint16_t *)rgb565a8);
#endif
    if (ret == ESP_EFC_OK) {
        ret = efc_dec_apply_alpha(handle, rgb565a8);
    }
    if (ret != ESP_EFC_OK) {
        ESP_LOGE(TAG, "esp_efc_dec_frame_with_alpha failed: %d", (int)ret);
    }
    return ret;
}

void esp_efc_dec_close(esp_efc_dec_handle_t handle)
{
    efc_dec_close(handle);
}
