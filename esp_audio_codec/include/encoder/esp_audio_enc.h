/**
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include <stdint.h>
#include "esp_audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Audio encoder infomation
 */
typedef struct {
    uint32_t       sample_rate;      /*!< The sample rate of audio */
    uint8_t        channel;          /*!< The channel number of audio */
    uint8_t        bits_per_sample;  /*!< The bits per sample of audio */
    uint32_t       bitrate;          /*!< The bit rate of audio */
    const uint8_t *codec_spec_info;  /*!< Codec specified information, decoder need this information to be initialized.
                                          It's valid before call `esp_audio_enc_close`. */
    uint32_t       spec_info_len;    /*!< Length of codec specified information */
} esp_audio_enc_info_t;

/**
 * @brief  Audio encoder input frame structure
 */
typedef struct {
    uint8_t *buffer; /*!< Input data buffer which user can allocate times of input frame size */
    uint32_t len;    /*!< It is an input parameter and is one or several times of input frame size,
                          which is get from 'esp_xxx_enc_get_frame_size'. */
} esp_audio_enc_in_frame_t;

/**
 * @brief  Audio encoder output frame structure
 */
typedef struct {
    uint8_t *buffer;        /*!< Output data buffer which user can allocate times of output frame size */
    uint32_t len;           /*!< It is an input parameter and is one or several times of output frame size,
                                 which is get from 'esp_xxx_enc_get_frame_size'. */
    uint32_t encoded_bytes; /*!< It is an output parameter which means encoded data length */
    uint64_t pts;           /*!< Presentation time stamp(PTS) calculated from accumulated input raw frame unit ms */
} esp_audio_enc_out_frame_t;

/**
 * @brief  Encoder configuration
 */
typedef struct {
    esp_audio_type_t type;   /*!< Audio encoder type which from 'esp_audio_type_t'. */
    void            *cfg;    /*!< Audio encoder configuration. For example, if choose AAC encoder,
                               user need to config 'esp_aac_enc_config_t' and set the pointer
                               of this configuration to 'cfg'. */
    uint32_t         cfg_sz; /*!< Size of "cfg". For example, if choose AAC encoder, the 'cfg_sz'
                               is sizeof 'esp_aac_enc_config_t'*/
} esp_audio_enc_config_t;

/**
 * @brief  Handle for audio encoder instance
 */
typedef void *esp_audio_enc_handle_t;

/**
 * @brief  Create encoder handle through encoder configuration
 *
 * @param[in]   config  Audio encoder configuration
 * @param[out]  enc_hd  The encoder handle. If encoder handle allocation failed, will be set to NULL.
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_FAIL               Encoder initialize failed
 *       - ESP_AUDIO_ERR_MEM_LACK           Fail to allocate memory
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_audio_enc_open(esp_audio_enc_config_t *config, esp_audio_enc_handle_t *enc_hd);

/**
 * @brief  Get audio encoder information from encoder handle
 *
 * @param[in]  enc_hd    The encoder handle
 * @param[in]  enc_info  The encoder information
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_audio_enc_get_info(esp_audio_enc_handle_t enc_hd, esp_audio_enc_info_t *enc_info);

/**
 * @brief  Get the input PCM data length and recommended output buffer length needed by encoding one frame
 *
 * @note  As for PCM and G711 encoder, the 'in_size' and 'out_size' is one sample size.
 *
 * @param[in]   enc_hd    The audio encoder handle
 * @param[out]  in_size   The input frame size
 * @param[out]  out_size  The output frame size
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_audio_enc_get_frame_size(esp_audio_enc_handle_t enc_hd, int *in_size, int *out_size);

/**
 * @brief  Encode one or multi audio frame which the frame num is dependent on input data length
 *
 * @param[in]      enc_hd     The audio encoder handle
 * @param[in]      in_frame   Pointer to input data frame
 * @param[in,out]  out_frame  Pointer to output data frame
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 On success
 *       - ESP_AUDIO_ERR_FAIL               Encode error
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid parameter
 */
esp_audio_err_t esp_audio_enc_process(esp_audio_enc_handle_t enc_hd, esp_audio_enc_in_frame_t *in_frame,
                                      esp_audio_enc_out_frame_t *out_frame);

/**
 * @brief  Close an encoder handle
 *
 * @param[in]  enc_hd  The encoder handle
 */
void esp_audio_enc_close(esp_audio_enc_handle_t enc_hd);

#ifdef __cplusplus
}
#endif
