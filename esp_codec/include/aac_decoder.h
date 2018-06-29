// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AAC_DECODER_H_
#define _AAC_DECODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      AAC Decoder configurations
 */
typedef struct {
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} aac_decoder_cfg_t;

#define AAC_DECODER_TASK_STACK          (5 * 1024)
#define AAC_DECODER_TASK_CORE           (0)
#define AAC_DECODER_TASK_PRIO           (5)
#define AAC_DECODER_RINGBUFFER_SIZE     (8 * 1024)

#define DEFAULT_AAC_DECODER_CONFIG() {\
    .out_rb_size        = AAC_DECODER_RINGBUFFER_SIZE,\
    .task_stack         = AAC_DECODER_TASK_STACK,\
    .task_core          = AAC_DECODER_TASK_CORE,\
    .task_prio          = AAC_DECODER_TASK_PRIO,\
}

/**
 * @brief      Create an Audio Element handle to decode incoming AAC data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t aac_decoder_init(aac_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
