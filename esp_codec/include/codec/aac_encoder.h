// Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
// All rights reserved.

#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

#include "audio_element.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AAC_ENCODER_SAMPLE_RATE (44100)
#define AAC_ENCODER_CHANNELS    (2)
#define AAC_ENCODER_BITRATE     (80000)

/**
 * @brief      AAC Encoder configurations
 */
typedef struct {
    int sample_rate;               /*!< the sample rate of AAC audio*/
    int channel;                   /*!< the numble of channels of AAC audio*/
    int bitrate;                   /*!< the bitrate of AAC audio*/
    int out_rb_size;               /*!< Size of output ringbuffer */
    int task_stack;                /*!< Task stack size */
    int task_core;                 /*!< Task running in core (0 or 1) */
    int task_prio;                 /*!< Task priority (based on freeRTOS priority) */
    bool stack_in_ext;             /*!< Try to allocate stack in external memory */
} aac_encoder_cfg_t;

#define AAC_ENCODER_TASK_STACK      (10 * 1024)
#define AAC_ENCODER_TASK_CORE       (0)
#define AAC_ENCODER_TASK_PRIO       (5)
#define AAC_ENCODER_RINGBUFFER_SIZE (2 * 1024)

#define DEFAULT_AAC_ENCODER_CONFIG() {                         \
    .sample_rate              = AAC_ENCODER_SAMPLE_RATE,       \
    .channel                  = AAC_ENCODER_CHANNELS,          \
    .bitrate                  = AAC_ENCODER_BITRATE,           \
    .out_rb_size              = AAC_ENCODER_RINGBUFFER_SIZE,   \
    .task_stack               = AAC_ENCODER_TASK_STACK,        \
    .task_core                = AAC_ENCODER_TASK_CORE,         \
    .task_prio                = AAC_ENCODER_TASK_PRIO,         \
    .stack_in_ext             = true,                          \
}

/**
 * @brief      Create a handle to an Audio Element to encode incoming data using AAC format
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t aac_encoder_init(aac_encoder_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
