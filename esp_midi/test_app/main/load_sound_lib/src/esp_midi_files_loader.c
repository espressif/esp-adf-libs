/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <stdio.h>
#include "esp_midi_files_loader.h"
#include "esp_midi_sample_cache.h"
#include "esp_midi_sf_loader.h"

static const char *TAG = "MIDI.FILES.LOADER";

typedef struct {
    uint32_t                  buf_len;
    uint16_t                  num_nodes;
    esp_midi_sample_cache_t  *sample_cache;
} esp_midi_files_loader_hd_t;

esp_midi_err_t esp_midi_files_loader_new(esp_midi_lib_loader_cfg_t cfg, esp_midi_lib_loader_handle_t *lib_handle)
{
    ESP_MIDI_RETURN_ON_FALSE(lib_handle, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "The sound font loader handle is NULL");
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    // create one handle
    esp_midi_files_loader_hd_t *hd = calloc(1, sizeof(esp_midi_files_loader_hd_t));
    ESP_MIDI_RETURN_ON_FALSE(hd, ESP_MIDI_ERR_MEM_LACK, TAG, "The sound font loader handle(size %d) allocation failed", sizeof(esp_midi_files_loader_hd_t));
    // Create link list to store sample data for note on music
    hd->num_nodes = 2;
    hd->buf_len = 2048;
    ret = esp_midi_sample_cache_create_node(hd->num_nodes, hd->buf_len, &hd->sample_cache);
    ESP_MIDI_GOTO_ON_FAIL(_files_loader_exit, TAG, "Sound font parser get sample data handle failed");
    // update paremeter
    *lib_handle = hd;
    return ret;
_files_loader_exit:
    esp_midi_files_loader_delete(hd);
    return ret;
}

esp_midi_err_t esp_midi_files_loader_delete(esp_midi_lib_loader_handle_t lib_handle)
{
    if (lib_handle) {
        esp_midi_files_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
        esp_midi_sample_cache_delete_node(&hd->sample_cache, hd->num_nodes);
        free(lib_handle);
    }
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t esp_midi_files_loader_noteon(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_files_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    int32_t index = 0, note_val = 0, vel_min = 0, vel_max = 0;
    char c[20];
    char *filename[39] = {
        "1_22_Pedal_Hihat_0_40.wav",
        "1_22_Pedal_Hihat_40_80.wav",
        "1_22_Pedal_Hihat_80_127.wav",
        "11_45_Mid_Tom_0_40.wav",
        "11_45_Mid_Tom_40_80.wav",
        "11_45_Mid_Tom_80_127.wav",
        "13_43_High_Tom_0_40.wav",
        "13_43_High_Tom_40_80.wav",
        "13_43_High_Tom_80_127.wav",
        "13_47_High_Tom_0_40.wav",
        "13_47_High_Tom_40_80.wav",
        "13_47_High_Tom_80_127.wav",
        "1_44_Pedal_Hihat_0_40.wav",
        "1_44_Pedal_Hihat_40_80.wav",
        "1_44_Pedal_Hihat_80_127.wav",
        "20_51_Ride_Cymbal_0_40.wav",
        "20_51_Ride_Cymbal_40_80.wav",
        "20_51_Ride_Cymbal_80_127.wav",
        "21_55_Splash_Cymbal_0_40.wav",
        "21_55_Splash_Cymbal_40_80.wav",
        "21_55_Splash_Cymbal_80_127.wav",
        "2_49_Crash_Cymbal_0_40.wav",
        "2_49_Crash_Cymbal_40_80.wav",
        "2_49_Crash_Cymbal_80_127.wav",
        "3_38_Snare_Drum_0_40.wav",
        "3_38_Snare_Drum_40_80.wav",
        "3_38_Snare_Drum_80_127.wav",
        "4_36_Bass_Drum_0_40.wav",
        "4_36_Bass_Drum_40_80.wav",
        "4_36_Bass_Drum_80_127.wav",
        "5_37_Side_Stick_0_40.wav",
        "5_37_Side_Stick_40_80.wav",
        "5_37_Side_Stick_80_127.wav",
        "7_40_Snare_Drum_0_40.wav",
        "7_40_Snare_Drum_40_80.wav",
        "7_40_Snare_Drum_80_127.wav",
        "9_41_Low_Tom_0_40.wav",
        "9_41_Low_Tom_40_80.wav",
        "9_41_Low_Tom_80_127.wav",
    };
    for (int32_t i = 0; i < sizeof(filename) / sizeof(filename[0]); i++) {
        sscanf(filename[i], "%ld_%ld_%[A-z]%ld_%ld.wav", &index, &note_val, c, &vel_min, &vel_max);
        if (vel_max == 127) {
            vel_max = 128;
        }
        if (note_val == sample_desc->note
            && (sample_desc->velocity >= vel_min && sample_desc->velocity < vel_max)) {
            char path[100] = "/sdcard/";
            strcat(path, filename[i]);
            FILE *in_file = fopen(path, "rb");
            ESP_MIDI_RETURN_ON_FALSE(in_file, ESP_MIDI_ERR_FAIL, TAG, "Open input file(%s) failed", filename[i]);
            fseek(in_file, 40, SEEK_SET);
            uint8_t data_len[4];
            int32_t ret = fread(data_len, 1, 4, in_file);
            if (ret != 4) {
                ESP_MIDI_LOGE(TAG, "Read failed, line %d ", __LINE__);
                fclose(in_file);
                return ESP_MIDI_ERR_FAIL;
            }
            uint32_t buf_len = (data_len[3] << 24) | (data_len[2] << 16) | (data_len[1] << 8) | data_len[0];
            esp_midi_sample_cache_t *sample_cache = esp_midi_sample_cache_get_empty_data(&hd->sample_cache, buf_len, &hd->num_nodes);
            if (sample_cache == NULL) {
                fclose(in_file);
                return ESP_MIDI_ERR_FAIL;
            }
            sample_cache->data.len = fread(sample_cache->data.buffer, 1, buf_len, in_file);
            if (sample_cache->data.len != buf_len) {
                ESP_MIDI_LOGE(TAG, "Read failed, line %d ", __LINE__);
                fclose(in_file);
                return ESP_MIDI_ERR_FAIL;
            }
            // open one file and read all data
            sample_desc->loopstart = 0;
            sample_desc->loopend = buf_len >> 1;
            sample_desc->samplerate = 44100;
            sample_desc->bits = 16;
            sample_desc->sampletype = ESP_MIDI_SF_SAMPLE_TYPE_MONO;
            sample_desc->gain = 1.0;
            memcpy(&sample_desc->data, &sample_cache->data, sizeof(esp_midi_data_t));
            fclose(in_file);
            return ESP_MIDI_ERR_OK;
        }
    }
    return ESP_MIDI_ERR_FAIL;
}

esp_midi_err_t esp_midi_files_loader_free_data(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_files_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    esp_midi_sample_cache_search_set_data_unuse(hd->sample_cache, hd->num_nodes, sample_desc->data);
    return ESP_MIDI_ERR_OK;
}
