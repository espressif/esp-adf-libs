/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2023 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "esp_audio_enc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  G711 Encoder configurations
 */
typedef struct {
    int sample_rate;      /*!< The sample rate of audio */
    int channel;          /*!< The channel num of audio */
    int bits_per_sample;  /*!< Supported bits per sample: 16 bit */
    int frame_duration;   /*!< The frame duration of audio, unit: ms */
} esp_g711_enc_config_t;

#define ESP_G711_ENC_CONFIG_DEFAULT() {            \
    .sample_rate       = ESP_AUDIO_SAMPLE_RATE_8K, \
    .channel           = ESP_AUDIO_MONO,           \
    .bits_per_sample   = ESP_AUDIO_BIT16,          \
    .frame_duration    = 10,                       \
}

/**
 * @brief  Register G711 a-LAW encoder
 *
 * @note  If user want to use encoder through encoder common API, need register it firstly.
 *        Register can use either of following methods:
 *          1: Manually call `esp_g711a_enc_register`.
 *          2: Call `esp_audio_enc_register_default` and use menuconfig to enable it.
 *        When user want to use G711 a-LAW encoder only and not manage it by common part, no need to call this API,
 *        Directly call `esp_g711a_enc_open`, `esp_g711_enc_process`, `esp_g711_enc_close` instead.
 *        To keep code reuse, G711 a-LAW and u-LAW only open API different, other API are all same
 *
 * @return
 *       - ESP_AUDIO_ERR_OK        On success
 *       - ESP_AUDIO_ERR_MEM_LACK  Fail to allocate memory
 */
esp_audio_err_t esp_g711a_enc_register(void);

/**
 * @brief  Register G711 u-LAW encoder
 *
 * @note   See notes above for `esp_g711a_enc_register`
 *
 * @return
 *       - ESP_AUDIO_ERR_OK        On success
 *       - ESP_AUDIO_ERR_MEM_LACK  Fail to allocate memory
 */
esp_audio_err_t esp_g711u_enc_register(void);

/**
 * @brief  Query frame information with encoder configuration
 *
 * @param[in]   cfg         G711 encoder configuration
 * @param[out]  frame_info  The structure of frame information
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711_enc_get_frame_info_by_cfg(void *cfg, esp_audio_enc_frame_info_t *frame_info);

/**
 * @brief  Create G711 a-LAW encoder handle through encoder configuration
 *
 * @param[in]   cfg     G711 encoder configuration
 * @param[in]   cfg_sz  Size of "esp_g711_enc_config_t"
 * @param[out]  enc_hd  The G711 encoder handle. If G711 encoder handle allocation failed, will be set to NULL.
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_FAIL               Encoder initialize failed
 *       - ESP_AUDIO_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711a_enc_open(void *cfg, uint32_t cfg_sz, void **enc_hd);

/**
 * @brief  Create G711 u-LAW encoder handle through encoder configuration
 *
 * @param[in]   cfg     G711 encoder configuration
 * @param[in]   cfg_sz  Size of "esp_g711_enc_config_t"
 * @param[out]  enc_hd  The G711 encoder handle. If G711 encoder handle allocation failed, will be set to NULL.
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_FAIL               Encoder initialize failed
 *       - ESP_AUDIO_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711u_enc_open(void *cfg, uint32_t cfg_sz, void **enc_hd);

/**
 * @brief  Get the input PCM data length and recommended output buffer length needed by encoding one frame
 *
 * @param[in]   enc_hd    The G711 a-LAW/u-LAW encoder handle
 * @param[out]  in_size   The input frame size which is one sample size.
 *                        If user want to encode more samples at once, 
 *                        the input length should be set as a multiple of "in_size".
 * @param[out]  out_size  The output frame size which is one sample size.
 *                        If user want to encode more samples at once, 
 *                        the output length should be set as a multiple of 'out_size'.
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711_enc_get_frame_size(void *enc_hd, int *in_size, int *out_size);

/**
 * @brief  Encode one or multi G711 a-LAW frame which the frame num is dependent on input data length
 *
 * @param[in]      enc_hd     The G711 a-LAW/u-LAW encoder handle
 * @param[in]      in_frame   Pointer to input data frame
 * @param[in,out]  out_frame  Pointer to output data frame
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_DATA_LACK          Not enough input data to encode one or several frames
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711_enc_process(void *enc_hd, esp_audio_enc_in_frame_t *in_frame,
                                     esp_audio_enc_out_frame_t *out_frame);

/**
 * @brief  Get G711 a-LAW/u-LAW encoder information from encoder handle
 *
 * @param[in]  enc_hd    The G711 encoder handle
 * @param[in]  enc_info  The G711 encoder information
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_g711_enc_get_info(void *enc_hd, esp_audio_enc_info_t *enc_info);

/**
 * @brief  Deinitialize G711 a-LAW/u-LAW encoder
 *
 * @param[in]  enc_hd  The G711 encoder handle
 */
void esp_g711_enc_close(void *enc_hd);

#ifdef __cplusplus
}
#endif
