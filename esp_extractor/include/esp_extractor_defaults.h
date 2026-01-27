/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "esp_wav_extractor.h"
#include "esp_mp4_extractor.h"
#include "esp_audio_es_extractor.h"
#include "esp_ogg_extractor.h"
#include "esp_ts_extractor.h"
#include "esp_avi_extractor.h"
#include "esp_caf_extractor.h"
#include "esp_flv_extractor.h"
#include "esp_raw_extractor.h"

/**
 * @brief  Register all supported extractor type
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      On success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Not enough memory
 */
esp_extractor_err_t esp_extractor_register_default(void);

/**
 * @brief  Unregister all registered default extractor
 *
 * @note  Do not unregister when extractor is running to avoid resource leakage
 *
 */
void esp_extractor_unregister_default(void);

/**
 * @brief  Unregister all registered extractor including customized ones
 *
 * @note  Do not unregister when extractor is running to avoid resource leakage
 *
 */
void esp_extractor_unregister_all(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
