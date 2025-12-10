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
 * @brief  A handle to a MIDI sound font library
 */
typedef void *esp_midi_lib_loader_handle_t;

/**
 * @brief  Configure to a MIDI sound font library
 */
typedef void *esp_midi_lib_loader_cfg_t;

/**
 * @brief  An enum type defining the sample types
 */
typedef enum {
    ESP_MIDI_SF_SAMPLE_TYPE_MONO  = 0,  /*<! Mono sample */
    ESP_MIDI_SF_SAMPLE_TYPE_LEFT  = 1,  /*<! Left channel of a stereo sample */
    ESP_MIDI_SF_SAMPLE_TYPE_RIGHT = 2,  /*<! Right channel of a stereo sample */
} esp_midi_lib_sample_type_t;

/**
 * @brief  A structure containing detailed information about a MIDI SoundFont sample
 */
typedef struct
{
    /*The description of sample data from MIDI event*/
    uint16_t                    bank;            /*<! The bank number of channel */
    uint16_t                    program_change;  /*<! The program change number of channel */
    uint16_t                    velocity;        /*<! The velocity index of channel */
    uint16_t                    note;            /*<! The note index of channel */
    uint16_t                    ch_index;        /*<! The index of channel */
    /*The description of sample data from library*/
    uint32_t                    loopstart;   /*<! Sample number offset from start to start of loop */
    uint32_t                    loopend;     /*<! Sample number offset from start to end of loop, marks the first point after loop, whose sample value is ideally equivalent to loopstart */
    uint8_t                     origpitch;   /*<! Root midi key number */
    int8_t                      pitchadj;    /*<! Pitch correction in cents */
    float                       gain;        /*<! Gain of sample */
    int32_t                     samplerate;  /*<! Samplerate recorded at */
    uint8_t                     bits;        /*<! Bits per sample */
    esp_midi_lib_sample_type_t  sampletype;  /*<! An enum type defining the sample types*/
    /*The address and size of sample data */
    esp_midi_data_t             data;  /*<! Field for additional sample data. The `loader_set_data_t` callback can set the data. The `loader_free_data_t` can free the data.*/
} esp_midi_lib_sample_desc_t;

/**
 * @brief  Loads a MIDI SoundFont library
 *
 * @param  cfg         The configuration for the loader, which includes parameters needed to load the instrument
 * @param  lib_handle  A pointer to a variable that will receive the handle to the loaded MIDI SoundFont library
 *
 * @return
 */
typedef esp_midi_err_t (*loader_load_cb_t)(esp_midi_lib_loader_cfg_t cfg, esp_midi_lib_loader_handle_t *lib_handle);

/**
 * @brief  Deletes a MIDI SoundFont library
 *
 * @param  lib_handle  The handle to the MIDI SoundFont library from which the instrument will be deleted
 *
 * @return
 */
typedef esp_midi_err_t (*loader_delete_cb_t)(esp_midi_lib_loader_handle_t lib_handle);

/**
 * @brief  Sets sample data(sample_desc->data) for a specific preset by `sample_desc` description
 *
 * @param  lib_handle   The handle to the MIDI SoundFont library where the preset is located
 * @param  sample_desc  A pointer to a structure containing the sample data and its description
 *
 * @return
 */
typedef esp_midi_err_t (*loader_set_data_t)(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Frees sample data(sample_desc->data) for a specific preset in the MIDI SoundFont library
 *
 * @param  lib_handle   The handle to the MIDI SoundFont library where the preset's sample data is stored
 * @param  sample_desc  A pointer to a structure containing the sample data(sample_desc->data) that needs to be freed
 *
 * @return
 */
typedef esp_midi_err_t (*loader_free_data_t)(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Handles a note-on MIDI event by selecting a sample based on the provided sample description
 *
 * @param  lib_handle   The handle to the MIDI SoundFont library where the preset and sample data are stored
 * @param  sample_desc  A pointer to a structure containing the sample description that should be played for the note-on event
 * @param  chan         MIDI channel number on which the note-on event occurred
 * @param  key          MIDI note number (0-127) representing the pitch of the note
 * @param  vel          MIDI velocity (0-127) representing the intensity of the note strike
 *
 * @return
 */
typedef esp_midi_err_t (*loader_noteon_t)(esp_midi_lib_loader_handle_t lib_handle, esp_midi_lib_sample_desc_t *sample_desc);

/**
 * @brief  Sets the MIDI bank number for a SoundFont preset within the MIDI SoundFont library
 *
 * @param  lib_handle  The handle to the MIDI SoundFont library where the preset is located
 * @param  bank        MIDI bank number (0-127) to be set for the SoundFont preset
 *
 * @return
 */
typedef esp_midi_err_t (*loader_set_bank_t)(esp_midi_lib_loader_handle_t lib_handle, int32_t bank);

/**
 * @brief  Sets the MIDI preset number (program change number) for a SoundFont within the MIDI SoundFont library
 *
 * @param  lib_handle  The handle to the MIDI SoundFont library where the preset is located
 * @param  prenum      MIDI program change number (0-127) to be set for the SoundFont preset
 *
 * @return
 */
typedef esp_midi_err_t (*loader_set_program_t)(esp_midi_lib_loader_handle_t lib_handle, int32_t prenum);

/**
 * @brief  A structure containing pointers to various callback functions required for operating the MIDI SoundFont library
 */
typedef struct
{
    esp_midi_lib_loader_cfg_t  loader_cfg;     /*<! Configuration for the loader, of type esp_midi_lib_loader_cfg_t */
    loader_load_cb_t           loader_cb;      /*<!  Callback function for loading, of type loader_load_cb_t */
    loader_delete_cb_t         delete_cb;      /*<!  Callback function for deleting (or unloading), of type loader_delete_cb_t */
    loader_noteon_t            note_on_cb;     /*<!  Callback function for note-on events, of type loader_noteon_t */
    loader_set_bank_t          set_bank_cb;    /*<! Callback function for setting the MIDI bank, of type loader_set_bank_t */
    loader_set_program_t       set_preset_cb;  /*<!  Callback function for setting the MIDI program change (rename preset) */
    loader_set_data_t          set_data_cb;    /*<!  Callback function for setting additional data, of type loader_set_data_t */
    loader_free_data_t         free_data_cb;   /*<!  Callback function for freeing allocated data, of type loader_free_data_t */
} esp_midi_lib_cfg_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
