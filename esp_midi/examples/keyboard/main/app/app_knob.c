/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "iot_knob.h"
#include "app_board.h"
#include "app_midi_keyboard.h"
#include "app_knob.h"

static const char *TAG = "KNOB";

static knob_handle_t knob_a = NULL;
static knob_handle_t knob_b = NULL;
/**
 * Since the internal variables `encoder_a` and `encoder_b` of the knob component directly point to the `config` structure,
 * it needs to persist until `deinit` is called. 
 */
static knob_config_t *knob_cfg_a = NULL;
static knob_config_t *knob_cfg_b = NULL;

static void knob_a_left_cb(void *arg, void *data)
{
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_KNOB,
        .value1 = 0,
        .value2 = KNOB_LEFT,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "KNOB A: KNOB_LEFT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
}

static void knob_a_right_cb(void *arg, void *data)
{
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_KNOB,
        .value1 = 0,
        .value2 = KNOB_RIGHT,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "KNOB A: KNOB_RIGHT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
}

static void knob_b_left_cb(void *arg, void *data)
{
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_KNOB,
        .value1 = 1,
        .value2 = KNOB_LEFT,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "KNOB B: KNOB_LEFT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
}

static void knob_b_right_cb(void *arg, void *data)
{
    QueueHandle_t input_queue = (QueueHandle_t)data;
    keyboard_input_t input = {
        .type = KEYBOARD_INPUT_KNOB,
        .value1 = 1,
        .value2 = KNOB_RIGHT,
    };
    xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);
    ESP_LOGD(TAG, "KNOB B: KNOB_RIGHT Count is %d", iot_knob_get_count_value((knob_handle_t)arg));
}

esp_err_t knob_init(void *input_queue)
{
    knob_cfg_a = calloc(1, sizeof(knob_config_t));
    if (knob_cfg_a == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for knob_cfg_a");
        return ESP_FAIL;
    }
    knob_cfg_a->default_direction = 0;
    knob_cfg_a->gpio_encoder_a = KNOB_LEFT_PINA;
    knob_cfg_a->gpio_encoder_b = KNOB_LEFT_PINB;
    knob_a = iot_knob_create(knob_cfg_a);
    if (knob_a == NULL) {
        ESP_LOGE(TAG, "Failed to create knob_a");
        free(knob_cfg_a);
        return ESP_FAIL;
    }
    iot_knob_register_cb(knob_a, KNOB_LEFT, knob_a_left_cb, input_queue);
    iot_knob_register_cb(knob_a, KNOB_RIGHT, knob_a_right_cb, input_queue);

    knob_cfg_b = calloc(1, sizeof(knob_config_t));
    if (knob_cfg_b == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for knob_cfg_b");
        iot_knob_delete(knob_a);
        free(knob_cfg_a);
        return ESP_FAIL;
    }
    knob_cfg_b->default_direction = 0;
    knob_cfg_b->gpio_encoder_a = KNOB_RIGHT_PINA;
    knob_cfg_b->gpio_encoder_b = KNOB_RIGHT_PINB;
    knob_b = iot_knob_create(knob_cfg_b);
    if (knob_b == NULL) {
        ESP_LOGE(TAG, "Failed to create knob_b");
        iot_knob_delete(knob_a);
        free(knob_cfg_a);
        free(knob_cfg_b);
        return ESP_FAIL;
    }
    iot_knob_register_cb(knob_b, KNOB_LEFT, knob_b_left_cb, input_queue);
    iot_knob_register_cb(knob_b, KNOB_RIGHT, knob_b_right_cb, input_queue);

    return ESP_OK;
}

esp_err_t knob_deinit(void)
{
    esp_err_t ret = ESP_OK;

    if (knob_a != NULL) {
        ret |= iot_knob_delete(knob_a);
        free(knob_cfg_a);
        knob_a = NULL;
        knob_cfg_a = NULL;
    }
    
    if (knob_b != NULL) {
        ret |= iot_knob_delete(knob_b);
        free(knob_cfg_b);
        knob_b = NULL;
        knob_cfg_b = NULL;
    }
    
    return ret;
}
