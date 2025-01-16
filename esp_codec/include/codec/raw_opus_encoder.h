// Copyright 2025 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include "esp_err.h"
#include "audio_element.h"

#define OPUS_ENCODER_TASK_STACK          (40 * 1024)
#define OPUS_ENCODER_TASK_CORE           (0)
#define OPUS_ENCODER_TASK_PRIO           (3)
#define OPUS_ENCODER_RINGBUFFER_SIZE     (2 * 1024)

/**
 * @brief  Enum of RAW OPUS Encoder frame duration choose.
 */
typedef enum {
    RAW_OPUS_ENC_FRAME_DURATION_ARG    = -1,    /*!< Invalid mode */
    RAW_OPUS_ENC_FRAME_DURATION_2_5_MS =  0,     /*!< Use 2.5 ms frames */
    RAW_OPUS_ENC_FRAME_DURATION_5_MS   =  1,     /*!< Use 5 ms frames */
    RAW_OPUS_ENC_FRAME_DURATION_10_MS  =  2,     /*!< Use 10 ms frames */
    RAW_OPUS_ENC_FRAME_DURATION_20_MS  =  3,     /*!< Use 20 ms frames */
    RAW_OPUS_ENC_FRAME_DURATION_40_MS  =  4,     /*!< Use 40 ms frames */
    RAW_OPUS_ENC_FRAME_DURATION_60_MS  =  5,     /*!< Use 60 ms frames */
} raw_opus_enc_frame_duration_t;

/**
 * @brief  Enum of OPUS Encoder application choose.
 */
typedef enum {
    RAW_OPUS_ENC_APPLICATION_ARG      = -1,      /*!< Invalid mode */
    RAW_OPUS_ENC_APPLICATION_VOIP     =  0,       /*!< Voip mode which is best for most VoIP/videoconference applications 
                                                      where listening quality and intelligibility matter most. */  
    RAW_OPUS_ENC_APPLICATION_AUDIO    =  1,       /*!< Audio mode which is best for broadcast/high-fidelity application 
                                                      where the decoded audio should be as close as possible to the input. */
    RAW_OPUS_ENC_APPLICATION_LOWDELAY =  2,       /*!< LOWDELAY mode is only use when lowest-achievable latency is what matters most. */
} raw_opus_enc_application_t;

/**
 * @brief  OPUS Encoder configurations
 */
typedef struct {
    int                           sample_rate;        /*!< The sample rate of OPUS audio.
                                                           This must be one of 8000, 12000,
                                                           16000, 24000, or 48000. */
    int                           channel;            /*!< The numble of channels of OPUS audio.
                                                           This must be mono or dual. */
    int bit_per_sample;                               /*!< The bits per sample of OPUS audio.
                                                           Currently only supported 16 */
    int                           bitrate;            /*!< Suggest bitrate(Kbps) range on mono stream :
                                                           | frame_duration(ms)|    2.5    |     5     |    10    |    20    |    40    |    60    | 
                                                           |   samplerate(Hz)  |           |           |          |          |          |          |
                                                           |       8000        | 50 - 128  | 40 - 128  | 20 - 128 | 20 - 128 | 20 - 128 | 20 - 128 |
                                                           |       12000       | 60 - 192  | 50 - 192  | 30 - 192 | 20 - 192 | 20 - 192 | 20 - 192 |
                                                           |       16000       | 70 - 256  | 60 - 256  | 50 - 256 | 20 - 256 | 20 - 256 | 20 - 256 |
                                                           |       24000       | 70 - 384  | 60 - 384  | 60 - 384 | 60 - 384 | 50 - 384 | 60 - 384 |
                                                           |       48000       | 80 - 510  | 80 - 510  | 80 - 510 | 70 - 510 | 70 - 510 | 70 - 510 |
                                                           Note : 1) This table shows the bitrate range corresponding to each samplerate and frame duration.
                                                                  2) The bitrate range of dual stream is the same that of mono. */
    raw_opus_enc_frame_duration_t frame_duration;     /*!< The duration of one frame.
                                                           This must be 2.5, 5, 10, 20, 40 or 60 ms. */
    raw_opus_enc_application_t    application_mode;   /*!< The application mode. */
    int                           complexity;         /*!< Indicates the complexity of OPUS encoding. 0 is lowest. 10 is higest.*/
    bool                          enable_fec;         /*!< Configures the encoder's use of inband forward error correction (FEC) */
    bool                          enable_dtx;         /*!< Configures the encoder's use of discontinuous transmission (DTX) */
    int                           out_rb_size;    /*!< Size of output ringbuffer */
    int                           task_stack;     /*!< Task stack size */
    int                           task_core;      /*!< Task running in core (0 or 1) */
    int                           task_prio;      /*!< Task priority (based on freeRTOS priority) */
    bool                          stack_in_ext;   /*!< Try to allocate stack in external memory */
} raw_opus_enc_config_t;

#define RAW_OPUS_ENC_CONFIG_DEFAULT() {                     \
    .sample_rate      = 16000,                              \
    .channel          = 1,                                  \
    .bit_per_sample   = 16,                                 \
    .bitrate          = 64000,                              \
    .frame_duration   = RAW_OPUS_ENC_FRAME_DURATION_20_MS,  \
    .application_mode = RAW_OPUS_ENC_APPLICATION_VOIP,      \
    .complexity       = 5,                                  \
    .enable_fec       = false,                              \
    .enable_dtx       = true,                               \
    .out_rb_size      = OPUS_ENCODER_RINGBUFFER_SIZE,       \
    .task_stack       = OPUS_ENCODER_TASK_STACK,            \
    .task_core        = OPUS_ENCODER_TASK_CORE,             \
    .task_prio        = OPUS_ENCODER_TASK_PRIO,             \
    .stack_in_ext     = true,                               \
}

/**
 * @brief  Create an Audio Element handle to encode incoming raw opus data
 *
 * @note: We only support the CBR(Constant Bit Rate) encoding mode for Opus
 * 
 * @param  config  The configuration
 *
 * @return The audio element handle
 */
audio_element_handle_t raw_opus_encoder_init(raw_opus_enc_config_t *config);
