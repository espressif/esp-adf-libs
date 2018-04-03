// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_AUDIO_H_
#define _ESP_AUDIO_H_
#include "audio_def.h"
#include "audio_common.h"
#include "audio_element.h"
#include "audio_hal.h"

typedef void *esp_audio_handle_t;

/**
 * @brief esp_audio configuration parameters
 */
typedef struct esp_audio_cfg_t {
    int                 in_stream_buf_size;         /*!< Input buffer size */
    int                 out_stream_buf_size;        /*!< Output buffer size */
    int                 resample_rate;              /*!< Destination sample rate,0:disable rsample;others:44.1K,48K,32K,16K,8K has supported*/
    audio_hal_handle_t  hal;                        /*!< Codec IC hardware handle*/
    QueueHandle_t       evt_que;                    /*!< For received esp_audio events (optional)*/
    int                 task_prio;                  /*!< esp_audio task priority*/
} esp_audio_cfg_t;

/**
 * @brief esp_audio setup parameters by manual
 */
typedef struct esp_audio_setup_t {
    audio_codec_type_t  set_type;               /*!< Set codec type */
    int                 set_sample_rate;        /*!< Set music sample rate */
    int                 set_channel;            /*!< Set music channels */
    char               *set_path;               /*!< Set out stream path */
    char               *set_uri;                /*!< Set URI */
    char               *set_in_stream;          /*!< Tag of in_stream */
    char               *set_codec;              /*!< Tag of the codec */
    char               *set_out_stream;         /*!< Tag of out_stream */
} esp_audio_setup_t;

/**
 * @brief Create esp_audio instance according to 'cfg' parameter
 *
 * This function create an esp_audio instance, at the specified configuration.
 *
 * @param[in] cfg  Provide esp_audio initialization configuration
 *
 * @return
 *      - NULL: Error
 *      - Others: esp_audio instance fully certifying
 */
esp_audio_handle_t esp_audio_create(const esp_audio_cfg_t *cfg);

/**
 * @brief Specific esp_audio instance will be destroyed
 *
 * @param[in] handle  The esp_audio instance
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on success
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: no instance to free, call esp_audio_init first
 */
audio_err_t esp_audio_destroy(esp_audio_handle_t handle);

/**
 * @brief Add audio input stream to specific esp_audio instance
 *
 * @param[in] handle    The esp_audio instance
 * @param[in] in_stream Audio stream instance
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on success
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_ALREADY_EXISTS: in_stream has already exist or have the same stream tag.
 */
audio_err_t esp_audio_input_stream_add(esp_audio_handle_t handle, audio_element_handle_t in_stream);

/**
 * @brief Add audio output stream to specific esp_audio instance
 *
 * @param[in] handle       The esp_audio instance
 * @param[in] out_stream   The audio stream element instance
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on success
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_ALREADY_EXISTS: out_stream has already exist or have the same stream tag.
 */
audio_err_t esp_audio_output_stream_add(esp_audio_handle_t handle, audio_element_handle_t out_stream);

/**
 * @brief Add a new codec lib that can decode or encode a music file
 *
 * @param[in] handle    The esp_audio instance
 * @param[in] type      The audio codec type(encoder or decoder)
 * @param[in] lib       To provide audio stream element
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on success
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_ALREADY_EXISTS: lib has already exist or have the same extension.
 */
audio_err_t esp_audio_codec_lib_add(esp_audio_handle_t handle, audio_codec_type_t type, audio_element_handle_t lib);

/*
 * @brief Check if this kind of music extension is supported or not
 *
 * @note This function just query the codec which has already add by esp_audio_codec_lib_add.
 *       The max length of extension is 6.
 *
 * @param[in] handle    The esp_audio instance
 * @param[in] type      The CODEC_ENCODER or CODEC_DECODER
 * @param[in] extension Such as "mp3", "wav", "aac"
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: supported
 *      - ESP_ERR_AUDIO_NOT_SUPPORT: not support
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 */
audio_err_t esp_audio_codec_lib_query(esp_audio_handle_t handle, audio_codec_type_t type, const char *extension);

/*
 * @brief Play the given uri
 *
 * The esp_audio_play have follow activity, setup inputstream, outputstream and codec by uri, start all of them.
 * There is a rule that esp_audio will select input stream, codec and output stream by URI field.
 * Rule of URI field are as follow.
 *     --`UF_SCHEMA` field of URI for choose input stream from existing streams. e.g:"http","file"
 *     --`UF_PATH` field of URI for choose codec from existing codecs. e.g:"/audio/mp3_music.mp3"
 *     --`UF_FRAGMENT` field of URI for choose output stream from existing streams, output stream is I2S by default.
 *     --`UF_USERINFO` field of URI for specific sample rate and channels at encode mode.
 *        The format "user:password" in the userinfo field,"user" is sample rate, "password" is channels.
 *
 * Now esp_audio_play support follow URIs.
 *     -- "https://dl.espressif.com/dl/audio/mp3_music.mp3"
 *     -- "http://media-ice.musicradio.com/ClassicFMMP3"
 *     -- "file://sdcard/test.mp3"
 *     -- "iis://16000:2@from.pcm/rec.wav#file"
 *     -- "iis://16000:1@record.pcm/record.wav#raw"
 *
 * @note The URI parse by `http_parser_parse_url`,any illegal string will be return `ESP_ERR_AUDIO_INVALID_URI`.
 *
 * @param handle The esp_audio_handle_t instance
 * @param uri    Such as "file://sdcard/test.wav" or "http://iot.espressif.com/file/example.mp3",
 * @param type   Specific handle type decoder or encoder
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_TIMEOUT: timeout the play activity.
 *      - ESP_ERR_AUDIO_INVALID_URI: URI is illegal
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 */
audio_err_t esp_audio_play(esp_audio_handle_t handle, audio_codec_type_t type, const char *uri);

/*
 * @brief Stop the esp_audio
 *
 * @note If user queue has been registered by evt_que, AUDIO_STATUS_STOPED event for success
 *       or AUDIO_STATUS_ERROR event for error will be received.
 *
 * @param[in] handle The esp_audio instance
 * @param[in] type   Stop immediately or done
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_TIMEOUT: timeout the stop activity.
 */
audio_err_t esp_audio_stop(esp_audio_handle_t handle, audio_termination_type_t type);

/*
 * @brief Pause the esp_audio
 *
 * @note Only support music and without live stream. If user queue has been registered by evt_que, AUDIO_STATUS_PAUSED event for success
 *       or AUDIO_STATUS_ERROR event for error will be received.
 *
 * @param[in] handle The esp_audio instance
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_TIMEOUT: timeout the pause activity.
 */
audio_err_t esp_audio_pause(esp_audio_handle_t handle);

/*
 * @brief Resume the music paused
 *
 * @note Only support music and without live stream. If user queue has been registered by evt_que, AUDIO_STATUS_PLAYING event for success
 *       or AUDIO_STATUS_ERROR event for error will be received.
 *
 * @param[in] handle The esp_audio instance
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 *      - ESP_ERR_AUDIO_TIMEOUT: timeout the resume activity.
 */
audio_err_t esp_audio_resume(esp_audio_handle_t handle);

/*
 * @brief Setting esp_audio volume.
 *
 * @param[in] handle    The esp_audio instance
 * @param[in] vol       Specific volume will be set. 0-100 is legal. 0 will be mute.
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_CTRL_HAL_FAIL: error with hardware.
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 */
audio_err_t esp_audio_vol_set(esp_audio_handle_t handle, int vol);

/*
 * @brief Get esp_audio volume
 *
 * @param[in] handle    The esp_audio instance
 * @param[out] vol      A pointer to int that indicates esp_audio volume.
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_CTRL_HAL_FAIL: error with hardware.
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: invalid arguments
 */
audio_err_t esp_audio_vol_get(esp_audio_handle_t handle, int *vol);

/*
 * @brief Get esp_audio status
 *
 * @param[in] handle    The esp_audio instance
 * @param[out] state    A pointer to esp_audio_state_t that indicates esp_audio status.
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: no esp_audio instance or esp_audio does not playing
 */
audio_err_t esp_audio_state_get(esp_audio_handle_t handle, esp_audio_state_t *state);

/*
 * @brief Get the position of current music that is playing in microseconds.
 *
 * @note This function works with decoding music.
 *
 * @param[in] handle    The esp_audio instance
 * @param[out] pos      A pointer to int that indicates esp_audio decoding position.
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: no esp_audio instance
 *      - ESP_ERR_AUDIO_NOT_READY:no out stream.
 */
audio_err_t esp_audio_pos_get(esp_audio_handle_t handle, int *pos);

/*
 * @brief Choose the `in_stream`, `codec` and `out_stream` definitely, and set `uri`.
 *
 * @note This function provide a manual way to select in/out stream and codec, should be called before the `esp_audio_play`,
 *       then ignore the `esp_audio_play` URI parameter only one time.
 *
 * @param[in] handle    The esp_audio instance
 * @param[in] sets      A pointer to esp_audio_setup_t.
 *
 * @return
 *      - ESP_ERR_AUDIO_NO_ERROR: on succss
 *      - ESP_ERR_AUDIO_INVALID_PARAMETER: no esp_audio instance
 *      - ESP_ERR_AUDIO_MEMORY_LACK: allocate memory fail
 */
audio_err_t esp_audio_setup(esp_audio_handle_t handle, esp_audio_setup_t *sets);
#endif
