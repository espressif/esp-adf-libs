// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUTO_AMR_DEC_H_
#define _AUTO_AMR_DEC_H_

#include "audio_type_def.h"

/**
 * @brief      Open an Audio Element type data
 *
 * @param      el   The audio element handle
 *
 * @return    
 *             ESP_OK
 *             ESP_FAIL
 */
esp_err_t amr_decoder_open(audio_element_handle_t el);

/**
 * @brief      Close an Audio Element type data
 *
 * @param      el   The audio element handle
 *
 * @return    
 *             ESP_OK
 *             ESP_FAIL
 */
esp_err_t amr_decoder_close(audio_element_handle_t el);

/**
 * @brief      Do music data to decode
 *
 * @param      el   The audio element handle
 *
 * @return    
 *             ESP_OK
 *             ESP_FAIL
 */
esp_codec_err_t amr_decoder_process(audio_element_handle_t el);

#endif
