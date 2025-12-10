/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_midi_types.h"
#include "esp_midi_lib_loader_defines.h"
#include "esp_midi_define.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_MIDI_MAX_BPM 60000000

/**
 * @brief  Midi sound configure information
 */
typedef struct {
    esp_midi_synth_note_off_resp_t  note_off_resp;         /* MIDI synthesizer response mode for note-off events.
                                                              This determines how the synthesizer should react when a note-off message is received,
                                                              such as immediately stopping the note or fading it out. */
    uint32_t                        max_out_stream_frame;  /* Maximum number of bytes for the output audio stream.
                                                              This specifies the maximum amount of audio data that the synthesizer can output in a single stream frame,
                                                              aiding in the efficient handling and transmission of audio data. */
    esp_midi_audio_info_t           out_stream_info;       /* Audio output information.
                                                              This structure contains detailed information about the audio output stream,
                                                              such as the samplerate, bits per sample, and number of channels (mono or stereo),
                                                              ensuring that audio data is processed and played back correctly. */
    esp_midi_out_stream_cb_t        out_stream_cb;         /* Callback function for the output audio stream.
                                                              This is a pointer to a function that is called when the synthesizer has audio data to output.
                                                              The callback function is responsible for writing the audio data to the specified output device or buffer. */
    void                           *out_stream_ctx;        /* User-defined context for the output audio stream callback function.
                                                              This is a pointer that can be used to pass any user-specific data or structures to the callback function.
                                                              It is provided to allow the callback function to access context-specific information. */
    esp_midi_lib_cfg_t              sf_lib_cfg;            /* Configuration settings for the sound font library.
                                                              This contains the necessary configuration details for the sound font library */
} esp_midi_sound_cfg_t;

/**
 * @brief  MIDI sound handle
 */
typedef void *esp_midi_sound_handle_t;

/**
 * @brief  MIDI sound information
 */
typedef struct {
    int32_t  track_num;
} esp_midi_sound_info_t;

/**
 * @brief  Create a MIDI sound handle
 *
 * @param  cfg     MIDI sound configure information
 * @param  handle  MIDI sound handle. If the function returns error, handle will be set to NULL.
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_open(esp_midi_sound_cfg_t *cfg, esp_midi_sound_handle_t *handle);

/**
 * @brief  Parse MIDI the first track
 *
 * @code{c}
 *           esp_midi_packet_t midi_pkt = {.data.buffer = buffer, .data.len = len};
 *           while (midi_pkt.data.len)  {
 *             ret = esp_midi_sound_parse(handle, midi_pkt);
 *             if（ret == ESP_MIDI_ERR_CONTINUE）{
 *              midi_pkt.data.buffer += midi_pkt.consumed;
 *              midi_pkt.data.len -= midi_pkt.consumed;
 *             }
 *             if（ret == ESP_MIDI_ERR_OK）{
 *              break;
 *             }
 *           }
 * @endcode
 *
 * @param  handle    MIDI sound handle.
 * @param  midi_pkt  A address to store MIDI data stream and size
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_CONTINUE           Succeeded.The data given has been consumed but the file has not been parsed.
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_parse(esp_midi_sound_handle_t handle, esp_midi_packet_t *midi_pkt, esp_midi_sound_info_t *sound_info);

/**
 * @brief  Loads a sound font library into the MIDI sound engine.
 *
 * @param  handle  MIDI sound handle
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_CONTINUE           Succeeded.The data given has been consumed but the file has not been parsed.
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_load_sf_lib(esp_midi_sound_handle_t handle);

/**
 * @brief  Play music based on midi events. If the midi stream is not updated, this can always play the original midi stream
 *
 * @param  handle  MIDI sound handle
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_process(esp_midi_sound_handle_t handle);

/**
 * @brief  Write a MIDI header to a MIDI packet
 *         This function takes a MIDI header structure and writes it into the provided MIDI packet
 *
 * @param  pkt     Pointer to the MIDI packet where the header will be written
 * @param  header  Pointer to the MIDI header structure that contains the header information
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The header was successfully written
 *       - ESP_MIDI_ERR_BUFF_NOT_ENOUGH    Buffer not enough
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function
 */
esp_midi_err_t esp_midi_sound_enc_midi_header(esp_midi_packet_t *pkt, esp_midi_header_t *header);

/**
 * @brief  Write a MIDI track header to a buffer
 *         This function writes a MIDI track header to the provided buffer, including the track's chunk size
 *
 * @param  buffer      Pointer to the buffer where the track header will be written
 * @param  buffer_len  The length of `buffer`. It must be greater than or equal to ESP_MIDI_TRACK_HEADER_SIZE
 * @param  chunk_size  The size of the MIDI track chunk, excluding the header itself
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The track header was successfully written
 *       - ESP_MIDI_ERR_BUFF_NOT_ENOUGH    Buffer not enough
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function
 */
esp_midi_err_t esp_midi_sound_enc_track_header(uint8_t *buffer, uint32_t buffer_len, uint32_t chunk_size);

/**
 * @brief  Write tempo meta MIDI event to a MIDI packet.
 *
 * @param  pkt    Pointer to the MIDI packet where the event will be written
 * @param  tempo  Tempo of the MIDI sound in beats per minute.
 *                The tempo determines the speed of the MIDI playback
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The event was successfully written
 *       - ESP_MIDI_ERR_FAIL               Failed encoder
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function
 */
esp_midi_err_t esp_midi_sound_enc_tempo(esp_midi_packet_t *pkt, uint32_t tempo);

/**
 * @brief  Write a MIDI event to a MIDI packet.
 *         In a channel message, if the current state is the same as the previous state, user can omit the state.
 *
 * @note  Running Status is for channel messages(such as 0x80、0x90、0xa0、0xb0、0xc0、0xd0、0xe0). The other message will be forced to write status, `is_save_status` is invalid
 *
 * @param  event           Pointer to the parsed MIDI event that needs to be written
 * @param  pkt             Pointer to the MIDI packet where the event will be written
 * @param  is_save_status  Whether to use the status value of the previous event. True: use, False:un-use
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The event was successfully written
 *       - ESP_MIDI_ERR_FAIL               Failed encoder
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function
 */
esp_midi_err_t esp_midi_sound_enc_event(esp_midi_parse_event_t *event, esp_midi_packet_t *pkt, bool is_save_status);

/**
 * @brief  Set the time division for MIDI sound
 *         This function is used to configure the time division parameter, which determines the timing of MIDI events. The time division
 *         specifies how the MIDI clock is divided into smaller units, affecting the timing of notes and other MIDI events. The default is 480
 *
 * @param  handle         The MIDI sound handle that represents a connection to a MIDI sound backend
 * @param  time_division  time_division The time division value to set. This value determines the resolution of MIDI timing, typically expressed: ticks per quarter note (TPQN). A higher value means more precise timing
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The event was successfully processed
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function, such as a NULL pointer for event_node
 */
esp_midi_err_t esp_midi_sound_set_time_division(esp_midi_sound_handle_t handle, uint16_t time_division);

/**
 * @brief  Parse a single MIDI event
 *
 * @code{c}
 *           esp_midi_packet_t raw = {.data.buffer = buffer, .data.len = len};
 *           while (raw.data.len)  {
 *             uint32_t consumed = raw.consumed;
 *             ret = esp_midi_sound_parse_event(last_status, raw, event);
 *             if（ret == ESP_MIDI_ERR_DATA_LACK）{
 *                  raw.consumed = consumed;
 *                  //load data
 *             }
 *             if（ret == ESP_MIDI_ERR_OK）{
 *                  raw.data.buffer += raw.consumed;
 *                  raw.data.len -= raw.consumed;
 *             }
 *           }
 * @endcode
 *
 * @param  last_status  Pointer to the last status of the event. This is used to maintain state between events
 * @param  raw          Pointer to the raw MIDI data packet to be parsed
 * @param  event        Pointer to the structure where the parsed event will be stored
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_DATA_LACK          Data lack
 *       - ESP_MIDI_ERR_MEM_LACK           Insufficient memory
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_parse_event(int32_t *last_status, esp_midi_packet_t *raw, esp_midi_parse_event_t *event);

/**
 * @brief  Free the MIDI event list
 *
 * @param  event  Pointer to the esp_midi_parse_event_t structure node, which will be freed
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_FAIL               No track exist
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_free_event_note(esp_midi_parse_event_t *event);

/**
 * @brief  Free one MIDI event
 *
 * @param  event  Pointer to the esp_midi_parse_event_t structure node, which will be freed
 */
void esp_midi_sound_free_event(esp_midi_parse_event_t *event);

/**
 * @brief  Process a MIDI sound event using a specified MIDI sound handle
 *         This function takes a MIDI sound handle and a parsed MIDI event, and processes the event to produce sound output. The specific behavior
 *         of this function depends on the MIDI sound backend that is associated with the handle
 *
 * @param  handle      The MIDI sound handle that represents a connection to a MIDI sound backend
 * @param  event_node  Pointer to a parsed MIDI event node that contains the event data to be processed
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 The event was successfully processed
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments were passed to the function, such as a NULL pointer for event_node
 */
esp_midi_err_t esp_midi_sound_pro_event(esp_midi_sound_handle_t handle, esp_midi_parse_event_t *event_node);

/**
 * @brief  Pauses the MIDI sound playback
 *         This function is used to temporarily stop the playback of a MIDI sound
 *         without losing the current state or position in the playback
 *
 * @param  handle  The handle to the MIDI sound object that needs to be paused
 *                 This handle is obtained when the MIDI sound is initialized or loaded
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_pause(esp_midi_sound_handle_t handle);

/**
 * @brief  Resumes the MIDI sound playback.
 *         This function is used to continue the playback of a MIDI sound that was
 *         previously paused using esp_midi_sound_pause().
 *
 * @param  handle  The handle to the MIDI sound object that needs to be resumed
 *                 This handle is obtained when the MIDI sound is initialized or loaded
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_resume(esp_midi_sound_handle_t handle);

/**
 * @brief  Set the beats per minute (BPM) for a MIDI sound
 *         This function allows user to specify the tempo of the MIDI sound in beats per minute
 *         The tempo determines the speed of the MIDI playback
 *
 * @param  handle  The handle to the MIDI sound object
 * @param  bpm     The desired BPM value to set for the MIDI sound
 *                 If bpm is set to 0, the MIDI sound tempo is from MIDI stream
 *                 And the maxinum is ESP_MIDI_MAX_BPM
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_set_bpm(esp_midi_sound_handle_t handle, uint32_t bpm);

/**
 * @brief  Get the current beats per minute (BPM) for a MIDI sound
 *         This function retrieves the current tempo of the MIDI sound in beats per minute
 *
 * @param  handle  The handle to the MIDI sound object
 * @param  bpm     A pointer to a variable where the current BPM value will be stored
 *
 * @return
 *       - ESP_MIDI_ERR_OK                 Succeeded
 *       - ESP_MIDI_ERR_INVALID_PARAMETER  Invalid arguments passed
 */
esp_midi_err_t esp_midi_sound_get_bpm(esp_midi_sound_handle_t handle, uint32_t *bpm);

/**
 * @brief  Close a MIDI sound handle
 *
 * @param  handle  MIDI sound handle
 *
 * @return
 *       - ESP_MIDI_ERR_OK  Succeeded
 */
esp_midi_err_t esp_midi_sound_close(esp_midi_sound_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
