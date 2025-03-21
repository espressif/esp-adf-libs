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
    *r_diff = abs((int16_t)image[0] - (int16_t)r);
    *g_diff = abs((int16_t)image[1] - (int16_t)g);
    *b_diff = abs((int16_t)image[2] - (int16_t)b);
    // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, ", image[0], image[1], image[2], r, g, b, *r_diff, *g_diff, *b_diff);
}

void rgb888_rgb565le_image_diff_pixel(uint8_t *image, uint8_t r, uint8_t g, uint8_t b, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    int32_t r0, g0, b0;
    get_rgb565_le(image, &r0, &g0, &b0);
    *r_diff = abs((int16_t)r0 - (int16_t)r);
    *g_diff = abs((int16_t)g0 - (int16_t)g);
    *b_diff = abs((int16_t)b0 - (int16_t)b);
    // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, ", image[0], image[1], image[2], r, g, b, *r_diff, *g_diff, *b_diff);
}

void rgb888_image_diff(uint8_t *image0, uint8_t *image1, int32_t width, int32_t height, int16_t *r_diff, int16_t *g_diff, int16_t *b_diff)
{
    *r_diff = *g_diff = *b_diff = 0;
    for (int32_t h = 0; h < height; h++) {
        for (int32_t w = 0; w < width; w++) {
            *r_diff = abs((int16_t)image0[0] - (int16_t)image1[0]) > *r_diff ? abs((int16_t)image0[0] - (int16_t)image1[0]) : *r_diff;
            *g_diff = abs((int16_t)image0[1] - (int16_t)image1[1]) > *g_diff ? abs((int16_t)image0[1] - (int16_t)image1[1]) : *g_diff;
            *b_diff = abs((int16_t)image0[2] - (int16_t)image1[2]) > *b_diff ? abs((int16_t)image0[2] - (int16_t)image1[2]) : *b_diff;
            printf("w:%ld, h:%ld, ori: %d,%d,%d, now:  %d,%d,%d, diff: %d,%d,%d, \n", w, h, image0[0], image0[1], image0[2], image1[0], image1[1], image1[2], abs((int16_t)image0[0] - (int16_t)image1[0]), abs((int16_t)image0[1] - (int16_t)image1[1]), abs((int16_t)image0[2] - (int16_t)image1[2]));
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
            *r_diff = abs((int16_t)image0[0] - r) > *r_diff ? abs((int16_t)image0[0] - r) : *r_diff;
            *g_diff = abs((int16_t)image0[1] - g) > *g_diff ? abs((int16_t)image0[1] - g) : *g_diff;
            *b_diff = abs((int16_t)image0[2] - b) > *b_diff ? abs((int16_t)image0[2] - b) : *b_diff;
            // printf("%d,%d,%d,%d,%d,%d,%d,%d,%d, \n", image0[0], image0[1], image0[2], r, g, b, *r_diff, *g_diff, *b_diff);
            image0 += 3;
            image1 += 2;
        }
    }
}
