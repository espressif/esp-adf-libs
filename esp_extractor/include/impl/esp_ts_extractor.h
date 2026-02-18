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
 * @brief  Register extractor for MPEG-TS container
 *
 * @note  TS extractor support following extra control:
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_FRAME_ACROSS_PES:
 *            Most of file, frame start is at PES boundary, in special case frame may across multiple PES
 *            Enable this flag to allow parse for such special file
 *            In most of cases can disable it for high parse performance
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_ts_extractor_register(void);

/**
 * @brief  Allow audio or video frame across multiple PES
 *
 * @note  Default not allow frame over multiple PES to decrease memory usage
 *        When enable if check frame is not whole in one PES,
 *        Need at least keep 2 frames to combine into one whole frame
 *
 */
void esp_ts_extractor_allow_frame_across_pes(bool enable);

/**
 * @brief  Unregister for TS container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_ts_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
