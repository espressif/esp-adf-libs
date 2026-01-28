/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "caf_muxer.h"
#include "flv_muxer.h"
#include "mp4_muxer.h"
#include "ts_muxer.h"
#include "wav_muxer.h"
#include "ogg_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Register default muxer
 *
 * @note  User can use `menuconfig` to select the supported muxer to save lin binary size
 *
 * @return
 *      - ESP_MUXER_ERR_OK      On success
 *      - ESP_MUXER_ERR_NO_MEM  Memory not enough
 */
esp_muxer_err_t esp_muxer_register_default(void);

/**
 * @brief  Unregister default muxer
 */
void esp_muxer_unregister_default(void);

#ifdef __cplusplus
}
#endif
