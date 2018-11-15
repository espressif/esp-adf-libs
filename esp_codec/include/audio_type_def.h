// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_TYPE_DEF_H_
#define _AUDIO_TYPE_DEF_H_

typedef enum
{
    ESP_CODEC_TYPE_UNKNOW        = 0,
    ESP_CODEC_TYPE_WAV           = 1,
    ESP_CODEC_TYPE_RAWFLAC       = 2,
    ESP_CODEC_TYPE_OGGFLAC       = 3,
    ESP_CODEC_TYPE_AMRNB         = 4,
    ESP_CODEC_TYPE_AMRWB         = 5,
    ESP_CODEC_TYPE_MP3           = 6,
    ESP_CODEC_TYPE_AAC           = 7,
    ESP_CODEC_TYPE_M4A           = 8,
    ESP_CODEC_TYPE_TSAAC         = 9,    
    ESP_CODEC_TYPE_VORBIS        = 10,
    ESP_CODEC_TYPE_OPUS          = 11,    
    ESP_CODEC_TYPE_UNSUPPORT     = 12,
} esp_codec_type_t;

typedef enum
{
    ESP_DECODER_WORK_MODE_MANUAL = 0,
    ESP_DECODER_WORK_MODE_AUTO   = 1,
} esp_decoder_work_mode_t;

typedef enum
{
    ESP_CODEC_ERR_CONTINUE       = 1,
    ESP_CODEC_ERR_DONE           = -2,
} esp_codec_err_t;

#endif
