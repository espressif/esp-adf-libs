/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "esp_midi_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @file
 *      This file aims to create a scheme for parsing SoundFont libraries. The entire parsing process is divided into two core parts.
 *      Firstly, it focuses on parsing sf files, which mainly involves the processing and analysis of sound data.
 *      Secondly, based on the parsing results, this file further constructs an index for the SoundFont library to facilitate efficient retrieval and management of sound resources.
 */

/**
 * @brief  MIDI SoundFont handle type
 *         This is a pointer to void, used as a handle for MIDI SoundFont parsing.
 */
typedef void *esp_midi_sf_parse_handle_t;

/**
 * @brief  Read callback function type
 *         This defines a function pointer type fread_cb_t.
 *         The function accepts three parameters: a buffer (buf) to read data into, the size of the buffer (size),
 *         and a context (ctx), which is a user-defined pointer that can be used to pass additional information to the function.
 */
typedef esp_midi_err_t (*fread_cb_t)(uint8_t *buf, uint32_t size, void *ctx);

/**
 * @brief  Seek callback function type
 *         This defines a function pointer type fseek_cb_t.
 *         The function accepts two parameters: a size representing the position to seek to in the stream,
 *         and a context (ctx), which is a user-defined pointer.
 */
typedef esp_midi_err_t (*fseek_cb_t)(uint32_t size, void *ctx);

/**
 * @brief  Configuration information structure for parsing SoundFont
 */
typedef struct {
    fread_cb_t  read;              /*<! Read stream callback function */
    fseek_cb_t  seek;              /*<! Seek stream callback function */
    void       *ctx;               /*<! A void pointer used as the context for the read and seek callback functions,
                                        allowing users to pass additional information or state to these functions. */
    bool        need_load_sample;  /*<! Indicates whether samples need to be loaded from the SoundFont file. */
} esp_midi_sf_parse_cfg_t;

/**
 * @brief  MIDI SoundFont stream sample structure
 */
typedef struct _sample {
    char      name[20];    /*<! Sample name */
    uint32_t  start;       /*<! Start offset of the sample within the sample area */
    uint32_t  end;         /*<! Offset from the start to the end of the sample,
                                which is the last point of the sample. In the SF specification, this is the first point after the loop, adjusted during loading/saving */
    uint32_t  loopstart;   /*<! Sample number offset from the start to the beginning of the loop */
    uint32_t  loopend;     /*<! Sample number offset from the start to the end of the loop, marking the first point after the loop, ideally having the same sample value as loopstart */
    int32_t   samplerate;  /*<! Sample rate at which it was recorded */
    uint8_t   origpitch;   /*<! Root MIDI key number */
    int8_t    pitchadj;    /*<! Pitch adjustment in cents */
    int16_t   samplelink;  /*<! Sample link */
    uint16_t  sampletype;  /*<! 1=Mono, 2=Right Channel, 4=Left Channel */
    int32_t   idx;         /*<! Index of this instrument in the SoundFont */
} esp_midi_sf_sample_t;

/**
 * @brief  Midi sound font stream sample data information in SDTA chunk
 */
typedef struct
{
    uint32_t  sm24size;   /*<! Size of the 24 bits sample data */
    uint32_t  sm24pos;    /*<! Start position of the 24 bits sample data */
    uint32_t  smpl_pos;   /*<! Start position of the 16 bits sample data */
    uint32_t  smpl_size;  /*<! Size of the 16 bits sample data */
} esp_midi_sf_sdat_t;

/**
 * @brief  Initialize and open the SoundFont parser using the provided configuration and handle
 *
 * @param  cfg     Pointer to the configuration structure `esp_midi_sf_parse_cfg_t`
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Success
 *       - ESP_MIDI_ERR_MEM_LACK           Memory shortage
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid parameter passed
 */
esp_midi_err_t esp_midi_sf_parse_open(esp_midi_sf_parse_cfg_t *cfg, esp_midi_sf_parse_handle_t *handle);

/**
 * @brief  Parse and process a MIDI SoundFont file in the ESP-IDF MIDI library using the `esp_midi_sf_parse_handle_t` handle
 *
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Success
 *       - ESP_MIDI_ERR_MEM_LACK           Memory shortage
 *       - ESP_MIDI_ERR_HEADER_PARSE       Error parsing the header
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid parameter passed
 */
esp_midi_err_t esp_midi_sf_parse_process(esp_midi_sf_parse_handle_t handle);

/**
 * @brief  Parse and retrieve a MIDI SoundFont sample based on the given key and velocity values
 *
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 * @param  key     Key number
 * @param  vel     Velocity value
 * @param  sample  Pointer to a pointer to the sample, used to return the found sample
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Success
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_parse_note_on(esp_midi_sf_parse_handle_t handle, uint32_t key, uint32_t vel, esp_midi_sf_sample_t **sample);

/**
 * @brief  Set the program number in the MIDI SoundFont parser
 *
 * @param  handle   Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 * @param  program  Program number
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Success
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_parse_set_program(esp_midi_sf_parse_handle_t handle, int32_t program);

/**
 * @brief  Set the bank number in the MIDI SoundFont parser
 *
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 * @param  bank    Bank number
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Success
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_parse_set_bank(esp_midi_sf_parse_handle_t handle, int32_t bank);

/**
 * @brief  Retrieve sample buffer from the MIDI SoundFont parser
 *
 * @param  handle     Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 * @param  loopstart  The starting point of the loop from which to retrieve the sample.
 * @param  loopend    The ending point of the loop from which to retrieve the sample.
 * @param  data       A pointer to store samples
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Success
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_parse_get_sample(esp_midi_sf_parse_handle_t handle, uint32_t loopstart, uint32_t loopend, esp_midi_data_t *data);

/**
 * @brief  Obtain SDAT information from the MIDI SoundFont parser
 *
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 * @param  sdat    A pointer to a variable of type esp_midi_sf_sdat_t where the retrieved SDAT infomation will be stored.
 *
 * @return
 *       - ESP_MIDI_ERR_OK    Success
 *       - ESP_MIDI_ERR_FAIL  Fail
 */
esp_midi_err_t esp_midi_sf_parse_get_sdat(esp_midi_sf_parse_handle_t handle, esp_midi_sf_sdat_t *sdat);

/**
 * @brief  Close the MIDI SoundFont parse handle. It accepts a handle as input and returns an error code of type esp_midi_err_t
 *
 * @param  handle  Pointer to the MIDI SoundFont parser handle `esp_midi_sf_parse_handle_t`
 *
 * @return
 *       - ESP_MIDI_ERR_OK  Success
 */
esp_midi_err_t esp_midi_sf_parse_close(esp_midi_sf_parse_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
