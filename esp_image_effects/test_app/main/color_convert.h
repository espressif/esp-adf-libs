#include "esp_imgfx_color_convert.h"
#include "esp_imgfx_types.h"
#include "rgb_image_gen.h"
#include "unity.h"
#include "esp_cpu.h"
#include "esp_timer.h"

void yuv444packet_rgb888_pure(int16_t *r_max, int16_t *g_max, int16_t *b_max);
void yuv444packet_rgb565_le_pure(int16_t *r_max, int16_t *g_max, int16_t *b_max);
void img_cc_process();

void test_esp_imgfx_color_convert_open();
void test_esp_imgfx_color_convert_open_invalid_cfg();
void test_esp_imgfx_color_convert_open_unsupported_format();
void test_esp_imgfx_color_convert_get_cfg();
void test_esp_imgfx_color_convert_get_cfg_invalid_handle();
void test_esp_imgfx_color_convert_set_cfg();
void test_esp_imgfx_color_convert_set_cfg_invalid_parameter();
void test_esp_imgfx_color_convert_process();
void test_esp_imgfx_color_convert_process_insufficient_data();
void test_esp_imgfx_color_convert_process_insufficient_buffer();
void test_esp_imgfx_color_convert_process_invalid_parameter();
void test_esp_imgfx_color_convert_close();