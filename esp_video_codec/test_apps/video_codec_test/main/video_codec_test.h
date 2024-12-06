
/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_video_codec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_video_codec_pixel_fmt_t  enc_in_fmt;
    esp_video_codec_pixel_fmt_t  dec_out_fmt;
    esp_video_codec_resolution_t res;
    uint8_t                     *raw_data;
    uint32_t                     raw_size;
    uint8_t                     *encoded_data;
    uint32_t                     encoded_size;
    uint8_t                     *decoded_data;
    uint32_t                     decoded_size;
    bool                         vertical;
    uint8_t                      bar_count;
} video_codec_test_res_t;

extern video_codec_test_res_t test_res;

#ifdef __cplusplus
}
#endif
