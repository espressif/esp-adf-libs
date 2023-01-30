// Copyright 2023-2026 Espressif Systems (Shanghai) CO., LTD
// All rights reserved.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_H264_VERSION "0.1.0"

/**
 * @brief  Get H264 version string
 *
 * @return 
 *       - ESP_H264_VERSION
 */
const char *esp_h264_get_version();

#ifdef __cplusplus
}
#endif