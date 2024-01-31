/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in
 * which case, it is free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef OGG_MUXER_H
#define OGG_MUXER_H

#include "esp_muxer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OGG muxer configuration
 */
typedef struct {
    esp_muxer_config_t base_config;     /*!< Base configuration */
    uint32_t           page_cache_size; /*!< Cache data until data size exceed cache size (optional)
                                             OGG files contain multiple of pages, small input frames can be gathered into page
                                             Page is flushed into storage when page cache is full
                                             Through this method to decrease the page head overloading */
} ogg_muxer_config_t;

/**
 * @brief Register muxer for OGG container
 *
 * @return
 *      - ESP_MEDIA_ERR_OK: Register ok
 *      - ESP_MEDIA_ERR_INVALID_ARG: Invalid input argument
 *      - ESP_MEDIA_ERR_NO_MEM: Memory not enough
 */
esp_muxer_err_t ogg_muxer_register(void);

#ifdef __cplusplus
}
#endif

#endif
