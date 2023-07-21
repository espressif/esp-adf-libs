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

#ifndef ESP_MUXER_ERR_H
#define ESP_MUXER_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

typedef enum {
    ESP_MUXER_ERR_OK                        = ESP_OK,
    ESP_MUXER_ERR_FAIL                      = ESP_FAIL,
    ESP_MUXER_ERR_NO_MEM                    = ESP_ERR_NO_MEM,
    ESP_MUXER_ERR_INVALID_ARG               = ESP_ERR_INVALID_ARG,
    ESP_MUXER_ERR_WRONG_STATE               = ESP_ERR_INVALID_STATE,
    ESP_MUXER_ERR_INVALID_SIZE              = ESP_ERR_INVALID_SIZE,
    ESP_MUXER_ERR_NOT_FOUND                 = ESP_ERR_NOT_FOUND,
    ESP_MUXER_ERR_NOT_SUPPORT               = ESP_ERR_NOT_SUPPORTED,
    ESP_MUXER_ERR_TIMEOUT                   = ESP_ERR_TIMEOUT,
    ESP_MUXER_ERR_INVALID_RESPONSE          = ESP_ERR_INVALID_RESPONSE,
    ESP_MUXER_ERR_INVALID_CRC               = ESP_ERR_INVALID_CRC,
    ESP_MUXER_ERR_INVALID_VERSION           = ESP_ERR_INVALID_VERSION,

    ESP_MUXER_ERR_BASE                      = 0x10000,
    ESP_MUXER_ERR_READ_DATA                 = (ESP_MUXER_ERR_BASE + 1),
    ESP_MUXER_ERR_WRITE_DATA                = (ESP_MUXER_ERR_BASE + 2),
    ESP_MUXER_ERR_BAD_DATA                  = (ESP_MUXER_ERR_BASE + 3),
    ESP_MUXER_ERR_EXCEED_LIMIT              = (ESP_MUXER_ERR_BASE + 4),
} esp_muxer_err_t;

#ifdef __cplusplus
}
#endif

#endif
