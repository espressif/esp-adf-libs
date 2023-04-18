/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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
#ifndef ESP_RTMP_VERSION_H
#define ESP_RTMP_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_RTMP_VERION "1.3.0"

/**
 *  Features:
 *     - Use RTMP version 3
 *     - Support RTMP Publisher, RTMP Server, Pull RTMP as FLV
 *     - Support audio codecs: MP3, AAC, PCM
 *     - Support video codecs: MJPEG, H264
 *     - Support AMF0 parse
 *     - Support HTTP
 *     - Support basic authorization
 *     - Support nginx-RTMP server
 *
 *  To be implemented:
 *     - To support AMF3 parse
 * 
 *  Release Notes:
 *     v1.1.0:
 *     - Add G711 alaw, ulaw support
 *     - Use select to poll all client request to avoid create receive threads
 *     - Add cache logic for client send buffer to avoid client block each other
 *
 *     v1.2.0:
 *     - Add RTMPS support for both server and client
 *     - Support url parser to get ip address, set default port if not provided
 *     - Fix H264 SPS-PPS parameter parse wrong
 *     - Fix racing condition when receive instance is freed after use
 * 
 *     v1.3.0:
 *     - Add customized command exchange between pusher and puller when using `esp_rtmp_server`
 *     - RTMP source add support for callback media data and metadata directly without mux to FLV
 *     - Support send command and data using priority
 *     - Add drop data logic when server detects client read data too slow instead of close client directly
 *     - Enhance receive data logic to avoid client with little input data being processed too later
 *
 *  Notes:
 *     - MJPEG video codec is supported in private format (use FLV video codecid 1)
 *       Users can refer to README.md under example folder to check the technical details
 */

/**
 * @brief         Get RTMP version string
 */
const char* esp_rtmp_get_version();

#ifdef __cplusplus
}
#endif

#endif
