/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#pragma once

#include <stdint.h>

void rgb888_image_show(uint8_t *image, int32_t width, int32_t height);

void rgb888_image_gen(int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t *image);

void yuv_packet_image_gen(int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t *image);

void rgb888_image_diff_pixel(uint8_t *image, uint8_t r, uint8_t g, uint8_t b, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff);
void rgb888_rgb565le_image_diff_pixel(uint8_t *image, uint8_t r, uint8_t g, uint8_t b, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff);

void rgb888_image_diff(uint8_t *image0, uint8_t *image1, int32_t width, int32_t height, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff);
void rgb888_rgb565le_image_diff(uint8_t *image0, uint8_t *image1, int32_t width, int32_t height, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff);
