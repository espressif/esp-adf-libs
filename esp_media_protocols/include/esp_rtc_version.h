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
#ifndef ESP_RTC_VERSION_H
#define ESP_RTC_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Features:
 *     - Use SIP version 2.0 (RFC3261)
 *     - Support real-time communication and VoIP
 *     - Support UDP, TCP, TLS transport
 *     - Support audio codecs: G711A, G711U, OPUS
 *     - Support video codecs: MJPEG, H264
 *     - Support md5 digest authentication
 *     - Support Freeswitch and FreePBX, etc
 *
 *  Release Notes:
 *     v1.2.0:
 *     - Rename `esp_rtc_init` to `esp_rtc_service_init` to avoid conflict with IDF `esp_system`
 *     - Rename `esp_rtc_deinit` to `esp_rtc_service_deinit`
 *     - Add `__esp_rtc_receive_dtmf` callback to receive raw DTMF out band data
 *     - Extent API `esp_rtc_send_dtmf` to customized send DTMF duration and volume
 *     - Fix SIP invite response not contain SDP, if invite received multiple times
 *     - Dynamic allocate buffer to hold and parse UPNP XML data (support big XML)
 * 
 */
#define ESP_RTC_VERSION "1.2.0"

#ifdef __cplusplus
}
#endif

#endif
