/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <stdio.h>
#include "esp_partition.h"
#include "esp_midi_flash_loader.h"
#include "esp_midi_sample_cache.h"
#include "esp_midi_sf_loader.h"
#include "midi_sound_files.h"

static const char *TAG = "MIDI.FLASH.LOADER";

typedef struct {
    esp_partition_t             *partition;
    void                        *partition_buffer;
    esp_partition_mmap_handle_t  partition_handle;
} esp_midi_flash_loader_hd_t;

esp_midi_err_t esp_midi_flash_loader_new(esp_midi_lib_loader_cfg_t cfg, esp_midi_lib_loader_handle_t *lib_handle)
{
    ESP_MIDI_RETURN_ON_FALSE(lib_handle, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "The sound font loader handle is NULL");
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    // create one handle
    esp_midi_flash_loader_hd_t *hd = calloc(1, sizeof(esp_midi_flash_loader_hd_t));
    ESP_MIDI_RETURN_ON_FALSE(hd, ESP_MIDI_ERR_MEM_LACK, TAG, "The sound font loader handle(size %d) allocation failed", sizeof(esp_midi_flash_loader_hd_t));

    // find soundfile global_partition
    hd->partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "soundfile");
    if (!hd->partition) {
        ESP_LOGE(TAG, "Can not found tone[%s] global_partition", "soundfile");
        goto _flash_loader_exit;
    }

    ret = esp_partition_mmap(hd->partition, 0, hd->partition->size, ESP_PARTITION_MMAP_DATA, (const void **)&hd->partition_buffer, &hd->partition_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_partition_mmap failed");
        goto _flash_loader_exit;
    }

    // update paremeter
    *lib_handle = hd;
    return ret;
_flash_loader_exit:
    esp_midi_flash_loader_delete(hd);
    return ret;
}

esp_midi_err_t esp_midi_flash_loader_noteon(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_flash_loader_hd_t *hd = (esp_midi_flash_loader_hd_t *)lib_handle;

    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    int32_t index = 0, note_val = 0, vel_min = 0, vel_max = 0;
    int wav_number = sizeof(wav_index_table) / sizeof(WavIndex);
    for (int i = 0; i < wav_number; i++) {
        index = wav_index_table[i].index;
        note_val = wav_index_table[i].note;
        vel_min = wav_index_table[i].minval;
        vel_max = wav_index_table[i].maxval;
        if (vel_max == 127) {
            vel_max = 128;
        }

        if (note_val == sample_desc->note
            && (sample_desc->velocity >= vel_min && sample_desc->velocity < vel_max)) {
            uint32_t buf_len = wav_index_table[i].size;
            sample_desc->data.buffer = (uint8_t *)hd->partition_buffer + wav_index_table[i].offset;
            sample_desc->data.len = buf_len;
            sample_desc->loopstart = 0;
            sample_desc->loopend = buf_len >> 1;
            sample_desc->samplerate = 44100;
            sample_desc->bits = 16;
            sample_desc->sampletype = ESP_MIDI_SF_SAMPLE_TYPE_MONO;
            sample_desc->gain = 1.0;
            return ESP_MIDI_ERR_OK;
        }
    }

    return ESP_MIDI_ERR_FAIL;
}

esp_midi_err_t esp_midi_flash_loader_delete(esp_midi_lib_loader_handle_t lib_handle)
{
    esp_midi_flash_loader_hd_t *hd = (esp_midi_flash_loader_hd_t *)lib_handle;
    esp_partition_munmap(hd->partition_handle);
    free(hd);
    return ESP_MIDI_ERR_OK;
}
