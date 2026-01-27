/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Register extractor for MP4 container
 *
 * @note  MP4 extractor support following extra control:
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_DYNAMIC_INDEXING:
 *            For big MP4 file will have large tables which can't be loaded into memory all at once
 *            Dynamic parse is added to allow users to load partial table and parse gradually
 *            When enabled, extra file seek operation is requested and may take longer time for network input
 *            If memory is enough and file size is small, recommend not enable it
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_NO_ACCURATE_SEEK:
 *            To play video (h264), if use accurate seek it will seek to last I-Frame position which may be ahead
 *            of setting seek time, and may take long time to resume until decode decoded to such frame
 *            If set this flag to `true`, it will skip this logic check and directly send from the right position
 *            Which will cause audio output instantly, video not show until next I-Frame
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_VERBOSE_LOG:
 *            Used to control whether show detail parse log
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_mp4_extractor_register(void);

/**
 * @brief  Unregister for MP4 container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_mp4_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
