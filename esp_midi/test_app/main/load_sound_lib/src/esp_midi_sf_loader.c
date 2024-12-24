/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <stdio.h>
#include "esp_midi_sf_loader.h"
#include "esp_midi_sf_parse.h"
#include "esp_midi_sample_cache.h"

static const char *TAG = "MIDI.SF.LOADER";

typedef struct {
    esp_midi_lib_loader_handle_t  lib_handle;
    FILE                         *sf_file_hd;
    uint32_t                      buf_len;
    uint16_t                      num_nodes;
    esp_midi_sample_cache_t      *sample_cache;
} esp_midi_sf_loader_hd_t;

esp_midi_err_t sf_fread_cb(uint8_t *buf, uint32_t size, void *ctx)
{
    if (fread(buf, 1, size, ctx) == size) {
        return ESP_MIDI_ERR_OK;
    }
    return ESP_MIDI_ERR_DATA_LACK;
}

esp_midi_err_t sf_fseek_cb(uint32_t size, void *ctx)
{
    fseek(ctx, size, SEEK_CUR);
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t esp_midi_sf_loader_new(esp_midi_lib_loader_cfg_t sf_name, esp_midi_lib_loader_handle_t *lib_handle)
{
    ESP_MIDI_RETURN_ON_FALSE(sf_name, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "The sound font loader configure is NULL");
    ESP_MIDI_RETURN_ON_FALSE(lib_handle, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "The sound font loader handle is NULL");
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    // configure parser
    esp_midi_sf_parse_cfg_t cfg = {
        .read = sf_fread_cb,
        .seek = sf_fseek_cb,
#ifdef USE_MEMOEY_STORE_SAMPLE_DATA
        .need_load_sample = true,
#else
        .need_load_sample = false,
#endif  /* USE_MEMOEY_STORE_SAMPLE_DATA */
    };
    // create one handle
    esp_midi_sf_loader_hd_t *hd = calloc(1, sizeof(esp_midi_sf_loader_hd_t));
    ESP_MIDI_RETURN_ON_FALSE(hd, ESP_MIDI_ERR_MEM_LACK, TAG, "The sound font loader handle(size %d) allocation failed", sizeof(esp_midi_sf_loader_hd_t));
    // open SF file
    cfg.ctx = fopen(sf_name, "rb");
    ESP_MIDI_GOTO_ON_FALSE(cfg.ctx, ESP_MIDI_ERR_FAIL, _sf_loader_exit, TAG, "Open input file(%s) failed", (char *)sf_name);
    hd->sf_file_hd = cfg.ctx;
    // open SF parser
    ret = esp_midi_sf_parse_open(&cfg, &hd->lib_handle);
    ESP_MIDI_GOTO_ON_FAIL(_sf_loader_exit, TAG, "Sound font parser open failed");
    // parsing SF file
    ret = esp_midi_sf_parse_process(hd->lib_handle);
    ESP_MIDI_GOTO_ON_FAIL(_sf_loader_exit, TAG, "Sound font parser process failed");

#ifdef USE_MEMOEY_STORE_SAMPLE_DATA
    // close SF file
    if (cfg.ctx) {
        fclose(cfg.ctx);
    }
#else
    // Create link list to store sample data for note on music
    hd->num_nodes = 2;
    hd->buf_len = 2048;
    ret = esp_midi_sample_cache_create_node(hd->num_nodes, hd->buf_len, &hd->sample_cache);
    ESP_MIDI_GOTO_ON_FAIL(_sf_loader_exit, TAG, "Sound font parser get sample data handle failed");
#endif  /* USE_MEMOEY_STORE_SAMPLE_DATA */
    // update paremeter
    *lib_handle = hd;
    return ret;
_sf_loader_exit:
    if (cfg.ctx) {
        fclose(cfg.ctx);
    }
    esp_midi_sf_loader_delete(hd);
    return ret;
}

esp_midi_err_t esp_midi_sf_loader_delete(esp_midi_lib_loader_handle_t lib_handle)
{
    if (lib_handle) {
        esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
#ifdef USE_MEMOEY_STORE_SAMPLE_DATA
#else
        if (hd->sf_file_hd) {
            fclose(hd->sf_file_hd);
        }
        esp_midi_sample_cache_delete_node(&hd->sample_cache, hd->num_nodes);
#endif  /* USE_MEMOEY_STORE_SAMPLE_DATA */
        esp_midi_sf_parse_close(hd->lib_handle);
        free(lib_handle);
    }
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t esp_midi_sf_loader_set_data(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
#ifdef USE_MEMOEY_STORE_SAMPLE_DATA
    // read SF file sample data
    ret = esp_midi_sf_parse_get_sample(hd->lib_handle, sample_desc->loopstart, sample_desc->loopend, &sample_desc->data);
    sample_desc->loopstart = 0;
    sample_desc->loopend = (sample_desc->data.len << 3) / sample_desc->bits;
    return ret;
#else
    uint32_t buf_len = (sample_desc->loopend - sample_desc->loopstart) << 1;
    esp_midi_sample_cache_t *sample_cache = esp_midi_sample_cache_get_empty_data(&hd->sample_cache, buf_len, &hd->num_nodes);
    ESP_MIDI_RETURN_ON_FALSE(sample_cache, ESP_MIDI_ERR_FAIL, TAG, "Get empty node failed");
    esp_midi_sf_sdat_t sdat = {0};
    ret = esp_midi_sf_parse_get_sdat(hd->lib_handle, &sdat);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Sound font parser get sdat failed");
    fseek(hd->sf_file_hd, (sample_desc->loopstart << 1) + sdat.smpl_pos, SEEK_SET);
    if (fread(sample_cache->data.buffer, 1, buf_len, hd->sf_file_hd) != buf_len) {
        perror("Error: ");
        ESP_MIDI_LOGE(TAG, "Read (size %ld) failed", buf_len);
        esp_midi_sample_cache_set_data_unuse(sample_cache);
        return ESP_MIDI_ERR_FAIL;
    }
    sample_desc->loopstart = 0;
    sample_desc->loopend = buf_len >> 1;
    memcpy(&sample_desc->data, &sample_cache->data, sizeof(esp_midi_data_t));
#endif  /* USE_MEMOEY_STORE_SAMPLE_DATA */
    return ret;
}

esp_midi_err_t esp_midi_sf_loader_free_data(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;

#ifdef USE_MEMOEY_STORE_SAMPLE_DATA
#else
    esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    esp_midi_sample_cache_set_data_unuse(hd->sample_cache);
#endif  /* USE_MEMOEY_STORE_SAMPLE_DATA */
    return ret;
}

esp_midi_err_t esp_midi_sf_loader_noteon(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    esp_midi_sf_sample_t *sample = NULL;
    ret = esp_midi_sf_parse_note_on(hd->lib_handle, sample_desc->note, sample_desc->velocity, &sample);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Sound font parser handle note on failed");
    sample_desc->loopstart = sample->loopstart;
    sample_desc->loopend = sample->loopend;
    sample_desc->origpitch = sample->origpitch;
    sample_desc->pitchadj = sample->pitchadj;
    sample_desc->samplerate = sample->samplerate;
    sample_desc->bits = 16;
    sample_desc->sampletype = sample->sampletype;
    sample_desc->gain = 1.0;
    return ret;
}

esp_midi_err_t esp_midi_sf_loader_set_bank(esp_midi_lib_loader_handle_t lib_handle, int32_t bank)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    ret = esp_midi_sf_parse_set_bank(hd->lib_handle, bank);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Sound font parser set bank failed");
    return ret;
}

esp_midi_err_t esp_midi_sf_loader_set_program(esp_midi_lib_loader_handle_t lib_handle, int32_t prenum)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sf_loader_hd_t *hd = (esp_midi_lib_loader_handle_t)lib_handle;
    ret = esp_midi_sf_parse_set_program(hd->lib_handle, prenum);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Sound font parser set prenum failed");
    return ret;
}
