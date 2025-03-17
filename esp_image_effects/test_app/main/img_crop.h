#include "unity.h"
#include "esp_cpu.h"
#include "esp_timer.h"

void img_crop_process();
void test_esp_imgfx_crop_open(void);
void test_esp_imgfx_crop_open_invalid_cfg();
void test_esp_imgfx_crop_open_not_supported_pixel_fmt();
void test_esp_imgfx_crop_get_cfg_invalid_handle();
void test_esp_imgfx_crop_get_cfg();
void test_esp_imgfx_crop_set_cfg_invalid_handle();
void test_esp_imgfx_crop_set_cfg();
void test_esp_imgfx_crop_process_invalid_handle();
void test_esp_imgfx_crop_process_insufficient_input_data();
void test_esp_imgfx_crop_process_insufficient_output_buffer();
void test_esp_imgfx_crop_process();
void test_esp_imgfx_crop_close_invalid_handle();
void test_esp_imgfx_crop_close();