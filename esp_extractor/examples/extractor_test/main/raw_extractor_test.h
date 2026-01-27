/* Raw extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "esp_extractor_defaults.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Do raw extractor test
 *
 * @note  This code will demonstrate how to extractor OPUS frame use aligned extractor API
 *        A read callback `opus_read` to simulate to read one OPUS frame with variable length
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to run raw extractor
 */
int raw_extractor_test(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
