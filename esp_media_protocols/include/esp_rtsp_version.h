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
#ifndef ESP_RTSP_VERSION_H
#define ESP_RTSP_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Features:
 *     - Use RTSP version 1.0 (RFC2326)
 *     - Support RTSP Pusher, RTSP Server, RTSP Client
 *     - Support audio codecs: G711A, G711U
 *     - Support video codecs: MJPEG, H264
 *     - Support unicast (rtp over udp, rtp over tcp)
 *     - Support Public method : OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN
 *     - Support EasyDarwin and other common RTSP server
 *     - Support VLC, Potplayer, FFMPEG, etc
 *
 *  To be implemented:
 *     - To support AAC
 *     - To support digest authentication
 */
#define ESP_RTSP_VERSION "1.1.0"

#ifdef __cplusplus
}
#endif

#endif
