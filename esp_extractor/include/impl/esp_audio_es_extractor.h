/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Register extractor for audio ES(elementary stream) container (MP3, AAC, AMR, FLAC etc)
 *
 * @note  Audio ES extractor support following extra control:
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_DEEP_INDEXING:
 *           In default, audio ES will use average bitrate to estimate duration and PTS
 *           It is fast but not accurate especially for VBR file
 *           Enable deep indexing it will traverse the whole file and build index table (500ms one item)
 *           After traverse done, memorized the accurate duration
 *           The index table then can be used for accurate seek afterwards
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_audio_es_extractor_register(void);

/**
 * @brief  Unregister for audio ES(elementary stream) container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_audio_es_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
