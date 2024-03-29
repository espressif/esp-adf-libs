// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MP3_DECODER_H_
#define _MP3_DECODER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"
#include "esp_id3_parse.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MP3_DECODER_TASK_STACK_SIZE     (5 * 1024)
#define MP3_DECODER_TASK_CORE           (0)
#define MP3_DECODER_TASK_PRIO           (5)
#define MP3_DECODER_RINGBUFFER_SIZE     (2 * 1024)

#define DEFAULT_MP3_DECODER_CONFIG() {                  \
    .out_rb_size        = MP3_DECODER_RINGBUFFER_SIZE,  \
    .task_stack         = MP3_DECODER_TASK_STACK_SIZE,  \
    .task_core          = MP3_DECODER_TASK_CORE,        \
    .task_prio          = MP3_DECODER_TASK_PRIO,        \
    .stack_in_ext       = true,                         \
    .id3_parse_enable   = false,                        \
}

/**
 * @brief      Mp3 Decoder configuration
 */
typedef struct {
    int                     out_rb_size;       /*!< Size of output ringbuffer */
    int                     task_stack;        /*!< Task stack size */
    int                     task_core;         /*!< CPU core number (0 or 1) where decoder task in running */
    int                     task_prio;         /*!< Task priority (based on freeRTOS priority) */
    bool                    stack_in_ext;      /*!< Try to allocate stack in external memory */
    bool                    id3_parse_enable;  /*!< True: parse ID3. False: Don't parse ID3 */
} mp3_decoder_cfg_t;

/**
 * @brief      Create an Audio Element handle to decode incoming MP3 data
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t mp3_decoder_init(mp3_decoder_cfg_t *config);

/**
 * @brief      Get ID3 information
 *
 * @param      self         The audio element handle
 * 
 * @return     esp_id3_info_t: success
 *             NULL: ID3 is not exist or memory aloocation failed.
 */
const esp_id3_info_t* mp3_decoder_get_id3_info(audio_element_handle_t self);

#ifdef __cplusplus
}
#endif

#endif
