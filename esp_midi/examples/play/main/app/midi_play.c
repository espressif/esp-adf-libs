/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_midi_sound.h"
#include "esp_midi_files_loader.h"
#include "esp_midi_flash_loader.h"
#include "esp_midi_sf_loader.h"
#include "bsp/esp-bsp.h"
#include "midi_button.h"
#include "midi_play.h"

static const char *TAG = "MIDI_PLAY";

typedef struct {
    esp_codec_dev_handle_t spk_codec_dev;
    int midi_file_index;
    int midi_volume;
    EventGroupHandle_t midi_event_group;
    esp_midi_sound_handle_t handle;
} midi_play_t;
midi_play_t *midi_play_hd = NULL;

#define BUTTON_PLAY_BIT        (1 << 0)
#define BUTTON_MUTE_BIT        (1 << 1)
#define BUTTON_REC_BIT         (1 << 2)

#define MIDI_FILE_NUM   4

char *midi_file_list[MIDI_FILE_NUM] = {
    "midi_sample_1.mid",
    "midi_sample_2.mid",
    "midi_sample_3.mid",
    "midi_sample_4.mid",
};

static void esp_system_mem_info(void)
{
    ESP_LOGI(TAG, "Internal free heap size: %ld bytes", esp_get_free_internal_heap_size());
    ESP_LOGI(TAG, "PSRAM    free heap size: %ld bytes", esp_get_free_heap_size() - esp_get_free_internal_heap_size());
    ESP_LOGI(TAG, "Total    free heap size: %ld bytes", esp_get_free_heap_size());
}

static esp_midi_err_t esp_midi_parse_midi_file(esp_midi_sound_handle_t handle, char *midi_file_name, esp_midi_sound_info_t *sound_info, bool use_fixed_buf_size)
{
    ESP_MIDI_RETURN_ON_FALSE(handle, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "Handle is NULL");
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_packet_t raw = {NULL};
    FILE *in_file = fopen(midi_file_name, "rb");
    if (in_file == NULL) {
        // perror("line %d, Read file failed: ", __LINE__);
        goto _reset_file_exit;
    }
    if (use_fixed_buf_size == true) {
        uint32_t buf_len = 1024;
        raw.consumed = buf_len;
        raw.data.len = buf_len;
        // alocate memory
        raw.data.buffer = calloc(1, buf_len);
        ESP_MIDI_GOTO_ON_FALSE(raw.data.buffer, ESP_MIDI_ERR_MEM_LACK, _reset_file_exit, TAG, "Input buffer (size %ld) allocation failed", buf_len);
        // read file
        while (1) {
            int32_t fret = fread(raw.data.buffer + raw.data.len - raw.consumed, 1, raw.consumed, in_file);
            if (fret <= 0) {
                if (fret < 0) {
                    ESP_MIDI_LOGE(TAG, "Fread failed. ret %ld \n", fret);
                }
                goto _reset_file_exit;
            };
            raw.data.len = fret + raw.data.len - raw.consumed;
            raw.consumed = 0;
            // parse midi
            ret = esp_midi_sound_parse(handle, &raw, sound_info);
            if (ret <= ESP_MIDI_ERR_OK) {
                goto _reset_file_exit;
            }
            memcpy(raw.data.buffer, raw.data.buffer + raw.consumed, raw.data.len - raw.consumed);
        }
    } else {
        // get file size;
        fseek(in_file, 0L, SEEK_END);
        raw.data.len = ftell(in_file);
        // alocate memory
        raw.data.buffer = calloc(1, raw.data.len);
        ESP_MIDI_GOTO_ON_FALSE(raw.data.buffer, ESP_MIDI_ERR_MEM_LACK, _reset_file_exit, TAG, "Input buffer (size %ld) allocation failed", raw.data.len);
        // seek to file start
        fseek(in_file, 0L, SEEK_SET);
        // read file
        if (raw.data.len != fread(raw.data.buffer, 1, raw.data.len, in_file)) {
            goto _reset_file_exit;
        };
        // parse midi
        ret |= esp_midi_sound_parse(handle, &raw, sound_info);
    }
_reset_file_exit:
    if (raw.data.buffer) {
        free(raw.data.buffer);
    }
    if (in_file) {
        fclose(in_file);
    }
    return ret;
}

static esp_midi_err_t esp_midi_out_data(uint8_t *buf, uint32_t buf_size, void *ctx)
{
    int ret = esp_codec_dev_write(midi_play_hd->spk_codec_dev, buf, buf_size);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "esp_codec_dev_write failed , ret %d ", ret);
    return ESP_MIDI_ERR_OK;
}

static esp_midi_err_t midi_play_init()
{
    midi_play_hd->handle = NULL;
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;

    esp_midi_sound_cfg_t sound_cfg = {
        .max_out_stream_frame = 10240,
        .note_off_resp = ESP_MIDI_SYNTH_NOTE_OFF_RESP_IGNORE,
        .out_stream_info.samplerate = 44100,
        .out_stream_info.bits = 16,
        .out_stream_info.channel = 1,
        .out_stream_cb = esp_midi_out_data,
        .sf_lib_cfg.loader_cb = esp_midi_flash_loader_new,
        .sf_lib_cfg.delete_cb = esp_midi_flash_loader_delete,
        .sf_lib_cfg.note_on_cb = esp_midi_flash_loader_noteon,
    };
    /* open sound handle*/
    ret = esp_midi_sound_open(&sound_cfg, &midi_play_hd->handle);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Open sound handle failed , ret %d ", ret);
    /* load soundfont library handle*/
    ret = esp_midi_sound_load_sf_lib(midi_play_hd->handle);
    ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Load soundfont library failed , ret %d ", ret);

    return ret;

_test_sound_exit:
    /* close midi*/
    ret = esp_midi_sound_close(midi_play_hd->handle);
    midi_play_hd->handle = NULL;
    return ret;
}

static esp_midi_err_t midi_play_resume(void)
{
    if (midi_play_hd->handle == NULL) {
        ESP_LOGE(TAG, "midi_play_hd->handle is NULL");
        return ESP_MIDI_ERR_INVALID_PARAMETER;
    }

    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    ret = esp_midi_sound_resume(midi_play_hd->handle);
    ESP_MIDI_GOTO_ON_FAIL(_resume_sound_exit, TAG, "esp_midi_sound_resume failed , ret %d ", ret);

    ret = esp_midi_sound_process(midi_play_hd->handle);
    ESP_MIDI_GOTO_ON_FAIL(_resume_sound_exit, TAG, "Sound process failed , ret %d ", ret);

    return ret;

_resume_sound_exit:
    /* close midi*/
    ret = esp_midi_sound_close(midi_play_hd->handle);
    midi_play_hd->handle = NULL;
    return ret;
}

static esp_midi_err_t midi_play_start(void)
{
    if (midi_play_hd->handle == NULL) {
        midi_play_init();
        if (midi_play_hd->handle == NULL) {
            ESP_LOGE(TAG, "midi_play_hd->handle is NULL");
            return ESP_MIDI_ERR_INVALID_PARAMETER;
        }
    }

    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sound_info_t sound_info = {0};

    char midi_file_name[100];
    sprintf(midi_file_name, "/sdcard/%s", midi_file_list[midi_play_hd->midi_file_index]);
    ESP_LOGI(TAG, "midi_file_name: %s", midi_file_name);

    /* parse midi file*/
    ret = esp_midi_parse_midi_file(midi_play_hd->handle, midi_file_name, &sound_info, true);
    ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Parse midi file failed , ret %d ", ret);

    /* playing one midi*/
    ret = midi_play_resume();
    ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "midi_play_resume failed , ret %d ", ret);

    return ret;

_test_sound_exit:
    /* close midi*/
    ret = esp_midi_sound_close(midi_play_hd->handle);
    midi_play_hd->handle = NULL;
    return ret;
}

esp_err_t midi_play_pause(void)
{
    esp_err_t ret = ESP_MIDI_ERR_OK;
    if (midi_play_hd->handle) {
        ret = esp_midi_sound_pause(midi_play_hd->handle);
    }
    return ret;
}

esp_err_t midi_play_close(void)
{
    esp_err_t ret = ESP_MIDI_ERR_OK;
    if (midi_play_hd->handle) {
        ret = esp_midi_sound_close(midi_play_hd->handle);
        midi_play_hd->handle = NULL;
    }
    return ret;
}

esp_err_t midi_play_set_bpm(void)
{
    esp_err_t ret = ESP_MIDI_ERR_OK;
    ret = esp_midi_sound_set_bpm(midi_play_hd->handle, 120);
    return ret;
}

void midi_volume_increase(void)
{
    midi_play_hd->midi_volume += 10;
    if (midi_play_hd->midi_volume > 100) {
        midi_play_hd->midi_volume = 100;
    }
    esp_codec_dev_set_out_vol(midi_play_hd->spk_codec_dev, midi_play_hd->midi_volume);
    ESP_LOGI(TAG, "midi_volume: %d", midi_play_hd->midi_volume);
}

void midi_volume_decrease(void)
{
    midi_play_hd->midi_volume -= 10;
    if (midi_play_hd->midi_volume < 0) {
        midi_play_hd->midi_volume = 0;
    }
    esp_codec_dev_set_out_vol(midi_play_hd->spk_codec_dev, midi_play_hd->midi_volume);
    ESP_LOGI(TAG, "midi_volume: %d", midi_play_hd->midi_volume);
}

void midi_file_index_increase(void)
{
    midi_play_hd->midi_file_index = (midi_play_hd->midi_file_index + 1) % MIDI_FILE_NUM;
    ESP_LOGI(TAG, "midi_file_index: %d", midi_play_hd->midi_file_index);
}

void midi_event_play(void)
{
    xEventGroupSetBits(midi_play_hd->midi_event_group, BUTTON_PLAY_BIT);
}

void midi_event_mute(void)
{
    static uint32_t bpm = 0;
    if (bpm == 0) {
        esp_midi_sound_get_bpm(midi_play_hd->handle, &bpm);
        ESP_LOGI(TAG, "get bpm: %ld", bpm);
    } else {
        bpm += 20;
        if (bpm > 200) {
            bpm -= 140;
        }
        ESP_LOGI(TAG, "set bpm: %ld", bpm);
        esp_midi_sound_set_bpm(midi_play_hd->handle, bpm);
    }
}

void midi_event_rec(void)
{
    midi_play_pause();

    xEventGroupSetBits(midi_play_hd->midi_event_group, BUTTON_REC_BIT);
}

void midi_play_task(void *arg)
{
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            midi_play_hd->midi_event_group, 
            BUTTON_PLAY_BIT | BUTTON_MUTE_BIT | BUTTON_REC_BIT, 
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        if (bits & BUTTON_PLAY_BIT) {
            midi_play_start();
            ESP_LOGI(TAG, "midi play finish");
        }

        if (bits & BUTTON_REC_BIT) {
            ESP_LOGI(TAG, "midi play close");
            midi_play_close();
        }

        xEventGroupClearBits(midi_play_hd->midi_event_group, BUTTON_PLAY_BIT | BUTTON_MUTE_BIT | BUTTON_REC_BIT);
    }
}

void midi_player_init(void)
{
    esp_err_t ret = ESP_MIDI_ERR_OK;

    midi_play_hd = (midi_play_t *)calloc(1, sizeof(midi_play_t));
    if (midi_play_hd == NULL) {
        ESP_LOGE(TAG, "init midi_play_hd failed");
        goto _init_midi_play_exit;
    }
    midi_play_hd->midi_file_index = 0;
    midi_play_hd->midi_volume = 50;
    midi_play_hd->midi_event_group = xEventGroupCreate();
    if (midi_play_hd->midi_event_group == NULL) {
        ESP_LOGE(TAG, "xEventGroupCreate failed");
        goto _init_midi_play_exit;
    }

    ret = midi_play_init();
    if (ret != ESP_MIDI_ERR_OK) {
        ESP_LOGE(TAG, "midi_play_init failed , ret %d ", ret);
        goto _init_midi_play_exit;
    }

    // init speaker
    midi_play_hd->spk_codec_dev = bsp_audio_codec_speaker_init();
    assert(midi_play_hd->spk_codec_dev);
    esp_codec_dev_set_out_vol(midi_play_hd->spk_codec_dev, midi_play_hd->midi_volume);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 44100,
        .channel = 1,
        .bits_per_sample = 16,
    };
    ret = esp_codec_dev_open(midi_play_hd->spk_codec_dev, &fs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed , ret %d ", ret);
        goto _init_midi_play_exit;
    }
    return;

_init_midi_play_exit:
    if (midi_play_hd) {
        midi_play_close();
        if (midi_play_hd->midi_event_group) {
            vEventGroupDelete(midi_play_hd->midi_event_group);
            midi_play_hd->midi_event_group = NULL;
        }
        free(midi_play_hd);
        midi_play_hd = NULL;
    }
    return;
}
