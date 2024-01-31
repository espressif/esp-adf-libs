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

#ifndef WAV_MUXER_H
#define WAV_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WAV muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config; /*!< Base configuration */
} wav_muxer_config_t;

/**
 * @brief Register muxer for WAV container
 *
 * @return
 *      - ESP_MUXER_ERR_OK: Register ok
 *      - ESP_MUXER_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MUXER_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t wav_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
