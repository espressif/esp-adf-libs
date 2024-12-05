
/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "video_codec_test.h"
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_RGB565_R(x) (((((x) >> 11) & 0x1F) << 3) | (((x) >> 8) & 0x07))
#define GET_RGB565_G(x) (((((x) >> 5) & 0x3F) << 2) | (((x) >> 3) & 0x03))
#define GET_RGB565_B(x) ((((x) & 0x1F) << 3) | (((x) & 0x1C) >> 2))
#define SWAP_EDIAN(x)   (((x) << 8) | ((x) >> 8))
#define CLAMP(x)        ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

typedef struct {
    uint8_t y;
    uint8_t u;
    uint8_t v;
} yuv_pixel_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb888_pixel_t;

static int gen_pattern_color_bar(bool vertical, uint8_t n)
{
    uint8_t *pixel = test_res.raw_data;
    test_res.bar_count = n;
    test_res.vertical = vertical;

    switch (test_res.enc_in_fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_BGR888: {
            rgb888_pixel_t *color = (rgb888_pixel_t *)malloc(n * sizeof(rgb888_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].r = (uint16_t)(rand() & 0xFF);
                color[i].g = (uint16_t)(rand() & 0xFF);
                color[i].g = (uint16_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = test_res.res.width / n;
                uint32_t last_bar_w = test_res.res.width - bar_w * (n - 1);
                for (int y = 0; y < test_res.res.height; y++) {
                    for (int i = 0; i < n; i++) {
                        int points = (i == n - 1 ? last_bar_w : bar_w);
                        for (int x = 0; x < points; x++) {
                            *(pixel++) = color[i].b;
                            *(pixel++) = color[i].g;
                            *(pixel++) = color[i].r;
                        }
                    }
                }
            } else {
                uint32_t bar_h = test_res.res.height / n;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                for (int i = 0; i < n; i++) {
                    int points = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width;
                    for (int x = 0; x < points; x++) {
                        *(pixel++) = color[i].b;
                        *(pixel++) = color[i].g;
                        *(pixel++) = color[i].r;
                    }
                }
            }
            free(color);
        } break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888: {
            rgb888_pixel_t *color = (rgb888_pixel_t *)malloc(n * sizeof(rgb888_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].r = (uint8_t)(rand() & 0xFF);
                color[i].g = (uint8_t)(rand() & 0xFF);
                color[i].g = (uint8_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = test_res.res.width / n;
                uint32_t last_bar_w = test_res.res.width - bar_w * (n - 1);
                for (int y = 0; y < test_res.res.height; y++) {
                    for (int i = 0; i < n; i++) {
                        int points = (i == n - 1 ? last_bar_w : bar_w);
                        for (int x = 0; x < points; x++) {
                            *(pixel++) = color[i].r;
                            *(pixel++) = color[i].g;
                            *(pixel++) = color[i].b;
                        }
                    }
                }
            } else {
                uint32_t bar_h = test_res.res.height / n;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                for (int i = 0; i < n; i++) {
                    int points = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width;
                    for (int x = 0; x < points; x++) {
                        *(pixel++) = color[i].r;
                        *(pixel++) = color[i].g;
                        *(pixel++) = color[i].b;
                    }
                }
            }
            free(color);
        } break;

        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE: {
            uint16_t *color = (uint16_t *)malloc(n * sizeof(uint16_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i] = (uint16_t)(rand() & 0xFFFF);
            }
            uint16_t *pixel16 = (uint16_t *)pixel;
            if (vertical) {
                uint32_t bar_w = test_res.res.width / n;
                uint32_t last_bar_w = test_res.res.width - bar_w * (n - 1);
                for (int y = 0; y < test_res.res.height; y++) {
                    for (int i = 0; i < n; i++) {
                        int points = (i == n - 1 ? last_bar_w : bar_w);
                        for (int x = 0; x < points; x++) {
                            *(pixel16++) = color[i];
                        }
                    }
                }
            } else {
                uint32_t bar_h = test_res.res.height / n;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                for (int i = 0; i < n; i++) {
                    int points = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width;
                    for (int x = 0; x < points; x++) {
                        *(pixel16++) = color[i];
                    }
                }
            }
            free(color);
        } break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P: {
            yuv_pixel_t *color = (yuv_pixel_t *)malloc(n * sizeof(yuv_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].y = (uint8_t)(rand() & 0xFF);
                color[i].u = (uint8_t)(rand() & 0xFF);
                color[i].v = (uint8_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = (test_res.res.width / n) >> 1 << 1;
                uint32_t last_bar_w = test_res.res.width - bar_w * (n - 1);
                // Fill Y firstly
                for (int y = 0; y < test_res.res.height; y++) {
                    for (int i = 0; i < n; i++) {
                        uint32_t bytes = (i == n - 1 ? last_bar_w : bar_w);
                        memset(pixel, color[i].y, bytes);
                        pixel += bytes;
                    }
                }
                // Fill U
                for (int y = 0; y < test_res.res.height >> 1; y++) {
                    for (int i = 0; i < n; i++) {
                        uint32_t bytes = (i == n - 1 ? last_bar_w : bar_w) >> 1;
                        memset(pixel, color[i].u, bytes);
                        pixel += bytes;
                    }
                }
                // Fill V
                for (int y = 0; y < test_res.res.height >> 1; y++) {
                    for (int i = 0; i < n; i++) {
                        uint32_t bytes = (i == n - 1 ? last_bar_w : bar_w) >> 1;
                        memset(pixel, color[i].v, bytes);
                        pixel += bytes;
                    }
                }
            } else {
                uint32_t bar_h = (test_res.res.height / n) >> 1 << 1;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                // Fill Y firstly
                for (int i = 0; i < n; i++) {
                    uint32_t bytes = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width;
                    memset(pixel, color[i].y, bytes);
                    pixel += bytes;
                }
                // Fill U
                for (int i = 0; i < n; i++) {
                    uint32_t bytes = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width >> 2;
                    memset(pixel, color[i].u, bytes);
                    pixel += bytes;
                }
                // Fill V
                for (int i = 0; i < n; i++) {
                    uint32_t bytes = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width >> 2;
                    memset(pixel, color[i].v, bytes);
                    pixel += bytes;
                }
            }
            free(color);
        } break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422: {
            yuv_pixel_t *color = (yuv_pixel_t *)malloc(n * sizeof(yuv_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].y = (uint8_t)(rand() & 0xFF);
                color[i].u = (uint8_t)(rand() & 0xFF);
                color[i].v = (uint8_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = (test_res.res.width / n) >> 1 << 1;
                // Fill Y firstly
                for (int y = 0; y < test_res.res.height; y++) {
                    int bar_filled = 0;
                    int i = 0;
                    for (int x = 0; x < (test_res.res.width >> 1); x++) {
                        *pixel++ = color[i].u;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].v;
                        *pixel++ = color[i].y;
                        bar_filled += 2;
                        if (bar_filled >= bar_w) {
                            bar_filled = 0;
                            if (i < n - 1) {
                                i++;
                            }
                        }
                    }
                }
            } else {
                uint32_t bar_h = (test_res.res.height / n) >> 1 << 1;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                // Fill Y firstly
                for (int i = 0; i < n; i++) {
                    uint32_t bytes = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width * 3 / 2;
                    while (bytes > 0) {
                        *pixel++ = color[i].u;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].v;
                        *pixel++ = color[i].y;
                        bytes -= 3;
                    }
                }
            }
            free(color);
        } break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV422: {
            yuv_pixel_t *color = (yuv_pixel_t *)malloc(n * sizeof(yuv_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].y = (uint8_t)(rand() & 0xFF);
                color[i].u = (uint8_t)(rand() & 0xFF);
                color[i].v = (uint8_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = (test_res.res.width / n) >> 1 << 1;
                // Fill Y firstly
                for (int y = 0; y < test_res.res.height; y++) {
                    int bar_filled = 0;
                    int i = 0;
                    for (int x = 0; x < (test_res.res.width >> 1); x++) {
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].u;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].v;
                        bar_filled += 2;
                        if (bar_filled >= bar_w) {
                            bar_filled = 0;
                            if (i < n - 1) {
                                i++;
                            }
                        }
                    }
                }
            } else {
                uint32_t bar_h = (test_res.res.height / n) >> 1 << 1;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                // Fill Y firstly
                for (int i = 0; i < n; i++) {
                    uint32_t bytes = (i == n - 1 ? last_bar_h : bar_h) * test_res.res.width * 3 / 2;
                    while (bytes > 0) {
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].u;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].v;
                        bytes -= 3;
                    }
                }
            }
            free(color);
        } break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY: {
            yuv_pixel_t *color = (yuv_pixel_t *)malloc(n * sizeof(yuv_pixel_t));
            if (color == NULL) {
                return -1;
            }
            for (int i = 0; i < n; i++) {
                color[i].y = (uint8_t)(rand() & 0xFF);
                color[i].u = (uint8_t)(rand() & 0xFF);
                color[i].v = (uint8_t)(rand() & 0xFF);
            }
            if (vertical) {
                uint32_t bar_w = (test_res.res.width / n) >> 1 << 1;
                // Fill Y firstly
                for (int y = 0; y < (test_res.res.height >> 1); y++) {
                    int bar_filled = 0;
                    int i = 0;
                    for (int x = 0; x < (test_res.res.width >> 1); x++) {
                        *pixel++ = color[i].u;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].y;
                        bar_filled += 2;
                        if (bar_filled >= bar_w) {
                            bar_filled = 0;
                            if (i < n - 1) {
                                i++;
                            }
                        }
                    }
                    bar_filled = 0;
                    i = 0;
                    for (int x = 0; x < (test_res.res.width >> 1); x++) {
                        *pixel++ = color[i].v;
                        *pixel++ = color[i].y;
                        *pixel++ = color[i].y;
                        bar_filled += 2;
                        if (bar_filled >= bar_w) {
                            bar_filled = 0;
                            if (i < n - 1) {
                                i++;
                            }
                        }
                    }
                }
            } else {
                uint32_t bar_h = (test_res.res.height / n) >> 1 << 1;
                uint32_t last_bar_h = test_res.res.height - bar_h * (n - 1);
                // Fill Y firstly
                for (int i = 0; i < n; i++) {
                    uint32_t height = (i == n - 1 ? last_bar_h : bar_h);
                    uint32_t width = test_res.res.width >> 1;
                    for (int y = 0; y < (height >> 1); y++) {
                        for (int x = 0; x < width; x++) {
                            *pixel++ = color[i].u;
                            *pixel++ = color[i].y;
                            *pixel++ = color[i].y;
                        }
                        for (int x = 0; x < width; x++) {
                            *pixel++ = color[i].v;
                            *pixel++ = color[i].y;
                            *pixel++ = color[i].y;
                        }
                    }
                }
            }
            free(color);
        }
        default:
            break;
    }
    return 0;
}

static void yuv_to_rgb(uint8_t y, uint8_t u, uint8_t v, rgb888_pixel_t *pixel)
{
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;
    int r_temp = (298 * c + 409 * e + 128) >> 8;           // R = Y + 1.403 * (V-128)
    int g_temp = (298 * c - 100 * d - 208 * e + 128) >> 8; // G = Y - 0.344 * (U-128) - 0.714 * (V-128)
    int b_temp = (298 * c + 516 * d + 128) >> 8;           // B = Y + 1.770 * (U-128)
    pixel->r = CLAMP(r_temp);
    pixel->g = CLAMP(g_temp);
    pixel->b = CLAMP(b_temp);
}

static void get_pixel(rgb888_pixel_t *pixel, uint8_t *data, esp_video_codec_pixel_fmt_t fmt, int x, int y)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_BGR888: {
            rgb888_pixel_t *src = (rgb888_pixel_t *)data;
            src += y * test_res.res.width + x;
            pixel->b = src->r;
            pixel->g = src->g;
            pixel->r = src->b;
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888: {
            rgb888_pixel_t *src = (rgb888_pixel_t *)data;
            src += y * test_res.res.width + x;
            *pixel = *src;
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE: {
            uint16_t *src = (uint16_t *)data;
            src += y * test_res.res.width + x;
            pixel->r = GET_RGB565_R(*src);
            pixel->g = GET_RGB565_G(*src);
            pixel->b = GET_RGB565_B(*src);
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE: {
            uint16_t *src = (uint16_t *)data;
            src += y * test_res.res.width + x;
            *src = SWAP_EDIAN(*src);
            pixel->r = GET_RGB565_R(*src);
            pixel->g = GET_RGB565_G(*src);
            pixel->b = GET_RGB565_B(*src);
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422: {
            x = (x >> 1 << 1);
            uint8_t *yuyv = data + y * test_res.res.width * 2 + x * 2;
            yuv_to_rgb(yuyv[3], yuyv[0], yuyv[2], pixel);
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV422: {
            x = (x >> 1 << 1);
            uint8_t *yuyv = data + y * test_res.res.width * 2 + x * 2;
            yuv_to_rgb(yuyv[0], yuyv[1], yuyv[3], pixel);
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P: {
            uint8_t *py = data + y * test_res.res.width + x;
            y >>= 1;
            x >>= 1;
            uint8_t *pu = data + test_res.res.height * test_res.res.width + y * test_res.res.width / 2 + x;
            uint8_t *pv = data + test_res.res.height * test_res.res.width * 5 / 4 + y * test_res.res.width / 2 + x;
            yuv_to_rgb(py[0], pu[0], pv[0], pixel);
            break;
        }
        case ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY: {
            y >>= 1;
            x >>= 1;
            uint8_t *uyy = data + y * test_res.res.width * 3 + x * 3;
            uint8_t *vyy = uyy + test_res.res.width * 3 / 2;
            yuv_to_rgb(vyy[2], uyy[0], vyy[0], pixel);
            break;
        }
        default:
            break;
    }
}

static void draw_decode_result(void)
{
    if (test_res.bar_count == 0) {
        return;
    }
    printf("\n");
    int y = 0;
    rgb888_pixel_t block_start = {};
    rgb888_pixel_t block_end = {};
    int n = test_res.bar_count;
    uint32_t bar_w = (test_res.res.width / n) >> 1 << 1;
    uint32_t last_bar_w = test_res.res.width - bar_w * (n - 1);
    uint32_t bar_h = test_res.res.height / n;
    for (int col = 0; col < n; col++) {
        int x = 0;
        for (int row = 0; row < n; row++) {
            get_pixel(&block_start, test_res.raw_data, test_res.enc_in_fmt, x, y);
            x += (row == n - 1 ? last_bar_w : bar_w);
            get_pixel(&block_end, test_res.raw_data, test_res.enc_in_fmt, x - 1, y);
            printf("\033[48;2;%d;%d;%dm%c\033[0m", block_start.r, block_start.g, block_start.b, ' ');
            printf("\033[48;2;%d;%d;%dm%c\033[0m", block_end.r, block_end.g, block_end.b, ' ');
        }
        printf("  |  ");
        for (int row = 0; row < n; row++) {
            get_pixel(&block_start, test_res.decoded_data, test_res.dec_out_fmt, x, y);
            x += (row == n - 1 ? last_bar_w : bar_w);
            get_pixel(&block_end, test_res.decoded_data, test_res.dec_out_fmt, x - 1, y);
            printf("\033[48;2;%d;%d;%dm%c\033[0m", block_start.r, block_start.g, block_start.b, ' ');
            printf("\033[48;2;%d;%d;%dm%c\033[0m", block_end.r, block_end.g, block_end.b, ' ');
        }
        y += bar_h;
        printf("\n");
    }
}
#ifdef __cplusplus
}
#endif
