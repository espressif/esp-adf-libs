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
    int task_stack;         /*!< AAC task stack size */
} aac_decoder_cfg_t;

#define AAC_DECODER_TASK_STACK (1024 * 5)

#define DEFAULT_AAC_DECODER_CONFIG() {\
    .task_stack         = AAC_DECODER_TASK_STACK,\
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
