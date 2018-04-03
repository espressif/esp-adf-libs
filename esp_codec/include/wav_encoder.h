// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _WAV_ENCODER_H_
#define _WAV_ENCODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      WAV Encoder configurations
 */
typedef struct {
    int task_stack; /*!< WAV Encoder task stack size */
} wav_encoder_cfg_t;

#define WAV_ENCODER_TASK_STACK (3072)

#define DEFAULT_WAV_ENCODER_CONFIG() {\
    .task_stack         = WAV_ENCODER_TASK_STACK,\
}

/**
 * @brief      Create a handle to an Audio Element to encode incoming data using WAV format
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t wav_encoder_init(wav_encoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
