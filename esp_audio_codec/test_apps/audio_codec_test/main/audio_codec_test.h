/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "esp_audio_types.h"
#include "esp_audio_simple_dec.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Audio codec test module name
 */
#define CODEC_TEST_MODULE_NAME "[esp_audio_codec]"

/**
 * @brief  Basic audio information
 */
typedef struct {
    uint8_t     channel;          /*!< Audio channel */
    uint8_t     bits_per_sample;  /*!< Audio bits per sample */
    int         sample_rate;      /*!< Sample rate */
    const void *spec_info;        /*!< Specified information for certain audio codec */
    int         spec_info_size;   /*!< Specified information length */
} audio_info_t;

/**
 * @brief  Read data callback
 */
typedef int (*codec_read_cb)(uint8_t *data, int size);

/**
 * @brief  Write data callback
 */
typedef int (*codec_write_cb)(uint8_t *data, int size);

/**
 * @brief  Codec test configuration
 */
typedef struct {
    codec_read_cb  read;   /*!< Callback to read codec input data */
    codec_write_cb write;  /*!< Callback to write codec output data */
} audio_codec_test_cfg_t;

/**
 * @brief  Generate test PCM audio data
 *
 * @param[in]  info  Audio information
 * @param[in]  data  PCM buffer to be wrote
 * @param[in]  size  Maximum size can be written
 *
 * @return
 *       Actually size being written
 */
int audio_codec_gen_pcm(audio_info_t *info, uint8_t *data, int size);

/**
 * @brief  Do encoder test
 *
 * @param[in]  type  Audio codec type
 * @param[in]  cfg   Configuration for input and output data callback
 * @param[in]  info  Audio information
 *
 * @return
 *       - 0       On Success
 *       - Others  Fail to do encoder test
 */
int audio_encoder_test(esp_audio_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info);

/**
 * @brief  Do decoder test
 *
 * @param[in]  type  Audio codec type
 * @param[in]  cfg   Configuration for input and output data callback
 * @param[in]  info  Audio information
 *
 * @return
 *       - 0       On Success
 *       - Others  Fail to do decoder test
 */
int audio_decoder_test(esp_audio_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info);

/**
 * @brief  Do simple decoder test
 *
 * @param[in]   type  Audio simple decoder type
 * @param[in]   cfg   Configuration for input and output data callback
 * @param[out]  info  Audio information
 *
 * @return
 *       - 0       On Success
 *       - Others  Fail to do simple decoder test
 */
int audio_simple_decoder_test(esp_audio_simple_dec_type_t type, audio_codec_test_cfg_t *cfg, audio_info_t *info);

/**
 * @brief  Do simple decoder test, get input data from file
 *
 * @param[in]   file_name  Filename to test
 * @param[in]   writer     Write callback for output pcm data
 * @param[out]  info       Audio information
 *
 * @return
 *       - 0       On Success
 *       - Others  Fail to do simple decoder test
 */
int audio_simple_decoder_test_file(char *in_file, char *out_file, audio_info_t *info);

#ifdef __cplusplus
}
#endif
