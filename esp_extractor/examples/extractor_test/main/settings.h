/* Raw extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define SDCARD_CLK  15
#define SDCARD_CMD  7
#define SDCARD_D0   4
#define SDCARD_D1   (-1)
#define SDCARD_D2   (-1)
#define SDCARD_D3   (-1)
#elif CONFIG_IDF_TARGET_ESP32P4
#define SDCARD_CLK  43
#define SDCARD_CMD  44
#define SDCARD_D0   39
#define SDCARD_D1   40
#define SDCARD_D2   41
#define SDCARD_D3   42
#else
#define SDCARD_CLK  (-1)
#define SDCARD_CMD  (-1)
#define SDCARD_D0   (-1)
#define SDCARD_D1   (-1)
#define SDCARD_D2   (-1)
#define SDCARD_D3   (-1)
#endif  /* CONFIG_IDF_TARGET_ESP32S3 */
