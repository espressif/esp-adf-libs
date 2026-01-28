/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef ESP_MUXER_VERSION_H
#define ESP_MUXER_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_MUXER_VERSION "1.1.1"

/**
 *  Features:
 *     - Muxing both audio and video data
 *     - Support for multiple audio and video tracks if supported in containers
 *     - Compatible with common video and audio formats
 *     - Direct file saving
 *     - File slice setting and customized file storage name
 *     - Support for streaming callback data
 *     - Customizable write function to use customized storage media
 *
 *  Release Notes:
 *     v1.0.0:
 *     - Add MP4, TS, FLV, WAV container support
 *
 *     v1.1.0:
 *     - Add CAF, OGG container support
 *     - Add ALAC and OPUS new audio codec support
 *     - Support write to storage use aligned RAM cache to increase write speed through `ram_cache_size`
 *
 *     v1.1.1:
 *     - Add key frame parser for H264 (in case that `key_frame` flag not set correctly)
 *       User can disable it by set `no_key_frame_verify`
 *     - Always write file slice on key frame boundary (for H264) to avoid lost data during decoding
 *
 */
const char *esp_muxer_get_version(void);

#ifdef __cplusplus
}
#endif

#endif
