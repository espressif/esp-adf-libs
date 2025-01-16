// Copyright 2025 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "audio_element.h"

#define OPUS_DECODER_TASK_STACK          (30 * 1024)
#define OPUS_DECODER_TASK_PRIO           (3)
#define OPUS_DECODER_TASK_CORE           (0)
#define OPUS_DECODER_RINGBUFFER_SIZE     (2 * 1024)

/**
 * @brief  Configuration for RAW OPUS audio decoder
 */
typedef struct {
    uint32_t sample_rate;                /*!< Audio samplerate */
    uint8_t  channels;                   /*!< Audio channel */
    int      dec_frame_size;             /*!< Audio sample size (bytes) */
    bool     self_delimited;             /*!< Whether use self delimited packet
                                              Self delimited packet need an extra 1 or 2 bytes for packet size
                                              Set to `false` if encapsulated in OGG */
    bool     enable_frame_length_prefix; /*!< When set to 'true', a 2-byte frame length (in big-endian format) is added 
                                              as a prefix to each Opus frame before processing or transmission*/
    int      out_rb_size;                /*!< Size of output ringbuffer */
    int      task_stack;                 /*!< Task stack size */
    int      task_core;                  /*!< CPU core number (0 or 1) where decoder task in running */
    int      task_prio;                  /*!< Task priority (based on freeRTOS priority) */
    bool     stack_in_ext;               /*!< Try to allocate stack in external memory */
} raw_opus_dec_cfg_t;

#define RAW_OPUS_DEC_CONFIG_DEFAULT() {              \
    .sample_rate    = 16000,                         \
    .channels       = 2,                             \
    .dec_frame_size = 160,                           \
    .self_delimited = false,                         \
    .out_rb_size    = OPUS_DECODER_RINGBUFFER_SIZE,  \
    .task_stack     = OPUS_DECODER_TASK_STACK,       \
    .task_core      = OPUS_DECODER_TASK_CORE,        \
    .task_prio      = OPUS_DECODER_TASK_PRIO,        \
    .stack_in_ext   = true                           \
}

/**
 * @brief      Create an Audio Element handle to decode incoming raw opus data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t raw_opus_decoder_init(raw_opus_dec_cfg_t *config);
