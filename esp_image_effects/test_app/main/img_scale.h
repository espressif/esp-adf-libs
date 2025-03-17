#include "unity.h"
#include "esp_cpu.h"
#include "esp_timer.h"

void img_scale_process();
void test_esp_imgfx_scale_open();
void test_esp_imgfx_scale_get_cfg();
void test_esp_imgfx_scale_set_cfg();
void test_esp_imgfx_scale_process();
void test_esp_imgfx_scale_close();

void test_esp_imgfx_scale_open_with_invalid_config();
void test_esp_imgfx_scale_process_with_invalid_handle();
void test_esp_imgfx_scale_process_with_invalid_input_image();
void test_esp_imgfx_scale_process_with_invalid_output_image();
void test_esp_imgfx_scale_close_with_invalid_handle();
void test_esp_imgfx_scale_get_cfg_with_invalid_handle();
void test_esp_imgfx_scale_set_cfg_with_invalid_handle();
void test_esp_imgfx_scale_set_cfg_with_invalid_config();
