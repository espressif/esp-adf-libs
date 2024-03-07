/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2023 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
