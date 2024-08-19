// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _EQUALIZER_H_
#define _EQUALIZER_H_

#include "esp_err.h"
#include "audio_element.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @note
 *        1) This figure indicate the default eq gain of every band in current equalizer.
 *        2) Every channel have 10 band to set.
 *
 *        +-----------------------------------------------------------+
 *        |                               MONO                        |
 *        +-----------------------------------------------------------+
 *        |               Only Left channel/Only Right channel        |
 *        +-----------------------------------------------------------+
 *        |band0|band1|band2|band3|band4|band5|band6|band7|band8|band9|
 *        +-----------------------------------------------------------+
 *        |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
 *        +-----------------------------------------------------------+
 *
 *        +-----------------------------------------------------------+-----------------------------------------------------------+
 *        |                                                          DUAL                                                         |
 *        +-----------------------------------------------------------+-----------------------------------------------------------+
 *        |                          Left channel                     |                          Right channel                    |
 *        +-----------------------------------------------------------+-----------------------------------------------------------+
 *        |band0|band1|band2|band3|band4|band5|band6|band7|band8|band9|band0|band1|band2|band3|band4|band5|band6|band7|band8|band9|
 *        +-----------------------------------------------------------+-----------------------------------------------------------+
 *        |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
 *        +-----------------------------------------------------------+-----------------------------------------------------------+
 * 
 *        3) Different sample rates support different EQ frequency bands. 
 *         11025: {31, 62, 125, 250, 500, 1000, 2000, 3000, 4000, 5500}
 *         22050: {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 11000}
 *         44100/48000: {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000}
 */

/**
 * @brief  Equalizer Configuration
 */
typedef struct equalizer_cfg {
    int  samplerate;    /*!< Audio sample rate.
                             Supported samplerate: 11025, 22050, 44100, 48000, unit: Hz */
    int  channel;       /*!< Number of audio channels.
                             Supported channel: mono, stereo */
    int *set_gain;      /*!< Equalizer gain */
    int  out_rb_size;   /*!< Size of output ring buffer */
    int  task_stack;    /*!< Task stack size */
    int  task_core;     /*!< Task running in core...*/
    int  task_prio;     /*!< Task priority*/
    bool stack_in_ext;  /*!< Try to allocate stack in external memory */
} equalizer_cfg_t;

#define EQUALIZER_TASK_STACK      (4 * 1024)
#define EQUALIZER_TASK_CORE       (0)
#define EQUALIZER_TASK_PRIO       (5)
#define EQUALIZER_RINGBUFFER_SIZE (8 * 1024)

/**
 * @note  `set_value_gain` is defined in c file.
 *        values is {-13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13};
 */
extern int set_value_gain[];

#define DEFAULT_EQUALIZER_CONFIG() {            \
    .samplerate   = 48000,                      \
    .channel      = 1,                          \
    .set_gain     = set_value_gain,             \
    .out_rb_size  = EQUALIZER_RINGBUFFER_SIZE,  \
    .task_stack   = EQUALIZER_TASK_STACK,       \
    .task_core    = EQUALIZER_TASK_CORE,        \
    .task_prio    = EQUALIZER_TASK_PRIO,        \
    .stack_in_ext = true,                       \
}

/**
 * @brief  Set the audio sample rate and the number of channels to be processed by the equalizer.
 *
 * @param  self  Audio element handle
 * @param  rate  Audio sample rate
 * @param  ch    Audio channel
 *
 * @return
 *       - ESP_OK               on success
 *       - ESP_ERR_INVALID_ARG  invalid arguments
 */
esp_err_t equalizer_set_info(audio_element_handle_t self, int rate, int ch);

/**
 * @brief  Set the audio gain to be processed by the equalizer.
 *
 * @param  self                    Audio element handle
 * @param  index                   The position of center frequencies of equalizer.
 *                                 If channel is mono, the index range is [0, 9];
 *                                 If channel is stereo and `is_channels_gain_equal` is true, the index range is [0, 9];
 *                                 If channel is stereo and `is_channels_gain_equal` is false, the index range is [0, 19];
 * @param  value_gain              The value of audio gain which in `index`
 * @param  is_channels_gain_equal  If audio channel is stereo, the audio gain values of two channels are equal when `is_channels_gain_equal` is `true`,
 *                                 otherwise it means unequal.
 *
 * @return
 *       - ESP_OK               on success
 *       - ESP_ERR_INVALID_ARG  invalid arguments
 */
esp_err_t equalizer_set_gain_info(audio_element_handle_t self, int index, int value_gain, bool is_channels_gain_equal);

/**
 * @brief  Create an Audio Element handle that equalizes incoming data.
 *
 * @param  config  The configuration
 *
 * @return The audio element handler
 */
audio_element_handle_t equalizer_init(equalizer_cfg_t *config);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _EQUALIZER_H_ */