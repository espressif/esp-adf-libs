// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _WAV_DECODER_H_
#define _WAV_DECODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WAV_DECODER_TASK_STACK (3072)

/**
 * brief      WAV Decoder configurations
 */
typedef struct {
    int task_stack; /*!< WAV Decover task stack size */
} wav_decoder_cfg_t;

#define DEFAULT_WAV_DECODER_CONFIG() {\
    .task_stack         = WAV_DECODER_TASK_STACK,\
}

/**
 * @brief      Create an Audio Element handle to decode incoming WAV data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t wav_decoder_init(wav_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
