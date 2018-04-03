// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MP3_DECODER_H_
#define _MP3_DECODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Mp3 Decoder configurations
 */
typedef struct {
    int task_stack;  /*!< MP3 task stack size */
} mp3_decoder_cfg_t;

#define MP3_DECODER_TASK_STACK (1024 * 4)

#define DEFAULT_MP3_DECODER_CONFIG() {\
    .task_stack         = MP3_DECODER_TASK_STACK,\
}

/**
 * @brief      Create an Audio Element handle to decode incoming MP3 data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t mp3_decoder_init(mp3_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
