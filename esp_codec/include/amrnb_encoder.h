// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AMRNB_ENCODER_H_
#define _AMRNB_ENCODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif
#define AMRNB_ENCODER_TASK_STACK          (15 * 1024)
#define AMRNB_ENCODER_TASK_CORE           (0)
#define AMRNB_ENCODER_TASK_PRIO           (5)
#define AMRNB_ENCODER_RINGBUFFER_SIZE     (8 * 1024)

#define DEFAULT_AMRNB_ENCODER_CONFIG() {\
        .out_rb_size        = AMRNB_ENCODER_RINGBUFFER_SIZE,\
        .task_stack         = AMRNB_ENCODER_TASK_STACK,\
        .task_core          = AMRNB_ENCODER_TASK_CORE,\
        .task_prio          = AMRNB_ENCODER_TASK_PRIO,\
    }

/**
 * @brief      AMRNB Encoder configurations
 */
typedef struct {
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} amrnb_encoder_cfg_t;

/**
 * @brief      Create an Audio Element handle to encode incoming AMRNB data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t amrnb_encoder_init(amrnb_encoder_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
