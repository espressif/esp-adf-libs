/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "test_common.h"
#include "wifi_fs.h"
#include "sdkconfig.h"

static const char *TAG = "TEST_COMMON";

esp_err_t ae_test_ensure_wifi_fs_ready(const char *mount_point)
{
    wifi_fs_wifi_config_t wifi_cfg = {
        .ssid = CONFIG_EXAMPLE_WIFI_SSID,
        .password = CONFIG_EXAMPLE_WIFI_PASSWORD,
    };
    esp_err_t ret = wifi_fs_connect_wifi(&wifi_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi_fs_connect_wifi failed: %s", esp_err_to_name(ret));
        return ret;
    }
    wifi_fs_mount_config_t mount_cfg = {
        .mount_point = mount_point,
        .server_ip = CONFIG_WIFI_FS_SERVER_IP,
        .server_port = CONFIG_WIFI_FS_SERVER_PORT,
        .max_files = 8,
    };
    ret = wifi_fs_mount(&mount_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi_fs_mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

esp_err_t ae_test_wifi_fs_cleanup(const char *mount_point)
{
    esp_err_t ret = wifi_fs_unmount(mount_point);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi_fs_unmount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

bool ae_test_parse_wav_header(FILE *fp, ae_audio_info_t *audio_info, uint32_t *data_size)
{
    char chunk_id[4];
    uint32_t chunk_size = 0;
    bool found_fmt = false;
    bool found_data = false;
    while (!found_fmt || !found_data) {
        if (fread(chunk_id, 1, 4, fp) != 4) {
            return false;
        }
        if (fread(&chunk_size, 1, 4, fp) != 4) {
            return false;
        }
        if (memcmp(chunk_id, "RIFF", 4) == 0) {
            char wave[4];
            if (fread(wave, 1, 4, fp) != 4 || memcmp(wave, "WAVE", 4) != 0) {
                return false;
            }
            continue;
        }
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            uint16_t audio_format = 0;
            uint16_t channels = 0;
            uint32_t sample_rate = 0;
            uint32_t byte_rate = 0;
            uint16_t block_align = 0;
            uint16_t bits_per_sample = 0;
            if (chunk_size < 16
                || fread(&audio_format, 1, 2, fp) != 2
                || fread(&channels, 1, 2, fp) != 2
                || fread(&sample_rate, 1, 4, fp) != 4
                || fread(&byte_rate, 1, 4, fp) != 4
                || fread(&block_align, 1, 2, fp) != 2
                || fread(&bits_per_sample, 1, 2, fp) != 2) {
                return false;
            }
            if (chunk_size > 16 && fseek(fp, chunk_size - 16, SEEK_CUR) != 0) {
                return false;
            }
            if (audio_format != 1) {
                return false;
            }
            audio_info->sample_rate = sample_rate;
            audio_info->channels = channels;
            audio_info->bits_per_sample = bits_per_sample;
            found_fmt = true;
            continue;
        }
        if (memcmp(chunk_id, "data", 4) == 0) {
            *data_size = chunk_size;
            found_data = true;
            continue;
        }
        if (fseek(fp, chunk_size, SEEK_CUR) != 0) {
            return false;
        }
    }
    return true;
}

bool ae_test_write_wav_header(FILE *fp, const ae_audio_info_t *audio_info, uint32_t data_size)
{
    uint32_t riff_size = data_size + 36;
    uint16_t audio_format = 1;
    uint32_t byte_rate = audio_info->sample_rate * audio_info->channels
        * (audio_info->bits_per_sample >> 3);
    uint16_t block_align = audio_info->channels * (audio_info->bits_per_sample >> 3);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        return false;
    }
    uint32_t fmt_size = 16;
    return fwrite("RIFF", 1, 4, fp) == 4
        && fwrite(&riff_size, 1, 4, fp) == 4
        && fwrite("WAVE", 1, 4, fp) == 4
        && fwrite("fmt ", 1, 4, fp) == 4
        && fwrite(&fmt_size, 1, 4, fp) == 4
        && fwrite(&audio_format, 1, 2, fp) == 2
        && fwrite(&audio_info->channels, 1, 2, fp) == 2
        && fwrite(&audio_info->sample_rate, 1, 4, fp) == 4
        && fwrite(&byte_rate, 1, 4, fp) == 4
        && fwrite(&block_align, 1, 2, fp) == 2
        && fwrite(&audio_info->bits_per_sample, 1, 2, fp) == 2
        && fwrite("data", 1, 4, fp) == 4
        && fwrite(&data_size, 1, 4, fp) == 4;
}
