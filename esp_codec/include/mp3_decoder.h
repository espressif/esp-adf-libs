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
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} mp3_decoder_cfg_t;

#define MP3_DECODER_TASK_STACK          (5 * 1024)
#define MP3_DECODER_TASK_CORE           (0)
#define MP3_DECODER_TASK_PRIO           (5)
#define MP3_DECODER_RINGBUFFER_SIZE     (8 * 1024)

#define DEFAULT_MP3_DECODER_CONFIG() {\
    .out_rb_size        = MP3_DECODER_RINGBUFFER_SIZE,\
    .task_stack         = MP3_DECODER_TASK_STACK,\
    .task_core          = MP3_DECODER_TASK_CORE,\
    .task_prio          = MP3_DECODER_TASK_PRIO,\
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