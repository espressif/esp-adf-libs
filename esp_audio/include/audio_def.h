// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_DEF_H_
#define _AUDIO_DEF_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"

#define ESP_ERR_AUDIO_BASE  0x10000         /*!< Starting number of ESP audio error codes */

typedef enum audio_err_t {
    ESP_ERR_AUDIO_NO_ERROR                  = ESP_OK,
    ESP_ERR_AUDIO_FAIL                      = ESP_FAIL,

    /* esp audio errors */
    ESP_ERR_AUDIO_NO_INPUT_STREAM           = ESP_ERR_AUDIO_BASE + 1,  // 0x10001
    ESP_ERR_AUDIO_NO_OUTPUT_STREAM          = ESP_ERR_AUDIO_BASE + 2,
    ESP_ERR_AUDIO_NO_CODEC                  = ESP_ERR_AUDIO_BASE + 3,
    ESP_ERR_AUDIO_HAL_FAIL                  = ESP_ERR_AUDIO_BASE + 4,
    ESP_ERR_AUDIO_MEMORY_LACK               = ESP_ERR_AUDIO_BASE + 5,
    ESP_ERR_AUDIO_INVALID_URI               = ESP_ERR_AUDIO_BASE + 6,
    ESP_ERR_AUDIO_INVALID_PATH              = ESP_ERR_AUDIO_BASE + 7,
    ESP_ERR_AUDIO_INVALID_PARAMETER         = ESP_ERR_AUDIO_BASE + 8,
    ESP_ERR_AUDIO_NOT_READY                 = ESP_ERR_AUDIO_BASE + 9,
    ESP_ERR_AUDIO_NOT_SUPPORT               = ESP_ERR_AUDIO_BASE + 10,
    ESP_ERR_AUDIO_TIMEOUT                   = ESP_ERR_AUDIO_BASE + 11,
    ESP_ERR_AUDIO_ALREADY_EXISTS            = ESP_ERR_AUDIO_BASE + 12,
    ESP_ERR_AUDIO_UNKNOWN                   = ESP_ERR_AUDIO_BASE + 13,


    ESP_ERR_AUDIO_OPEN                      = ESP_ERR_AUDIO_BASE + 0x100,// 0x10100
    ESP_ERR_AUDIO_INPUT                     = ESP_ERR_AUDIO_BASE + 0x101,
    ESP_ERR_AUDIO_PROCESS                   = ESP_ERR_AUDIO_BASE + 0x102,
    ESP_ERR_AUDIO_OUTPUT                    = ESP_ERR_AUDIO_BASE + 0x103,
    ESP_ERR_AUDIO_CLOSE                     = ESP_ERR_AUDIO_BASE + 0x104,

} audio_err_t;

typedef enum esp_audio_status_t {
    AUDIO_STATUS_UNKNOWN = 0,
    AUDIO_STATUS_RUNNING = 1,
    AUDIO_STATUS_PAUSED = 2,
    AUDIO_STATUS_STOPED = 3,
    AUDIO_STATUS_ERROR = 4,
} esp_audio_status_t;

typedef enum {
    TERMINATION_TYPE_NOW = 0,   /*!<  Audio operation will be terminated immediately */
    TERMINATION_TYPE_DONE = 1,  /*!<  Audio operation will be stopped untill is finished */
    TERMINATION_TYPE_MAX,
} audio_termination_type_t;

typedef struct {
    esp_audio_status_t status;      /*!< Status of esp_audio */
    audio_err_t err_msg;            /*!< Status is `AUDIO_STATUS_ERROR`,err_msg will be setup */
} esp_audio_state_t;

#endif