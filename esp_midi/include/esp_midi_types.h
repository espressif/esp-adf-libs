/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once
#pragma pack(1)

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_check.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_MIDI_RETURN_ON_FALSE ESP_RETURN_ON_FALSE
#define ESP_MIDI_GOTO_ON_FALSE   ESP_GOTO_ON_FALSE
#define ESP_MIDI_LOGE            ESP_LOGE
#define ESP_MIDI_LOGI            ESP_LOGI

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message,
 * sets the local variable 'ret' to the supplied 'err_code', and then exits by jumping to 'goto_tag'.
 */
#define ESP_MIDI_GOTO_ON_FAIL(goto_tag, log_tag, format, ...) do {                         \
    if (ret != ESP_MIDI_ERR_OK) {                                                          \
        ESP_MIDI_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
        goto goto_tag;                                                                     \
    }                                                                                      \
} while (0)

/**
 * Macro which can be used to check the condition. If the condition is not 'true', it prints the message,
 * sets the local variable 'ret' to the supplied 'err_code', and then exits by jumping to 'goto_tag'.
 */
#define ESP_MIDI_RETURN_ON_FAIL(log_tag, format, ...) do {                                 \
    if (ret != ESP_MIDI_ERR_OK) {                                                          \
        ESP_MIDI_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
        return ret;                                                                        \
    }                                                                                      \
} while (0)

/**
 * @brief  Data packet
 */
typedef struct {
    uint8_t  *buffer;  /*!< Input encoded data buffer */
    uint32_t  len;     /*!< Input encoded data size */
} esp_midi_data_t;

/**
 * @brief  Packet of data
 */
typedef struct {
    esp_midi_data_t  data;      /*!< Data packet*/
    uint32_t         consumed;  /*!< Represents the size of the consumed input data.*/
} esp_midi_packet_t;

/**
 * @brief  MIDI error type definition
 */
typedef enum {
    ESP_MIDI_ERR_CONTINUE          = 1,   /*!< Continue */
    ESP_MIDI_ERR_OK                = 0,   /*!< Success */
    ESP_MIDI_ERR_FAIL              = -1,  /*!< Fail */
    ESP_MIDI_ERR_MEM_LACK          = -2,  /*!< Fail to malloc memory */
    ESP_MIDI_ERR_DATA_LACK         = -3,  /*!< Data is not enough */
    ESP_MIDI_ERR_HEADER_PARSE      = -4,  /*!< Parse header happened error */
    ESP_MIDI_ERR_INVALID_PARAMETER = -5,  /*!< Input invalid parameter */
    ESP_MIDI_ERR_ALREADY_EXIST     = -6,  /*!< Audio library is already exist */
    ESP_MIDI_ERR_NOT_SUPPORT       = -7,  /*!< Not support type */
    ESP_MIDI_ERR_BUFF_NOT_ENOUGH   = -8,  /*!< Buffer not enough */
    ESP_MIDI_ERR_NOT_FOUND         = -9,  /*!< Not found */
} esp_midi_err_t;

/**
 * @brief  Audio information
 */
typedef struct _esp_midi_audio_info_t {
    uint32_t  samplerate;  /*< Samplerate of audio */
    uint8_t   channel;     /*< Channel of audio */
    uint8_t   bits;        /*< Bits per sample of audio */
} esp_midi_audio_info_t;

/**
 * @brief  MIDI synther response mode for note off event
 */
typedef enum _esp_midi_synth_note_off_resp_t {
    ESP_MIDI_SYNTH_NOTE_OFF_RESP_IGNORE = 0,  /*< Ignore the note off event */
    ESP_MIDI_SYNTH_NOTE_OFF_RESP_RELEASE,     /*<! Release the note stream */
    ESP_MIDI_SYNTH_NOTE_OFF_RESP_REDUCE,      /*<! Linear reduction the note stream by velocity  */
} esp_midi_synth_note_off_resp_t;

/**
 * @brief  MIDI information per channel
 */
typedef struct _audio_ch_t {
    uint8_t   bank;              /*<! The bank number of channel */
    uint8_t   program_change;    /*<! The program change number of channel */
    uint8_t   channel_pressure;  /*<! The channel pressure number of channel */
    uint16_t  pitch;             /*<! The pitch number of channel */
    uint8_t   note;              /*<! The note indx of channel */
    uint8_t   velocity;          /*<! The velocity indx of channel */
    int32_t   gain;              /*<! The gain of channel */
} esp_midi_channel_t;

/**
 * @brief  Output audio stream callback function
 *
 * @param  buf       A address to store the output stream data
 * @param  buf_size  The size of  `buf`
 * @param  ctx       The handle of output audio stream callback function
 */
typedef esp_midi_err_t (*esp_midi_out_stream_cb_t)(uint8_t *buf, uint32_t buf_size, void *ctx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
