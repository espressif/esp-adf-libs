// Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
// All rights reserved.

#ifndef ESP_AAC_ENC_H
#define ESP_AAC_ENC_H

#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sample_rate;
    int channel;
    int bit;
    int bit_rate;
    int adts_used;
} esp_aac_enc_config_t;

#define DEFAULT_ESP_AAC_ENC_CONFIG() {              \
    .sample_rate = 44100,                           \
    .channel = 2,                                   \
    .bit = 16,                                      \
    .bit_rate = 80000,                              \
    .adts_used = 1,                                 \
}

/**
 * @brief       Create an AAC encoder handle, set user config info to encoder handle
 * 
 * @param[in]   cfg          The configuration information
 * @param[out]  out_handle   The AAC encoder handle
 * 
 * @return      esp_err_t
 *              - ESP_OK: on success
 *              - ESP_FAIL: error occurs
 */
esp_err_t esp_aac_enc_open(esp_aac_enc_config_t *cfg, void **out_handle);

/**
 * @brief       Encode pcm to AAC
 * 
 * @param[in]   in_handle      The AAC encoder handle
 * @param[in]   in_buf         The input pcm data buf
 * @param[in]   in_byte_len    The input pcm data length
 * @param[out]  out_buf        The output aac data buf
 * @param[out]  out_byte_len   The output aac data length
 * 
 * @return      esp_err_t
 *              - ESP_OK: on success
 *              - ESP_FAIL: error occurs  
 */
esp_err_t esp_aac_enc_process(void *in_handle, uint8_t *in_buf, int in_byte_len, uint8_t *out_buf, int *out_byte_len);

/**
 * @brief      Deinitialize AAC encoder handle 
 * 
 * @param[in]  in_handle    The AAC encoder handle     
 * 
 * @return     esp_err_t
 *             - ESP_OK: on success
 *             - ESP_FAIL: error occurs 
 */
esp_err_t esp_aac_enc_close(void *in_handle);

#ifdef __cplusplus
}
#endif

#endif
