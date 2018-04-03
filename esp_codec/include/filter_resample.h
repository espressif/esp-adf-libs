// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _RSP_FILTER_H_
#define _RSP_FILTER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * brief      Resample filter configurations
 */
typedef struct {
    int                 src_rate;   /*!< Source audio sample rates (in Hz)*/
    int                 src_ch;     /*!< Source audio channel (Mono=1, Stereo=2) */
    int                 dest_rate;  /*!< Destination audio sample rates (in Hz) */
    int                 dest_ch;    /*!< Destination audio channel (Mono=1, Stereo=2) */
    audio_codec_type_t  type;       /*!< Type of filter */
} rsp_filter_cfg_t;

#define DEFAULT_RESAMPLE_FILTER_CONFIG() {\
    .src_rate       = 0,    \
    .src_ch         = 0,    \
    .dest_rate      = 48000,\
    .dest_ch        = 2,    \
    .type           = AUDIO_CODEC_TYPE_DECODER,    \
}

/**
 * @brief      Create an Audio Element handle to a filter that is able to resample incoming data.
 *             Depending on configuration, the filter may upsample, downsample, as well as convert
 *             the data between mono and stereo.
 *             - If the audio_codec_type_t is `AUDIO_CODEC_TYPE_DECODER`, configuration values of `src_rate` and `src_ch`
 *             have no effect, the `src_*` information provide by `audio_element_getinfo`
 *             - If the audio_codec_type_t is `AUDIO_CODEC_TYPE_DECODER`, the configurations `src_*` and `dest_*`  must be provided
 *
 * @param      config  The configuration
 *
 * @return     The audio element handle
 */
audio_element_handle_t rsp_filter_init(rsp_filter_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif
