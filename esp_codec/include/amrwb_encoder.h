// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AMRWB_ENCODER_H_
#define _AMRWB_ENCODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     AMRWB Encoder configurations
 */
typedef struct {
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} amrwb_encoder_cfg_t;

#define AMRWB_ENCODER_TASK_STACK          (15 * 1024)
#define AMRWB_ENCODER_TASK_CORE           (0)
#define AMRWB_ENCODER_TASK_PRIO           (5)
#define AMRWB_ENCODER_RINGBUFFER_SIZE     (8 * 1024)

#define DEFAULT_amrwb_ENCODER_CONFIG() {\
        .out_rb_size        = AMRWB_ENCODER_RINGBUFFER_SIZE,\
        .task_stack         = AMRWB_ENCODER_TASK_STACK,\
        .task_core          = AMRWB_ENCODER_TASK_CORE,\
        .task_prio          = AMRWB_ENCODER_TASK_PRIO,\
    }


/**
 * @brief      Create an Audio Element handle to encode incoming amrwb data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t amrwb_encoder_init(amrwb_encoder_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
