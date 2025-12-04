/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"

typedef void (*sdcard_scan_cb_t)(void *user_data, char *url);

esp_err_t sdcard_scan(sdcard_scan_cb_t cb, const char *path, int depth, const char *file_extension[], int filter_num, void *user_data);

esp_err_t sdcard_list_create(void **handle);

esp_err_t sdcard_list_save(void *handle, const char *url);

esp_err_t sdcard_list_choose(void *handle, int url_id, char **url_buff);

int sdcard_list_get_url_num(void *handle);

esp_err_t sdcard_list_destroy(void *handle);
