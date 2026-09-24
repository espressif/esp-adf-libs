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
 *     - Support SIP MESSAGE and re-INVITE
 *     - Support RFC2435 JPEG over RTP
 *     - Support SDES-SRTP
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
 *     v1.3.0:
 *     - Added AAC support for RTSP
 *     - Added SPS-PPS parsing for H264
 *     - Added hangup reason for SIP
 *     - Added P2P SIP support
 *     - Added customized invite header support
 *     - Added keep alive support for RTSP client
 *     - Added call answer flow for SIP
 *     - Added OPUS support
 *     - Added fixed port support for SIP (UDP only)
 *     - Added lock and protect code to avoid timing issues
 *
 *     - Fixed build issues
 *     - Fixed RTSP server setup not responded
 *     - Fixed PTS calc not correct
 *     - Fixed RTP marker conflict
 *     - Fixed bad option free cause double free
 *
 *     v1.4.0:
 *     - Added domain for a single server divided into multiple domains
 *     - Added TCP/TLS keep-alive mechanism
 *     - Added configurable read/write and connect timeouts
 *     - Added response handling for NOTIFY messages
 *
 *     - Removed authentication headers from unwanted methods
 *     - Fixed missing expire handling in Contact and failure to reset after REGISTER OK
 *     - Removed Expires header from unwanted methods
 *     - Removed retry send logic for TLS/TCP
 *     - Fixed incorrect SIP message parsing when peer flushes input over TCP/TLS
 *     - Improved safety of `strcasecmp` to prevent NULL pointer crashes
 *     - Refined TLS error log output
 *     - Fixed incorrect ACK request sending
 *     - Fixed incorrect handling of `200 OK` responses after a CANCEL
 *
 *     v1.5.0:
 *     - Added support for SIP MESSAGE method
 *     - Added support for re-INVITE
 *     - Added support for custom SDP RTP payload type
 *     - Added option to suspend refresh register during a call
 *     - Added support for configuring private header at init for the first message
 *     - Added register refresh keepalive event
 *     - Added support for RFC2435 JPEG over RTP assemble and parse
 *     - Added support for configurable RTP frame size
 *     - Added support for reading raw SIP headers for custom parsing
 *
 *     - Fixed refresh register reusing previous `call_id`
 *     - Fixed SDP negotiation failure when only video is present
 *     - Fixed crash caused by race condition when stopping RTP
 *     - Fixed `use_public_addr` always using the default route
 *     - Fixed the abnormal organization of request-URI, branch, and from tag in special cases
 *
 *     v1.6.0:
 *     - Added SDES-SRTP support for SIP
 *
 *     - Fixed RTP timestamp for OPUS
 * 
 *     v1.6.1:
 *     - Added task scheduler config support for SIP
 *
 *     - Fixed jpeg make tables order zigzag instead of raster
 */
#define ESP_RTC_VERSION "1.6.1"

#ifdef __cplusplus
}
#endif

#endif
