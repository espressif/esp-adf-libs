// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _OPUS_ENCODER_H_
#define _OPUS_ENCODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPUS_ENCODER_TASK_STACK          (40 * 1024)
#define OPUS_ENCODER_TASK_CORE           (0)
#define OPUS_ENCODER_TASK_PRIO           (3)
#define OPUS_ENCODER_RINGBUFFER_SIZE     (2 * 1024)
#define OPUS_ENCODER_SAMPLE_RATE         (16000)
#define OPUS_ENCODER_CHANNELS            (1)
#define OPUS_ENCODER_BITRATE             (64000)
#define OPUS_ENCODER_COMPLEXITY          (10)

#define DEFAULT_OPUS_ENCODER_CONFIG() {                     \
        .sample_rate        = OPUS_ENCODER_SAMPLE_RATE,     \
        .channel            = OPUS_ENCODER_CHANNELS,        \
        .bitrate            = OPUS_ENCODER_BITRATE,         \
        .complexity         = OPUS_ENCODER_COMPLEXITY,      \
        .out_rb_size        = OPUS_ENCODER_RINGBUFFER_SIZE, \
        .task_stack         = OPUS_ENCODER_TASK_STACK,      \
        .task_core          = OPUS_ENCODER_TASK_CORE,       \
        .task_prio          = OPUS_ENCODER_TASK_PRIO,       \
    }

/**
 * @brief     OPUS Encoder configurations
 */
typedef struct {
    int                     sample_rate;    /*!< the sample rate of OPUS audio*/
    int                     channel;        /*!< the numble of channels of OPUS audio*/
    int                     bitrate;        /*!< the rate of bit of OPUS audio. unit:bps*/
    int                     complexity;     /*!< Indicates the complexity of OPUS encoding. 0 is lowest. 10 is higest.*/
    int                     out_rb_size;    /*!< Size of output ringbuffer */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} opus_encoder_cfg_t;

/**
 * @brief      Create an Audio Element handle to encode incoming opus data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t encoder_opus_init(opus_encoder_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
