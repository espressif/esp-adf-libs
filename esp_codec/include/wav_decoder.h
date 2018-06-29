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

/**
 * brief      WAV Decoder configurations
 */
typedef struct {
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} wav_decoder_cfg_t;

#define WAV_DECODER_TASK_STACK          (3 * 1024)
#define WAV_DECODER_TASK_CORE           (0)
#define WAV_DECODER_TASK_PRIO           (5)
#define WAV_DECODER_RINGBUFFER_SIZE     (8 * 1024)

#define DEFAULT_WAV_DECODER_CONFIG() {\
    .out_rb_size        = WAV_DECODER_RINGBUFFER_SIZE,\
    .task_stack         = WAV_DECODER_TASK_STACK,\
    .task_core          = WAV_DECODER_TASK_CORE,\
    .task_prio          = WAV_DECODER_TASK_PRIO,\
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
