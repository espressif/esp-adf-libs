// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_TYPE_DEF_H_
#define _AUDIO_TYPE_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    ESP_CODEC_TYPE_UNKNOW        = 0,
    ESP_CODEC_TYPE_RAW           = 1,
    ESP_CODEC_TYPE_WAV           = 2,
    ESP_CODEC_TYPE_MP3           = 3,
    ESP_CODEC_TYPE_AAC           = 4,
    ESP_CODEC_TYPE_OPUS          = 5,
    ESP_CODEC_TYPE_M4A           = 6,
    ESP_CODEC_TYPE_MP4           = 7,
    ESP_CODEC_TYPE_FLAC          = 8,
    ESP_CODEC_TYPE_OGG           = 9,
    ESP_CODEC_TYPE_TSAAC         = 10,
    ESP_CODEC_TYPE_AMRNB         = 11,
    ESP_CODEC_TYPE_AMRWB         = 12,  
    ESP_CODEC_TYPE_PCM           = 13,
    ESP_AUDIO_TYPE_M3U8          = 14,
    ESP_AUDIO_TYPE_PLS           = 15,  
    ESP_CODEC_TYPE_UNSUPPORT     = 16,
} esp_codec_type_t;

typedef enum
{
    ESP_DECODER_WORK_MODE_MANUAL = 0,
    ESP_DECODER_WORK_MODE_AUTO   = 1,
} esp_decoder_work_mode_t;

typedef enum
{   
    ESP_CODEC_ERR_CONTINUE       = 1,
    ESP_CODEC_ERR_OK             = 0,
    ESP_CODEC_ERR_FAIL           = -1,
    ESP_CODEC_ERR_DONE           = -2,
} esp_codec_err_t;

/**
 * @brief      Gain codec extension. need to sync by `esp_codec_type_t`.
 *              eg. esp_codec_type_t codec_type = ESP_CODEC_TYPE_MP3;
 *                  char* ext = get_codec_ext(codec_type);
 *                  the value of ext is `mp3`.
 *
 * @param      codec_type   the value of `esp_codec_type_t`
 *
 * @return     codec extension
 */
const char * get_codec_ext(esp_codec_type_t codec_type);

#ifdef __cplusplus
}
#endif

#endif
