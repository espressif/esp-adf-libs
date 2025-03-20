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
#include <stdbool.h>
#include "esp_audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Audio simple decoder handle
 *
 * @note  Most audio decoder can only process frame data.
 *        To support playback simply, simple decoder is imported.
 *        In simple playback scenario, like play MP3, AAC, WAV etc.
 *        User need only provide the original audio data, simple decoder will filter out frame data,
 *        send frame data to decoder, then return decoded data to user directly.
 */
typedef void *esp_audio_simple_dec_handle_t;

/**
 * @brief  Audio simple decoder type
 */
typedef enum {
    ESP_AUDIO_SIMPLE_DEC_TYPE_NONE       = 0,                                           /*!< Invalid simple decoder type */
    ESP_AUDIO_SIMPLE_DEC_TYPE_AAC        = ESP_AUDIO_TYPE_AAC,                          /*!< Simple decoder for AAC */
    ESP_AUDIO_SIMPLE_DEC_TYPE_MP3        = ESP_AUDIO_TYPE_MP3,                          /*!< Simple decoder for MP3 */
    ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB      = ESP_AUDIO_TYPE_AMRNB,                        /*!< Simple decoder for AMR-NB */
    ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB      = ESP_AUDIO_TYPE_AMRWB,                        /*!< Simple decoder for AMR-WB */
    ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC       = ESP_AUDIO_TYPE_FLAC,                         /*!< Simple decoder for FLAC */
    ESP_AUDIO_SIMPLE_DEC_TYPE_WAV        = ESP_AUDIO_FOURCC_TO_INT('W', 'A', 'V', ' '), /*!< Simple decoder for WAV */
    ESP_AUDIO_SIMPLE_DEC_TYPE_M4A        = ESP_AUDIO_FOURCC_TO_INT('M', '4', 'A', 'A'), /*!< Simple decoder for M4A */
    ESP_AUDIO_SIMPLE_DEC_TYPE_TS         = ESP_AUDIO_FOURCC_TO_INT('M', '2', 'T', 'S'), /*!< Simple decoder for TS */
    ESP_AUDIO_SIMPLE_DEC_TYPE_RAW_OPUS   = ESP_AUDIO_TYPE_OPUS,                         /*!< Simple decoder for OPUS (raw data with no extra header) */
    ESP_AUDIO_SIMPLE_DEC_TYPE_G711A      = ESP_AUDIO_TYPE_G711A,                        /*!< Simple decoder for G711A */
    ESP_AUDIO_SIMPLE_DEC_TYPE_G711U      = ESP_AUDIO_TYPE_G711U,                        /*!< Simple decoder for G711U */
    ESP_AUDIO_SIMPLE_DEC_TYPE_PCM        = ESP_AUDIO_TYPE_PCM,                          /*!< Simple decoder for PCM */
    ESP_AUDIO_SIMPLE_DEC_TYPE_ADPCM      = ESP_AUDIO_TYPE_ADPCM,                        /*!< Simple decoder for IMA-ADPCM */                                       /*!< Customized simple decoder type start */
    ESP_AUDIO_SIMPLE_DEC_TYPE_CUSTOM     = 0x1,                                         /*!< Customized simple decoder type start */
    ESP_AUDIO_SIMPLE_DEC_TYPE_CUSTOM_MAX = 0x10,                                        /*!< Customized simple decoder type end */
} esp_audio_simple_dec_type_t;

/**
 * @brief  Audio simple decoder configuration
 */
typedef struct {
    esp_audio_simple_dec_type_t dec_type; /*!< Simple decoder type */
    void                       *dec_cfg;  /*!< Decoder specified configuration data (check decoder header file for details)
                                              Ex: When use internal aac decoder can refer `esp_aac_dec.h`
                                                  If user want to enable AAC_PLUS can set it to pointer to
                                                  `esp_aac_dec_cfg_t` */
    int                         cfg_size; /*!< Decoder specified configuration size */
} esp_audio_simple_dec_cfg_t;

/**
 * @brief  Input data information to be decoded
 */
typedef struct {
    uint8_t *buffer;   /*!< Raw data to be decoded */
    uint32_t len;      /*!< Raw data length */
    bool     eos;      /*!< Whether end of stream (some parser like FLAC need eos flag to flush cached data) */
    uint32_t consumed; /*!< Consumed size of input raw data */
} esp_audio_simple_dec_raw_t;

/**
 * @brief  Audio simple decoder output frame
 */
typedef struct {
    uint8_t *buffer;       /*!< Output buffer to hold decoded PCM data */
    uint32_t len;          /*!< Output buffer size */
    uint32_t needed_size;  /*!< Set when output buffer size not enough (output) */
    uint32_t decoded_size; /*!< Decoded PCM data size (output) */
} esp_audio_simple_dec_out_t;

/**
 * @brief  Audio simple decoder information
 */
typedef struct {
    uint32_t sample_rate;     /*!< Audio sample rate */
    uint8_t  bits_per_sample; /*!< Bits per sample */
    uint8_t  channel;         /*!< Audio channel*/
    uint32_t bitrate;         /*!< Audio bitrate (bits per second) */
    uint32_t frame_size;      /*!< Audio frame size */
} esp_audio_simple_dec_info_t;

/**
 * @brief  Query whether the simple decoder type is supported
 *
 * @param[in]  dec_type  Simple decoder type
 *
 * @return
 *       - ESP_AUDIO_ERR_OK           On success
 *       - ESP_AUDIO_ERR_NOT_SUPPORT  Not support the audio type
 */
esp_audio_err_t esp_audio_simple_check_audio_type(esp_audio_simple_dec_type_t dec_type);

/**
 * @brief  Open audio simple decoder
 *
 * @param[in]   cfg         Decoder configuration
 * @param[out]  dec_handle  Pointer to output simple decoder handle
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 Open decoder success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid input argument
 *       - ESP_AUDIO_ERR_MEM_LACK           No memory for decoder instance
 *       - ESP_AUDIO_ERR_NOT_SUPPORT        Not supported parse type
 */
esp_audio_err_t esp_audio_simple_dec_open(esp_audio_simple_dec_cfg_t *cfg, esp_audio_simple_dec_handle_t *dec_handle);

/**
 * @brief  Decode input raw data to PCM data
 *
 * @param[in]      dec_handle  Simple decoder handle
 * @param[in,out]  raw         Raw data to be decoded
 * @param[in,out]  frame       Decoded PCM frame
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 Decode success or data feed into cached buffer
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid input argument
 *       - ESP_AUDIO_ERR_BUFF_NOT_ENOUGH    Output frame buffer not enough need reallocated and try again
 *       - ESP_AUDIO_ERR_NOT_SUPPORT        Decoder not supported
 */
esp_audio_err_t esp_audio_simple_dec_process(esp_audio_simple_dec_handle_t dec_handle, esp_audio_simple_dec_raw_t *raw,
                                             esp_audio_simple_dec_out_t *frame);

/**
 * @brief  Get audio simple decoder information
 *
 * @note  Audio decoder information contain 2 parts:
 *        1. Basic information which can be used for play (`sample_rate`, `channel`, `bits`)
 *           This information is ready when decoder decode output PCM data
 *           When `esp_audio_simple_dec_process` return `decoded_size` have value
 *           User no need frequently call this API to get it for it almost unchanged
 *        2. Frame related information such as (`frame_size`) which may differ for each frame
 *           User can use it to estimate duration and play position.
 *
 * @param[in]   dec_handle  Simple decoder handle
 * @param[out]  info        Decoder information
 *
 * @return
 *       - ESP_AUDIO_ERR_OK                 Open decoder success
 *       - ESP_AUDIO_ERR_INVALID_PARAMETER  Invalid input argument
 *       - ESP_AUDIO_ERR_NOT_FOUND          Decode information not ready yet
 */
esp_audio_err_t esp_audio_simple_dec_get_info(esp_audio_simple_dec_handle_t dec_handle, esp_audio_simple_dec_info_t *info);

/**
 * @brief  Close audio simple decoder
 *
 * @param[in]  dec_handle  Decoder handle
 *
 */
void esp_audio_simple_dec_close(esp_audio_simple_dec_handle_t dec_handle);

#ifdef __cplusplus
}
#endif
