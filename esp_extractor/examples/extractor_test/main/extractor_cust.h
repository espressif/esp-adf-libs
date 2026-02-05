/* Customized extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "esp_extractor_defaults.h"

#ifndef __linux__
#define MY_EXTRACTOR_TEST_URL  "/sdcard/test.myext"
#else
#define MY_EXTRACTOR_TEST_URL  "test.myext"
#endif  /* __linux__ */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef bool (*frame_verify_func)(esp_extractor_frame_info_t *frame);

esp_extractor_err_t my_extractor_register(void);

int my_extractor_gen_test(const char *url);

bool my_extractor_frame_verify(esp_extractor_frame_info_t *frame);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
