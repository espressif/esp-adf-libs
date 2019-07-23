// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _DOWNMIX_H_
#define _DOWNMIX_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"
#include "esp_downmix.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
* @brief      Downmix informatiom
*/
typedef struct {
    int samplerate[2];                 /*!< Audio sample rates (in Hz). samplerate[0]: the sample rate of the base audio file. samplerate[1]: the sample rate of the newcome audio file. */
    int channel[2];                    /*!< Audio channel (Mono=1, Dual=2). channel[0]: the number of channel(s) of the base audio file. channel[1]: the number of channel(s) of the newcome audio file. */
    float gain[4];                     /*!< The gain is expressed using the logarithmic decibel (dB) units (dB gain).
                                        When the downmixing is switched on, the gains of the audio files will be gradually changed from gain[0] to gain[1] in the transition period, and stay at gain[1] in the stable period;
                                        When the downmixing is switched off, the gains of the audio files will be gradually changed back from gain[3] to gain[2] in the transition period, and stay at gain[2] in the stable period;

                                        For the base audio file:
                                            * gain[0]: the original gain of the base audio file before the downmixing process. Usually, if the downmixing is not used, set_gain[0] is usually set to 0 dB.
                                            * gain[1]: the target gain of the base audio file after the downmixing process.

                                        For the newcome audio file:
                                            * gain[2]: the original gain of the newcome audio file before the downmixing process. Usually, if the set_gain[0] is set to a relatively large value, such as -96 dB, it means the newcome audio file can be ignored.
                                            * gain[3]: the target gain of the base audio file after the downmixing process. Usually, if the set_gain[0] is 0 dB, it means the newcome audio becomes the main audio source.The audio will gradually change from gain[0] to gain[1] in transit period when downmix switch on and downmix with set_gain[1] in stable period.
                                        */
    int transform_time[2];                         /*!< the length of the transition period in milliseconds. transform_time[0] is for the base audio file and transform_time[1] is for the newcome audio file.*/
    downmix_select_channel_t dual_two_mono_select; /*!< When first level downmix output file is stero, and channel number of final output file is set one, Select whether to proceed to second downmix  */
    downmix_play_status_t play_status;             /*!<The control status in downmixing. refer in esp_downmix.h*/
    downmix_source_info_t output_status;           /*!<The number channels of output file by processed downmix.refer in esp_downmix.h*/
} downmix_info_t;

/**
* @brief      Downmix configuration
*/
typedef struct {
    downmix_info_t downmix_info; /*!< Downmix information*/
    int out_rb_size;             /*!< Size of ring buffer */
    int task_stack;              /*!< Size of task stack */
    int task_core;               /*!< Task running in core...*/
    int task_prio;               /*!< Task priority (based on the FreeRTOS priority) */
} downmix_cfg_t;

#define DOWNMIX_TASK_STACK (8 * 1024)
#define DOWNMIX_TASK_CORE (0)
#define DOWNMIX_TASK_PRIO (5)
#define DOWNMIX_RINGBUFFER_SIZE (8 * 1024)

#define DEFAULT_DOWNMIX_CONFIG()                                      \
    {                                                                 \
        .downmix_info = {                                             \
            .samplerate = {44100, 44100},                             \
            .channel = {1, 2},                                        \
            .gain = {0, 0, 0, 0},                                     \
            .transform_time = {0, 50},                                \
            .dual_two_mono_select = SELECT_LEFT_CHANNEL,              \
            .play_status = DOWNMIX_BYPASS,                            \
            .output_status = ESP_DOWNMIX_TYPE_OUTPUT_ONE_CHANNEL      \
            },                                                        \
        .out_rb_size = DOWNMIX_RINGBUFFER_SIZE,                       \
        .task_stack = DOWNMIX_TASK_STACK,                             \
        .task_core = DOWNMIX_TASK_CORE,                               \
        .task_prio = DOWNMIX_TASK_PRIO,                               \
    }

/**
* @brief      Sets the downmix timeout.
*
* @param      self               audio element handle
* @param      ticks_to_wait      input ringbuffer timeout
*/
void downmix_set_second_input_rb_timeout(audio_element_handle_t self, int ticks_to_wait);

/**
* @brief      Sets the downmix second input ringbuffer.
*
* @param      self      audio element handle
* @param      rb        handle of ringbuffer.
*/
void downmix_set_second_input_rb(audio_element_handle_t self, ringbuf_handle_t rb);

/**
* @brief      Passes the downmix output status.
*
* @param      self               audio element handle
* @param      status_value       the value of the downmix output status.
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_output_status(audio_element_handle_t self, downmix_source_info_t status_value);

/**
* @brief      Passes the downmix play status.
*
* @param      self               audio element handle
* @param      status_value       the value of the downmix play status.
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_play_status(audio_element_handle_t self, downmix_play_status_t status_value);

/**
* @brief      Passes the downmix dual_two_mono_select status.
*
* @param      self               audio element handle
* @param      status_value       the value of the downmix dual_two_mono_select status.
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_dual_two_mono_select_info(audio_element_handle_t self, int dual_two_mono_select);

/**
* @brief      Sets the base audio sample rate and the number of channels to be processed.
*
* @param      self       audio element handle
* @param      rate      sample rate of the base audio file
* @param      ch        number of channel(s) of the base audio file
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_base_file_info(audio_element_handle_t self, int rate, int ch);

/**
* @brief      Sets the newcome audio sample rate and the number of channels to be processed.
*
* @param      self       audio element handle
* @param      rate      sample rate of the newcome audio file
* @param      ch        number of channel(s) of the newcome audio file
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_newcome_file_info(audio_element_handle_t self, int rate, int ch);

/**
* @brief      Sets the audio gain to be processed.
*
* @param      self                  audio element handle
* @param      gain                  the reset value of `gain` which in `downmix_info_t`. The `gain` is an array of four elements.
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_gain_info(audio_element_handle_t self, float *gain);

/**
* @brief      Sets the audio `transform_time` to be processed.
*
* @param      self                  audio element handle
* @param      transform_time        the reset value of `transform_time` which in `downmix_info_t`. The `transform_time` is an array of two elements.
*
* @return
*             ESP_OK
*             ESP_FAIL
*/
esp_err_t downmix_set_transform_time_info(audio_element_handle_t self, int *transform_time);

/**
* @brief      Initializes the Audio Element handle for downmixing.
*
* @param      config  the configuration
*
* @return     The initialized Audio Element handle
*/
audio_element_handle_t downmix_init(downmix_cfg_t *config);

#ifdef __cplusplus
}
#endif

#endif
