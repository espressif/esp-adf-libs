#include "unity.h"
#include "esp_cpu.h"
#include "esp_timer.h"

void img_rotate_process();

void test_esp_imgfx_rotate_open();
void test_esp_imgfx_rotate_open_invalid_pixel_format();
void test_esp_imgfx_rotate_open_invalid_resolution();

void test_esp_imgfx_rotate_process();
void test_esp_imgfx_rotate_process_invalid_handle();
void test_esp_imgfx_rotate_process_invalid_image_length();

void test_esp_imgfx_rotate_get_resolution();
void test_esp_imgfx_rotate_get_resolution_invalid_handle();

void test_esp_imgfx_rotate_set_cfg();
