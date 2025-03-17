// Copyright 2024 Espressif Systems (Shanghai) CO., LTD.
// All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "esp_cpu.h"
#include "sdkconfig.h"
#include "esp_heap_caps.h"

#include "color_convert.h"
#include "image_io.h"
#include "img_rotate.h"
#include "img_scale.h"
#include "img_crop.h"

TEST_CASE("rotate", "[imgfx]")
{
    test_esp_imgfx_rotate_open();
    test_esp_imgfx_rotate_set_cfg();
    test_esp_imgfx_rotate_process();
    test_esp_imgfx_rotate_get_resolution();
}

TEST_CASE("rotate_inv", "[imgfx]")
{
    test_esp_imgfx_rotate_open_invalid_pixel_format();
    test_esp_imgfx_rotate_open_invalid_resolution();
    test_esp_imgfx_rotate_process_invalid_handle();
    test_esp_imgfx_rotate_process_invalid_image_length();
    test_esp_imgfx_rotate_get_resolution_invalid_handle();
}

TEST_CASE("scale", "[imgfx]")
{
    test_esp_imgfx_scale_open();
    test_esp_imgfx_scale_get_cfg();
    test_esp_imgfx_scale_set_cfg();
    test_esp_imgfx_scale_process();
    test_esp_imgfx_scale_close();
}

TEST_CASE("scale_inv", "[imgfx]")
{
    test_esp_imgfx_scale_open_with_invalid_config();
    test_esp_imgfx_scale_process_with_invalid_handle();
    test_esp_imgfx_scale_process_with_invalid_input_image();
    test_esp_imgfx_scale_process_with_invalid_output_image();
    test_esp_imgfx_scale_close_with_invalid_handle();
    test_esp_imgfx_scale_get_cfg_with_invalid_handle();
    test_esp_imgfx_scale_set_cfg_with_invalid_handle();
    test_esp_imgfx_scale_set_cfg_with_invalid_config();
}

TEST_CASE("crop", "[imgfx]")
{
    test_esp_imgfx_crop_open();
    test_esp_imgfx_crop_get_cfg();
    test_esp_imgfx_crop_set_cfg();
    test_esp_imgfx_crop_process();
    test_esp_imgfx_crop_close();
}

TEST_CASE("crop_inv", "[imgfx]")
{
    test_esp_imgfx_crop_open_invalid_cfg();
    test_esp_imgfx_crop_open_not_supported_pixel_fmt();
    test_esp_imgfx_crop_get_cfg_invalid_handle();
    test_esp_imgfx_crop_set_cfg_invalid_handle();
    test_esp_imgfx_crop_process_invalid_handle();
    test_esp_imgfx_crop_process_insufficient_input_data();
    test_esp_imgfx_crop_process_insufficient_output_buffer();
    test_esp_imgfx_crop_close_invalid_handle();
}

TEST_CASE("color convert", "[imgfx]")
{
    test_esp_imgfx_color_convert_open();
    test_esp_imgfx_color_convert_get_cfg();
    test_esp_imgfx_color_convert_set_cfg();
    test_esp_imgfx_color_convert_process();
    test_esp_imgfx_color_convert_close();
}

TEST_CASE("color convert _inv", "[imgfx]")
{
    test_esp_imgfx_color_convert_open_invalid_cfg();
    test_esp_imgfx_color_convert_open_unsupported_format();
    test_esp_imgfx_color_convert_get_cfg_invalid_handle();
    test_esp_imgfx_color_convert_set_cfg_invalid_parameter();
    test_esp_imgfx_color_convert_process_insufficient_data();
    test_esp_imgfx_color_convert_process_insufficient_buffer();
    test_esp_imgfx_color_convert_process_invalid_parameter();
}
