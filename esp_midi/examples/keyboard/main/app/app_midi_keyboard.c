/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "iot_knob.h"
#include "iot_button.h"
#include "app_board.h"
#include "app_matrix.h"
#include "midi_sound_files.h"
#include "esp_midi_sound.h"
#include "esp_midi_flash_loader.h"
#include "esp_ae_sonic.h"
#include "app_midi_keyboard.h"

static const char *TAG = "MIDI_KEYBOARD";

#define BUTTON_STATUS_DOWN  0
#define BUTTON_STATUS_UP    1
#define MIDI_CENTER_NOTE    60
#define MIDI_SAMPLE_RATE    16000
#define MIDI_CHANNEL        1
#define SONIC_BUFFER_SIZE   1024

static QueueHandle_t keyboard_input_queue = NULL;
static int button_status[2] = {1, 1};

static esp_midi_sound_handle_t midi_handle = NULL;
static QueueHandle_t midi_event_queue = NULL;
static int midi_base_note = MIDI_CENTER_NOTE;
static int midi_cur_instr_index = 0;
static int midi_cur_lower = 0;
static int midi_cur_higher = 127;

static void *sonic_handle = NULL;
static uint8_t *sonic_buffer = NULL;
static float sonic_speed[7] = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};
static int sonic_speed_index = 2;

static inline void switch_to_prev_instrument(void)
{
    int target_instr = midi_cur_instr_index - 1;
    if (target_instr < 0) {
        target_instr = MIDI_INSTRUMENT_NUMBER - 1;
    }
    midi_cur_instr_index = target_instr;
    midi_base_note = MIDI_CENTER_NOTE;
    midi_cur_lower = instrument_info_table[midi_cur_instr_index].min_note;
    midi_cur_higher = instrument_info_table[midi_cur_instr_index].max_note;
    ESP_LOGI(TAG, "Knob B left, change current instrument index: %d", midi_cur_instr_index);
}

static inline void switch_to_next_instrument(void)
{
    int target_instr = midi_cur_instr_index + 1;
    if (target_instr >= MIDI_INSTRUMENT_NUMBER) {
        target_instr = 0;
    }
    midi_cur_instr_index = target_instr;
    midi_base_note = MIDI_CENTER_NOTE;
    midi_cur_lower = instrument_info_table[midi_cur_instr_index].min_note;
    midi_cur_higher = instrument_info_table[midi_cur_instr_index].max_note;
    ESP_LOGI(TAG, "Knob B right, change current instrument index: %d", midi_cur_instr_index);
}

static inline void decrease_playback_speed(void)
{
    if (sonic_speed_index > 0) {
        sonic_speed_index--;
        esp_ae_sonic_set_speed(sonic_handle, sonic_speed[sonic_speed_index]);
        ESP_LOGI(TAG, "Decrease playback speed: %.2f", sonic_speed[sonic_speed_index]);
    }
}

static inline void increase_playback_speed(void)
{
    if (sonic_speed_index < (sizeof(sonic_speed) / sizeof(sonic_speed[0])) - 1) {
        sonic_speed_index++;
        esp_ae_sonic_set_speed(sonic_handle, sonic_speed[sonic_speed_index]);
        ESP_LOGI(TAG, "Increase playback speed: %.2f", sonic_speed[sonic_speed_index]);
    }
}

static inline void action_button_a_down(void)
{
    button_status[0] = 0;
}

static inline void action_button_a_up(void)
{
    button_status[0] = 1;
}

static inline void action_button_a_click(void)
{

}

static inline void action_button_b_down(void)
{
    button_status[1] = 0;
}

static inline void action_button_b_up(void)
{
    button_status[1] = 1;
}

static inline void action_button_b_click(void)
{

}

static inline void action_knob_a_left(void)
{
    int target_note = midi_base_note;
    if (button_status[0] == BUTTON_STATUS_DOWN) {
        target_note = midi_base_note - 1;
    } else {
        target_note = midi_base_note - 12;
    }
    if (target_note < midi_cur_lower) {
        target_note = midi_cur_lower;
    }
    midi_base_note = target_note;
    ESP_LOGI(TAG, "Knob A left, change base note: %d", midi_base_note);
}

static inline void action_knob_a_right(void)
{
    int target_note = midi_base_note;
    if (button_status[0] == BUTTON_STATUS_DOWN) {
        target_note = midi_base_note + 1;
    } else {
        target_note = midi_base_note + 12;
    }
    if (target_note + KEYBOARD_INPUT_NOTE_NUMBER - 1 > midi_cur_higher) {
        target_note = midi_cur_higher - KEYBOARD_INPUT_NOTE_NUMBER + 1;
    }
    midi_base_note = target_note;
    ESP_LOGI(TAG, "Knob A right, change base note: %d", midi_base_note);
}

static inline void action_knob_b_left(void)
{
    if (button_status[1] == BUTTON_STATUS_DOWN) {
        switch_to_prev_instrument();
    } else {
        decrease_playback_speed();
    }
}

static inline void action_knob_b_right(void)
{
    if (button_status[1] == BUTTON_STATUS_DOWN) {
        switch_to_next_instrument();
    } else {
        increase_playback_speed();
    }
}

static inline int get_midi_note_from_matrix(int x, int y) {
    if (x < 0 || x >= 5 || y < 0 || y >= 5) {
        ESP_LOGW(TAG, "Invalid matrix key: (%d, %d)", x, y);
        return MIDI_CENTER_NOTE;
    }
    return midi_base_note + matrix_index_array[x][y];
}

static void keyboard_input_task(void *arg) {
    keyboard_input_t input = {0};
    esp_midi_parse_event_t event = {0};

    while (1) {
        if (xQueueReceive(keyboard_input_queue, &input, portMAX_DELAY) == pdTRUE) {
            if (input.type == KEYBOARD_INPUT_MATRIX) {
                event.type = ESP_MIDI_EVENT_TYPE_NOTE_ON;
                event.delta_ticks = 0;
                event.msg.note_msg.channel = 0;
                event.msg.note_msg.note = get_midi_note_from_matrix(input.value1, input.value2);
                event.msg.note_msg.velocity = 0x7F;
                ESP_LOGI(TAG, "note on: %d", event.msg.note_msg.note);
                xQueueSend(midi_event_queue, &event, KEYBOARD_INPUT_WAIT_TICK);
            } else if (input.type == KEYBOARD_INPUT_USB_HID) {
                event.type = ESP_MIDI_EVENT_TYPE_NOTE_ON;
                event.delta_ticks = 0;
                event.msg.note_msg.channel = 0;
                event.msg.note_msg.note = midi_base_note + input.value1;
                event.msg.note_msg.velocity = 0x7F;
                ESP_LOGI(TAG, "note on: %d", event.msg.note_msg.note);
                xQueueSend(midi_event_queue, &event, KEYBOARD_INPUT_WAIT_TICK);
            } else if (input.type == KEYBOARD_INPUT_KNOB) {
                if (input.value1 == 0) {
                    if (input.value2 == KNOB_LEFT) {
                        action_knob_a_left();
                    } else if (input.value2 == KNOB_RIGHT) {
                        action_knob_a_right();
                    }
                } else if (input.value1 == 1) {
                    if (input.value2 == KNOB_LEFT) {
                        action_knob_b_left();
                    } else if (input.value2 == KNOB_RIGHT) {
                        action_knob_b_right();
                    }
                }
            } else if (input.type == KEYBOARD_INPUT_BUTTON) {
                if (input.value1 == 0) {
                    if (input.value2 == BUTTON_PRESS_DOWN) {
                        action_button_a_down();
                    } else if (input.value2 == BUTTON_PRESS_UP) {
                        action_button_a_up();
                    } else if (input.value2 == BUTTON_SINGLE_CLICK) {
                        action_button_a_click();
                    }
                } else if (input.value1 == 1) {
                    if (input.value2 == BUTTON_PRESS_DOWN) {
                        action_button_b_down();
                    } else if (input.value2 == BUTTON_PRESS_UP) {
                        action_button_b_up();
                    } else if (input.value2 == BUTTON_SINGLE_CLICK) {
                        action_button_b_click();
                    }
                }
            } else if (input.type == KEYBOARD_INPUT_TASK_DEL) {
                ESP_LOGI(TAG, "Task delete signal received. Exiting task.\n");
                break;
            }
        }
    }

    vTaskDelete(NULL);
}

void *keyboard_input_init(void) {
    keyboard_input_queue = xQueueCreate(15, sizeof(keyboard_input_t));
    if (keyboard_input_queue == NULL) {
        return NULL;
    }

    esp_err_t ret = xTaskCreate(keyboard_input_task, "keyboard_input_task", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        vQueueDelete(keyboard_input_queue);
        return NULL;
    }

    return (void *)keyboard_input_queue;
}

void keyboard_input_deinit(void) {
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_TASK_DEL,
        .value1 = 0,
        .value2 = 0,
    };
    xQueueSend(keyboard_input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    vQueueDelete(keyboard_input_queue);
    keyboard_input_queue = NULL;
}

int midi_player_get_current_instr_index(void)
{
    return midi_cur_instr_index;
}

static void midi_player_task(void *arg)
{
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_parse_event_t event = {0};
    esp_midi_parse_event_t sync_event = {
        .type = ESP_MIDI_EVENT_TYPE_SYNC,
        .delta_ticks = 2,
    };

    while (1) {
        if (xQueueReceive(midi_event_queue, &event, 1 / portTICK_PERIOD_MS) == pdTRUE) {
            ret = esp_midi_sound_pro_event(midi_handle, &event);
        } else {
            ret = esp_midi_sound_pro_event(midi_handle, &sync_event);
        }
        if (ret != ESP_MIDI_ERR_OK) {
            ESP_LOGE(TAG, "Error processing MIDI event");
        }
    }

}

static esp_midi_err_t esp_midi_out_data(uint8_t *buf, uint32_t buf_size, void *ctx)
{
    esp_ae_sonic_in_data_t in_samples = {0};
    esp_ae_sonic_out_data_t out_samples = {0};
    out_samples.samples = sonic_buffer;
    int ret = 0;
    short *in = (short *)buf;
    int out_num = SONIC_BUFFER_SIZE / MIDI_CHANNEL / sizeof(short);
    int in_read = 0;
    int sample_num = 0;
    int remain_num = 0;
    int total_remain_len = buf_size;
    while (total_remain_len > 0) {
        in_read = total_remain_len > SONIC_BUFFER_SIZE ? SONIC_BUFFER_SIZE : total_remain_len;
        sample_num = in_read / (MIDI_CHANNEL * sizeof(short));
        remain_num = sample_num;

        in_samples.samples = in;
        in_samples.num = sample_num;
        out_samples.needed_num = out_num;
        while (remain_num > 0 || out_samples.out_num > 0) {
            ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Error processing sonic data");
                return ESP_MIDI_ERR_FAIL;
            }
            if (out_samples.out_num > 0) {
                app_board_speaker_write(out_samples.samples, out_samples.out_num * sizeof(short) * MIDI_CHANNEL);
            }
            in = in + in_samples.consume_num * MIDI_CHANNEL;
            remain_num -= in_samples.consume_num;
            in_samples.num = remain_num;
            in_samples.samples = in;
        }

        total_remain_len -= in_read;
    }
    
    return ESP_MIDI_ERR_OK;
}

esp_err_t midi_player_init(void) {
    esp_err_t ret = ESP_OK;

    midi_event_queue = xQueueCreate(10, sizeof(esp_midi_parse_event_t));
    if (midi_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create midi event queue");
        return ESP_FAIL;
    }

    midi_cur_lower = instrument_info_table[midi_cur_instr_index].min_note;
    midi_cur_higher = instrument_info_table[midi_cur_instr_index].max_note;

    esp_ae_sonic_cfg_t config = {0};
    config.sample_rate = MIDI_SAMPLE_RATE;
    config.channel = MIDI_CHANNEL;
    config.bits_per_sample = 16;
    ret = esp_ae_sonic_open(&config, &sonic_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open sonic handle");
        return ret;
    }
    esp_ae_sonic_set_speed(sonic_handle, sonic_speed[sonic_speed_index]);
    sonic_buffer = malloc(SONIC_BUFFER_SIZE);
    if (sonic_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate sonic buffer");
        return ESP_FAIL;
    }

    esp_midi_sound_cfg_t sound_cfg = {
        .max_out_stream_frame = 10240,
        .note_off_resp = ESP_MIDI_SYNTH_NOTE_OFF_RESP_IGNORE,
        .out_stream_info.samplerate = MIDI_SAMPLE_RATE,
        .out_stream_info.bits = 16,
        .out_stream_info.channel = MIDI_CHANNEL,
        .out_stream_cb = esp_midi_out_data,
        .sf_lib_cfg.loader_cb = esp_midi_flash_loader_new,
        .sf_lib_cfg.delete_cb = esp_midi_flash_loader_delete,
        .sf_lib_cfg.note_on_cb = esp_midi_flash_loader_noteon,
        .sf_lib_cfg.free_data_cb = NULL,
    };
    ret = esp_midi_sound_open(&sound_cfg, &midi_handle);
    if (ret != ESP_OK) {
        vQueueDelete(midi_event_queue);
        ESP_LOGE(TAG, "Failed to open midi sound handle");
        return ret;
    }
    ret = esp_midi_sound_set_bpm(midi_handle, 120);  // this api priority is higher than the tempo in midi file
    ret = esp_midi_sound_set_time_division(midi_handle, 480);
    ret = esp_midi_sound_load_sf_lib(midi_handle);
    if (ret != ESP_OK) {
        vQueueDelete(midi_event_queue);
        ESP_LOGE(TAG, "Failed to load soundfont library");
        return ret;
    }

    ret = xTaskCreatePinnedToCore(midi_player_task, "midi_player_task", 4096, NULL, 15, NULL, 1);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create midi player task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
