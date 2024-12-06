/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "esp_private/esp_cache_private.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_clk.h"
#include "video_codec_utils.h"

const char *esp_video_codec_get_pixel_fmt_str(esp_video_codec_pixel_fmt_t fmt)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P:
            return "yuv420p";
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV422:
            return "yuv422";
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
            return "rgb565";
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE:
            return "rgb565_be";
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888:
            return "rgb888";
        case ESP_VIDEO_CODEC_PIXEL_FMT_BGR888:
            return "bgr888";
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422:
            return "uyvy422";
        case ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY:
            return "o_uyy_e_vyy";
        default:
            return "none";
    }
}

const char *esp_video_codec_get_codec_str(esp_video_codec_type_t codec)
{
    switch (codec) {
        case ESP_VIDEO_CODEC_TYPE_H264:
            return "h264";
        case ESP_VIDEO_CODEC_TYPE_MJPEG:
            return "mjpeg";
        default:
            return "none";
    }
}

void *esp_video_codec_align_alloc(uint8_t align, uint32_t size, uint32_t *actual_size)
{
    size = ALIGN_UP(size, align);
    if (actual_size) {
        *actual_size = size;
    }
    return heap_caps_aligned_calloc(align, 1, size, MALLOC_CAP_SPIRAM);
}

void esp_video_codec_free(void *ptr)
{
    heap_caps_free(ptr);
}

uint8_t video_codec_get_align_size(uint32_t align_size_type)
{
    if (align_size_type == VIDEO_CODEC_HW_CACHE_LINE_ALIGNMENT) {
        size_t mem_alignment = 0;
        esp_cache_get_alignment(MALLOC_CAP_SPIRAM, (size_t *)&mem_alignment);
        return (uint8_t) mem_alignment;
    }
    return (uint8_t)align_size_type;
}

bool video_codec_is_fmt_support(esp_video_codec_pixel_fmt_t chk_fmt, esp_video_codec_pixel_fmt_t *fmts, int num)
{
    for (int i = 0; i < num; i++) {
        if (fmts[i] == chk_fmt) {
            return true;
        }
    }
    return false;
}

uint32_t esp_video_codec_get_image_size(esp_video_codec_pixel_fmt_t fmt, esp_video_codec_resolution_t *resolution)
{
    if (resolution == NULL) {
        return 0;
    }
     switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P:
        case ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY:
            return resolution->width * resolution->height * 3 / 2;
            break;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE:
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV422:
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422:
            return resolution->width * resolution->height * 2;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888:
        case ESP_VIDEO_CODEC_PIXEL_FMT_BGR888:
            return resolution->width * resolution->height * 3;
        default:
            return 0;
    }
}

uint8_t video_codec_calc_typical_fps(uint8_t refer_fps, uint32_t refer_freq)
{
    return refer_fps * (esp_clk_cpu_freq() / 1000000) / refer_freq;
}