/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_midi_sound.h"
#include "esp_midi_files_loader.h"
#include "esp_midi_sf_loader.h"
#include "midi_io.h"

static const char *TAG = "MIDI.TEST";

static void printf_mem()
{
    ESP_MIDI_LOGI(TAG, "Internal free heap size: %ld bytes", esp_get_free_internal_heap_size());
    ESP_MIDI_LOGI(TAG, "PSRAM    free heap size: %ld bytes", esp_get_free_heap_size() - esp_get_free_internal_heap_size());
    ESP_MIDI_LOGI(TAG, "Total    free heap size: %ld bytes", esp_get_free_heap_size());
}

esp_midi_err_t fread_cb(uint8_t *buf, uint32_t size, void *ctx)
{
    if (fread(buf, 1, size, ctx) == size) {
        return ESP_MIDI_ERR_OK;
    }
    return ESP_MIDI_ERR_DATA_LACK;
}

esp_midi_err_t fseek_cb(uint32_t size, void *ctx)
{
    fseek(ctx, size, SEEK_CUR);
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t test_sf_loader()
{
    esp_midi_lib_loader_handle_t lib_handle = NULL;
    char *sf_name = "/sdcard/test.sf2";
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    ret = esp_midi_sf_loader_new(sf_name, &lib_handle);
    ESP_MIDI_GOTO_ON_FAIL(_exit, TAG, "process failed , ret %d ", ret);
_exit:
    ret = esp_midi_sf_loader_delete(lib_handle);
    return ret;
}

// update midi file
esp_midi_err_t esp_midi_parse_midi_file(esp_midi_sound_handle_t handle, char *midi_file_name, esp_midi_sound_info_t *sound_info, bool use_fixed_buf_size)
{
    ESP_MIDI_RETURN_ON_FALSE(handle, ESP_MIDI_ERR_INVALID_PARAMETER, TAG, "Handle is NULL");
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_packet_t raw = {NULL};
    FILE *in_file = fopen(midi_file_name, "rb");
    if (in_file == NULL) {
        perror("Read file failed: ");
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
            perror("Read file failed: ");
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

esp_midi_err_t esp_midi_out_data(uint8_t *buf, uint32_t buf_size, void *ctx)
{
    fwrite(buf, 1, buf_size, ctx);
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t test_sound_sf()
{
    esp_midi_sound_handle_t handle = NULL;
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    char *out_file_name = "/sdcard/out_sound_sf.pcm";
    char *midi_file_name = "/sdcard/test.mid";
    esp_midi_sound_info_t sound_info = {0};
    esp_midi_sound_cfg_t sound_cfg = {
        .max_out_stream_frame = 10240,
        .note_off_resp = ESP_MIDI_SYNTH_NOTE_OFF_RESP_REDUCE,
        .out_stream_info.samplerate = 44100,
        .out_stream_info.bits = 16,
        .out_stream_info.channel = 1,
        .out_stream_cb = esp_midi_out_data,
        .sf_lib_cfg.loader_cfg = "/sdcard/test.sf2",
        .sf_lib_cfg.loader_cb = esp_midi_sf_loader_new,
        .sf_lib_cfg.delete_cb = esp_midi_sf_loader_delete,
        .sf_lib_cfg.note_on_cb = esp_midi_sf_loader_noteon,
        .sf_lib_cfg.set_bank_cb = esp_midi_sf_loader_set_bank,
        .sf_lib_cfg.set_preset_cb = esp_midi_sf_loader_set_program,
        .sf_lib_cfg.set_data_cb = esp_midi_sf_loader_set_data,
        .sf_lib_cfg.free_data_cb = esp_midi_sf_loader_free_data,
    };
    /* open output file*/
    sound_cfg.out_stream_ctx = fopen(out_file_name, "wb");
    ESP_MIDI_GOTO_ON_FALSE(sound_cfg.out_stream_ctx, ESP_MIDI_ERR_FAIL, _test_sound_exit, TAG, "Open output file failed");
    /* open sound handle*/
    ret = esp_midi_sound_open(&sound_cfg, &handle);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Open sound handle failed , ret %d ", ret);
    /* load soundfont library handle*/
    ret = esp_midi_sound_load_sf_lib(handle);
    ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Load soundfont library failed , ret %d ", ret);
    for (size_t i = 0; i < 5; i++) {  // can analyze a new MIDI file again
        /* parse midi file*/
        ret = esp_midi_parse_midi_file(handle, midi_file_name, &sound_info, false);
        ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Parse midi file failed , ret %d ", ret);
        /* playing one midi*/
        ret = esp_midi_sound_process(handle);
        ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Sound process failed , ret %d ", ret);
    }
_test_sound_exit:
    /* close  midi*/
    ret = esp_midi_sound_close(handle);
    if (sound_cfg.out_stream_ctx) {
        fclose(sound_cfg.out_stream_ctx);
    }
    return ret;
}

esp_midi_err_t test_sound_files()
{
    printf("%s: %d\n", __func__, __LINE__);
    printf_mem();
    esp_midi_sound_handle_t handle = NULL;
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    char *out_file_name = "/sdcard/out_sound_files.pcm";
    char *midi_file_name[] = {
        "/sdcard/15-94-86.mid",
        "/sdcard/02-110-44.mid",
        "/sdcard/03-100-44.mid",
        "/sdcard/08-130-44.mid",
        "/sdcard/esp_test.mid",
    };
    esp_midi_sound_info_t sound_info = {0};
    uint32_t bpm = 0;
    esp_midi_sound_cfg_t sound_cfg = {
        .max_out_stream_frame = 10240,
        .note_off_resp = ESP_MIDI_SYNTH_NOTE_OFF_RESP_IGNORE,
        .out_stream_info.samplerate = 44100,
        .out_stream_info.bits = 16,
        .out_stream_info.channel = 1,
        .out_stream_cb = esp_midi_out_data,
        .sf_lib_cfg.loader_cb = esp_midi_files_loader_new,
        .sf_lib_cfg.delete_cb = esp_midi_files_loader_delete,
        .sf_lib_cfg.note_on_cb = esp_midi_files_loader_noteon,
        .sf_lib_cfg.free_data_cb = esp_midi_files_loader_free_data,
    };
    /* open output file*/
    sound_cfg.out_stream_ctx = fopen(out_file_name, "wb");
    perror("error:");
    ESP_MIDI_GOTO_ON_FALSE(sound_cfg.out_stream_ctx, ESP_MIDI_ERR_FAIL, _test_sound_exit, TAG, "Open output file failed");
    /* open sound handle*/
    ret = esp_midi_sound_open(&sound_cfg, &handle);
    ESP_MIDI_RETURN_ON_FAIL(TAG, "Open sound handle failed , ret %d ", ret);
    /* load soundfont library handle*/
    ret = esp_midi_sound_load_sf_lib(handle);
    ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Load soundfont library failed , ret %d ", ret);
    for (size_t i = 0; i < 5; i++) {  // can analyze a new MIDI file again
        /* parse midi file*/
        ret = esp_midi_parse_midi_file(handle, midi_file_name[i % 5], &sound_info, true);
        ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Parse midi file failed , ret %d ", ret);
        ret |= esp_midi_sound_get_bpm(handle, &bpm);
        ESP_MIDI_LOGI(TAG, "index:%d bpm: %ld \n", i, bpm);
        /* playing one midi*/
        ret = esp_midi_sound_process(handle);
        ESP_MIDI_GOTO_ON_FAIL(_test_sound_exit, TAG, "Sound process failed , ret %d ", ret);
        bpm += 10;
        ret |= esp_midi_sound_set_bpm(handle, bpm);
    }
_test_sound_exit:
    /* close  midi*/
    ret = esp_midi_sound_close(handle);
    if (sound_cfg.out_stream_ctx) {
        fclose(sound_cfg.out_stream_ctx);
    }
    return ret;
}

// update midi file
esp_midi_err_t esp_midi_enc_file(char *midi_file_name)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    uint8_t buffer[200];
    int32_t fret = 0;
    uint32_t track_pos = 0;
    esp_midi_parse_event_t event = {0};
    esp_midi_header_t header = {
        .format_type = 0,
        .num_of_tracks = 1,
        .time_division = 480,
    };
    esp_midi_packet_t pkt = {
        .data.buffer = buffer,
        .data.len = 200,
        .consumed = 0,
    };
    FILE *out_file = fopen(midi_file_name, "wb");
    if (out_file == NULL) {
        perror("Open file failed");
        goto _file_exit;
    }
    ret |= esp_midi_sound_enc_midi_header(&pkt, &header);
    track_pos = pkt.consumed;
    pkt.consumed += ESP_MIDI_TRACK_HEADER_SIZE;  // track size

    // tempo meta event(opt)
    ret |= esp_midi_sound_enc_tempo(&pkt, 500000);

    // track name meta event(opt)
    event.type = ESP_MIDI_EVENT_TYPE_META_EVENT;
    event.msg.meta_msg.type = ESP_MIDI_META_TYPE_TRACK_NAME;
    event.msg.meta_msg.len = 6;
    char *track_name = "Track1";
    event.msg.meta_msg.data = (uint8_t *)track_name;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // program change event(opt)
    event.type = ESP_MIDI_EVENT_TYPE_PROGRAM_CHANGE;
    event.msg.prog_ch_msg.channel = 0;
    event.msg.prog_ch_msg.program_change = 0;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // Note on event one
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_ON;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 22;
    event.msg.note_msg.velocity = 0x7F;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // Note on event two
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_ON;
    event.delta_ticks = 70;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 44;
    event.msg.note_msg.velocity = 0x40;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // Note on event three
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_ON;
    event.delta_ticks = 100;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 49;
    event.msg.note_msg.velocity = 0x20;
    ret |= esp_midi_sound_enc_event(&event, &pkt, false);

    // Note off event one
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF;
    event.delta_ticks = 30;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 22;
    event.msg.note_msg.velocity = 0x7f;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // Note off event two
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF;
    event.delta_ticks = 10;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 44;
    event.msg.note_msg.velocity = 0x6f;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // Note off event three
    event.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF;
    event.delta_ticks = 10;
    event.msg.note_msg.channel = 0;
    event.msg.note_msg.note = 49;
    event.msg.note_msg.velocity = 0x5f;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // track end meta event
    event.type = ESP_MIDI_EVENT_TYPE_META_EVENT;
    event.msg.meta_msg.type = ESP_MIDI_META_TYPE_EOT;
    event.msg.meta_msg.len = 0;
    ret |= esp_midi_sound_enc_event(&event, &pkt, true);

    // track header
    ret |= esp_midi_sound_enc_track_header(&pkt.data.buffer[track_pos], pkt.data.len - track_pos, pkt.consumed - track_pos - ESP_MIDI_TRACK_HEADER_SIZE);

    fret = fwrite(pkt.data.buffer, 1, pkt.consumed, out_file);
    if (fret != pkt.consumed) {
        ESP_MIDI_LOGE(TAG, "Fwrite failed. ret %ld \n", fret);
        goto _file_exit;
    }
    for (size_t i = 0; i < pkt.consumed; i++) {
        if (i % 16 == 0) {
            printf("\n");
        }
        printf("%.2x,", pkt.data.buffer[i]);
    }
    printf("\n");

_file_exit:
    if (out_file) {
        fclose(out_file);
    }

    return ret;
}

esp_midi_err_t esp_midi_enc_dec_sound(char *out_file_name)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    uint8_t buffer[200];
    esp_midi_packet_t pkt = {
        .data.buffer = buffer,
        .data.len = 200,
        .consumed = 0,
    };
    uint32_t temp0 = 500000;
    char *track_name = "Track1";
    int32_t last_status = 0;
    esp_midi_parse_event_t event[9] = {
        {.type = ESP_MIDI_EVENT_TYPE_META_EVENT, .delta_ticks = 0, .msg.meta_msg.type = ESP_MIDI_META_TYPE_TRACK_NAME, .msg.meta_msg.len = 6, .msg.meta_msg.data = (uint8_t *)track_name},
        {.type = ESP_MIDI_EVENT_TYPE_PROGRAM_CHANGE, .delta_ticks = 0, .msg.prog_ch_msg.channel = 0, .msg.prog_ch_msg.program_change = 0},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_ON, .delta_ticks = 0, .msg.note_msg.channel = 0, .msg.note_msg.note = 22, .msg.note_msg.velocity = 0x7F},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_ON, .delta_ticks = 70, .msg.note_msg.channel = 0, .msg.note_msg.note = 44, .msg.note_msg.velocity = 0x40},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_ON, .delta_ticks = 100, .msg.note_msg.channel = 0, .msg.note_msg.note = 49, .msg.note_msg.velocity = 0x20},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF, .delta_ticks = 30, .msg.note_msg.channel = 0, .msg.note_msg.note = 22, .msg.note_msg.velocity = 0x7F},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF, .delta_ticks = 10, .msg.note_msg.channel = 0, .msg.note_msg.note = 44, .msg.note_msg.velocity = 0x6f},
        {.type = ESP_MIDI_EVENT_TYPE_NOTE_OFF, .delta_ticks = 10, .msg.note_msg.channel = 0, .msg.note_msg.note = 49, .msg.note_msg.velocity = 0x5f},
        {.type = ESP_MIDI_EVENT_TYPE_META_EVENT, .delta_ticks = 10, .msg.meta_msg.type = ESP_MIDI_META_TYPE_EOT, .msg.meta_msg.len = 0, .msg.meta_msg.data = NULL},
    };
    esp_midi_parse_event_t out_event = {0};
    esp_midi_sound_handle_t handle = NULL;
    esp_midi_sound_cfg_t sound_cfg = {
        .max_out_stream_frame = 10240,
        .note_off_resp = ESP_MIDI_SYNTH_NOTE_OFF_RESP_IGNORE,
        .out_stream_info.samplerate = 44100,
        .out_stream_info.bits = 16,
        .out_stream_info.channel = 1,
        .out_stream_cb = esp_midi_out_data,
        .sf_lib_cfg.loader_cb = esp_midi_files_loader_new,
        .sf_lib_cfg.delete_cb = esp_midi_files_loader_delete,
        .sf_lib_cfg.note_on_cb = esp_midi_files_loader_noteon,
        .sf_lib_cfg.free_data_cb = esp_midi_files_loader_free_data,
    };

    /* open output file*/
    sound_cfg.out_stream_ctx = fopen(out_file_name, "wb");
    perror("error:");
    ESP_MIDI_GOTO_ON_FALSE(sound_cfg.out_stream_ctx, ESP_MIDI_ERR_FAIL, _enc_dec_sound_exit, TAG, "Open output file failed");

    /* open sound handle*/
    ret = esp_midi_sound_open(&sound_cfg, &handle);

    ret = esp_midi_sound_set_bpm(handle, 120);
    ret = esp_midi_sound_set_time_division(handle, 480);

    /* load soundfont library handle*/
    ret = esp_midi_sound_load_sf_lib(handle);
    ESP_MIDI_GOTO_ON_FAIL(_enc_dec_sound_exit, TAG, "Load soundfont library failed , ret %d ", ret);

    for (size_t i = 0; i < 9; i++) {
        pkt.consumed = 0;
        ret = esp_midi_sound_enc_event(&event[i], &pkt, true);
        if (ret != ESP_MIDI_ERR_OK) {
            goto _enc_dec_sound_exit;
        }
        pkt.consumed = 0;
        ret = esp_midi_sound_parse_event(&last_status, &pkt, &out_event);
        if (ret != ESP_MIDI_ERR_OK) {
            goto _enc_dec_sound_exit;
        }
        ret = esp_midi_sound_pro_event(handle, &out_event);
        if (ret != ESP_MIDI_ERR_OK) {
            goto _enc_dec_sound_exit;
        }
        esp_midi_sound_free_event(&out_event);
    }

_enc_dec_sound_exit:
    /* close  midi*/
    ret = esp_midi_sound_close(handle);
    if (sound_cfg.out_stream_ctx) {
        fclose(sound_cfg.out_stream_ctx);
    }
    return ret;
}

TEST_CASE("midi_playing_file", "[midi]")
{
    mount_sd();
    TEST_ASSERT_EQUAL(ESP_MIDI_ERR_OK, test_sf_loader());
    TEST_ASSERT_EQUAL(ESP_MIDI_ERR_OK, test_sound_sf());
    TEST_ASSERT_EQUAL(ESP_MIDI_ERR_OK, test_sound_files());
    unmount_sd();
}

TEST_CASE("midi_enc_file", "[midi]")
{
    mount_sd();
    TEST_ASSERT_EQUAL(ESP_MIDI_ERR_OK, esp_midi_enc_file("/sdcard/esp_test.mid"));
    unmount_sd();
}

TEST_CASE("midi_enc_dec_sound", "[midi]")
{
    mount_sd();
    TEST_ASSERT_EQUAL(ESP_MIDI_ERR_OK, esp_midi_enc_dec_sound("/sdcard/esp_test.pcm"));
    unmount_sd();
}
