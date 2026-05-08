#include <stdio.h>
#include <math.h>
#include "rgb_image_gen.h"

#define get_rgb565_le(rgb, r, g, b) {                          \
    uint8_t tmp = ((rgb)[1] << 5) | (((rgb)[0] & 0xe0) >> 3);  \
    *(b)        = ((rgb)[0] << 3) & 0xf8;                      \
    *(g)        = tmp;                                         \
    *(r)        = (rgb)[1] & 0xf8;                             \
}

#define CC_SHIFT_BIT 16
#define FIX(a)       ((int32_t)((a) * (float)(1 << CC_SHIFT_BIT) + 0.5))

static int16_t rgb_abs_diff(int32_t a, int32_t b)
{
    int32_t diff = a - b;
    return (int16_t)(diff < 0 ? -diff : diff);
}

void rgb888_image_show(uint8_t *image, int32_t width, int32_t height)
{
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width / 1; w++) {
            // if (h % 1 == 0) {
            //     printf("\033[48;2;%d;%d;%dm%c\033[0m", image[0], image[1], image[2], ' ');
            // }

            // printf("%d,%d,%d, ", image[0], image[1], image[2]);
            image += 3;
        }
        if (h % 1 == 0) {
            printf("\n");
        }
    }
    printf("\n\r");
}

void rgb888_image_gen(int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t *image)
{
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width; w++) {
            image[0] = r;
            image[1] = g;
            image[2] = b;
            image += 3;
        }
    }
}

void rgb888_color_blocks_gen(uint8_t *image, int32_t width, int32_t height)
{
    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;
            if (y < height / 2) {
                if (x < width / 2) {
                    r = 255;
                } else {
                    g = 255;
                }
            } else {
                if (x < width / 2) {
                    b = 255;
                } else {
                    r = 255;
                    g = 255;
                }
            }
            image[0] = r;
            image[1] = g;
            image[2] = b;
            image += 3;
        }
    }
}

void rgb888_terminal_print(const char *title, const uint8_t *image, int32_t width, int32_t height)
{
    printf("\n%s (%dx%d):\n", title, (int)width, (int)height);
    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            const uint8_t *pix = &image[(y * width + x) * 3];
            printf("\033[48;2;%d;%d;%dm  \033[0m", pix[0], pix[1], pix[2]);
        }
        printf("\n");
    }
}

void yuv_packet_image_gen(int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t *image)
{
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width; w++) {
            image[0] = (uint8_t)(((int32_t)r * FIX(0.2126) + (int32_t)g * FIX(0.7152) + (int32_t)b * FIX(0.0722)) >> CC_SHIFT_BIT);
            image[1] = (uint8_t)(((int32_t)(b - image[0]) * FIX(0.539)) >> CC_SHIFT_BIT) + 128;
            image[2] = (uint8_t)(((int32_t)(r - image[0]) * FIX(0.635)) >> CC_SHIFT_BIT) + 128;
            // printf("%d,%d,%d, ", image[0], image[1], image[2]);
            image += 3;
        }
    }
}

void rgb888_image_diff_pixel(uint8_t *image, uint8_t r, uint8_t g, uint8_t b, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    *r_diff = rgb_abs_diff(image[0], r);
    *g_diff = rgb_abs_diff(image[1], g);
    *b_diff = rgb_abs_diff(image[2], b);
    // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, ", image[0], image[1], image[2], r, g, b, *r_diff, *g_diff, *b_diff);
}

void rgb888_rgb565le_image_diff_pixel(uint8_t *image, uint8_t r, uint8_t g, uint8_t b, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    int32_t r0, g0, b0;
    get_rgb565_le(image, &r0, &g0, &b0);
    *r_diff = rgb_abs_diff(r0, r);
    *g_diff = rgb_abs_diff(g0, g);
    *b_diff = rgb_abs_diff(b0, b);
    // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, ", image[0], image[1], image[2], r, g, b, *r_diff, *g_diff, *b_diff);
}

void rgb888_image_diff(uint8_t *image0, uint8_t *image1, int32_t width, int32_t height, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    *r_diff = *g_diff = *b_diff = 0;
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width; w++) {
            int16_t r_cur_diff = rgb_abs_diff(image0[0], image1[0]);
            int16_t g_cur_diff = rgb_abs_diff(image0[1], image1[1]);
            int16_t b_cur_diff = rgb_abs_diff(image0[2], image1[2]);
            *r_diff = r_cur_diff > *r_diff ? r_cur_diff : *r_diff;
            *g_diff = g_cur_diff > *g_diff ? g_cur_diff : *g_diff;
            *b_diff = b_cur_diff > *b_diff ? b_cur_diff : *b_diff;
            printf("w:%ld, h:%ld, ori: %d,%d,%d, now:  %d,%d,%d, diff: %d,%d,%d, \n", w, h, image0[0], image0[1], image0[2], image1[0], image1[1], image1[2], r_cur_diff, g_cur_diff, b_cur_diff);
            image0 += 3;
            image1 += 3;
        }
    }
    // printf("diff: %d %d %d ", *r_diff, *g_diff, *b_diff);
}

void rgb888_rgb565le_image_diff(uint8_t *image0, uint8_t *image1, int32_t width, int32_t height, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    *r_diff = *g_diff = *b_diff = 0;
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width; w++) {
            int32_t r, g, b;
            get_rgb565_le(image1, &r, &g, &b);
            int16_t r_cur_diff = rgb_abs_diff(image0[0], r);
            int16_t g_cur_diff = rgb_abs_diff(image0[1], g);
            int16_t b_cur_diff = rgb_abs_diff(image0[2], b);
            *r_diff = r_cur_diff > *r_diff ? r_cur_diff : *r_diff;
            *g_diff = g_cur_diff > *g_diff ? g_cur_diff : *g_diff;
            *b_diff = b_cur_diff > *b_diff ? b_cur_diff : *b_diff;
            // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, \n", image0[0], image0[1], image0[2], r, g, b, *r_diff, *g_diff, *b_diff);
            image0 += 3;
            image1 += 2;
        }
    }
}
