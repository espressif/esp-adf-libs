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
 * @brief  Register extractor for AVI container
 *
 * @note  AVI extractor support following extra control:
 *        - ESP_EXTRACTOR_CTRL_TYPE_SET_NO_INDEXING:
 *           Build index table will consume memory especially when file is big
 *           After this setting enabled, index table will not build
 *           Pay attention only enable it when no need seek scenario
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      Register success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  Memory not enough
 */
esp_extractor_err_t esp_avi_extractor_register(void);

/**
 * @brief  Unregister for AVI container
 *
 * @note  Do not unregister while extractor still under use
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not founded
 */
esp_extractor_err_t esp_avi_extractor_unregister(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
