// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "esp_log.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "esp_sonic.h"
#include "audio_sonic.h"
#include "audio_type_def.h"

static const char *TAG = "SONIC";

#define BUF_SIZE (512)
// #define DEBUG_SONIC_ENC_ISSUE
// #define SONIC_MEMORY_ANALYSIS

#ifdef DEBUG_SONIC_ENC_ISSUE
FILE *inone;
FILE *outone;
#endif

typedef struct {
    sonic_info_t sonic_info;
    int   emulate_chord_pitch;
    int   enable_nonlinear_speedup;
    int   quality;
    void  *sonic_handle;
    float volume;
    float rate;
    float pitch;
    float speed;
    int samplerate;
    int channel; 
    short *inbuf;
    short *outbuf;
} sonic_t;

static esp_err_t is_valid_sonic_samplerate(int samplerate)
{
    if (samplerate < 8000
        || samplerate > 48000) {
        ESP_LOGE(TAG, "The sample rate should be within range [8000,48000], here is %d Hz", samplerate);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t is_valid_sonic_channel(int channel)
{
    if (channel != 1 && channel != 2) {
        ESP_LOGE(TAG, "The number of channels should be either 1 or 2, here is %d", channel);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t sonic_set_info(audio_element_handle_t self, int rate, int ch)
{
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    if (sonic->sonic_info.samplerate == rate
        && sonic->sonic_info.channel == ch) {
        return ESP_OK;
    }
    if (is_valid_sonic_samplerate(rate) != ESP_OK
        || is_valid_sonic_channel(ch) != ESP_OK) {
        return ESP_FAIL;
    } else {
        sonic->sonic_info.samplerate = rate;
        sonic->sonic_info.channel = ch;
        ESP_LOGI(TAG, "reset sample rate of stream data : %d, reset channel of stream data : %d",
                 sonic->sonic_info.samplerate, sonic->sonic_info.channel);
    }
    return ESP_OK;
}

esp_err_t sonic_set_pitch_and_speed_info(audio_element_handle_t self, float pitch, float speed)
{
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    if ((pitch - 0 < 0.0001)
        && (speed - 0 < 0.0001)
        && (sonic->sonic_info.pitch - pitch < 0.0001)
        && (sonic->sonic_info.speed - speed < 0.0001)) {
        return ESP_OK;
    }
    if (sonic->sonic_info.pitch - pitch < 0.0001){
        if (pitch >= 0.1
            && pitch <= 10){
            sonic->sonic_info.pitch = pitch;
            ESP_LOGI(TAG, "reset pitch of stream data : %.2f", sonic->sonic_info.pitch);
        }else{
            ESP_LOGD(TAG, "The pitch must be in [0.1 10],reset pitch of stream data : %.2f", pitch);
            return ESP_FAIL;
        } 
    }
    if (sonic->sonic_info.speed - speed < 0.0001){
        if (speed >= 0.1
            && speed <= 10){
            sonic->sonic_info.speed = speed;   
            ESP_LOGI(TAG, "reset speed of stream data : %.2f", sonic->sonic_info.speed);
        }else{
            ESP_LOGD(TAG, "The speed must be in [0.1 10],reset speed of stream data : %.2f", speed);
            return ESP_FAIL;
        } 
    }  
    return ESP_OK;
}

static esp_err_t sonic_destroy(audio_element_handle_t self)
{
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    audio_free(sonic);
    return ESP_OK;
}

static esp_err_t sonic_open(audio_element_handle_t self)
{
#ifdef SONIC_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    ESP_LOGD(TAG, "sonic_open");
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    sonic->inbuf = (short *)calloc(sizeof(short), BUF_SIZE);
    if (sonic->inbuf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate input buffer");
        return ESP_FAIL;
    }
    sonic->outbuf = (short *)calloc(sizeof(short), BUF_SIZE);
    if (sonic->inbuf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate output buffer");
        return ESP_FAIL;
    }
    sonic->emulate_chord_pitch = 0;
    sonic->enable_nonlinear_speedup = 0;
    sonic->quality = 0;
    sonic->volume = SONIC_SET_VALUE_FOR_INITIALIZATION;
    sonic->rate = SONIC_SET_VALUE_FOR_INITIALIZATION;
    sonic->pitch = sonic->sonic_info.pitch;
    sonic->speed = sonic->sonic_info.speed;
    sonic->samplerate = sonic->sonic_info.samplerate;
    sonic->channel = sonic->sonic_info.channel;
    audio_element_info_t info = {0};
    audio_element_getinfo(self, &info);
    if (info.sample_rates && info.channels) {
        sonic->sonic_info.samplerate = info.sample_rates;
        sonic->sonic_info.channel = info.channels;
    }

    sonic->sonic_handle = esp_sonic_create_stream(sonic->sonic_info.samplerate, sonic->sonic_info.channel);
    esp_sonic_set_resample_mode(sonic->sonic_handle, sonic->sonic_info.resample_linear_interpolate);
    esp_sonic_set_speed(sonic->sonic_handle, sonic->sonic_info.speed);
    esp_sonic_set_pitch(sonic->sonic_handle, sonic->sonic_info.pitch);
    esp_sonic_set_rate(sonic->sonic_handle, sonic->rate);
    esp_sonic_set_volume(sonic->sonic_handle, sonic->volume);
    esp_sonic_set_chord_pitch(sonic->sonic_handle, sonic->emulate_chord_pitch);
    esp_sonic_set_quality(sonic->sonic_handle, sonic->quality);

#ifdef DEBUG_SONIC_ENC_ISSUE
    char fileName1[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '1', '.', 'p', 'c', 'm', '\0'};
    inone = fopen(fileName1, "rb");
    if (!inone) {
        perror(fileName1);
        return ESP_FAIL;
    }
    char fileName[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', '.', 'p', 'c', 'm', '\0'};
    outone = fopen(fileName, "w");
    if (!outone) {
        perror(fileName);
        return ESP_FAIL;
    }
#endif
    return ESP_OK;
}

static esp_err_t sonic_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "sonic_close");
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    if (sonic->sonic_handle != NULL) {
        esp_sonic_destroy_stream(sonic->sonic_handle);
    }
    if (sonic->inbuf != NULL) {
        audio_free(sonic->inbuf);
    }
    if (sonic->outbuf != NULL) {
        audio_free(sonic->outbuf);
    }
#ifdef SONIC_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
#ifdef DEBUG_SONIC_ENC_ISSUE
    fclose(inone);
    fclose(outone);
#endif
    return ESP_OK;
}

static int sonic_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    sonic_t *sonic = (sonic_t *)audio_element_getdata(self);
    if ((sonic->samplerate != sonic->sonic_info.samplerate)
        || (sonic->channel != sonic->sonic_info.channel)
        || (sonic->pitch != sonic->sonic_info.pitch)
        || (sonic->speed != sonic->sonic_info.speed)){
        sonic_close(self);
        sonic_open(self);
        return ESP_CODEC_ERR_CONTINUE;
    }
    int ret = 1;
    int samplesRead = 0;
    int samplesWritten = 0;
    int out_buffer_len = 0;
#ifdef DEBUG_SONIC_ENC_ISSUE
    samplesRead = fread(sonic->inbuf, sizeof(short), BUF_SIZE / sonic->sonic_info.channel, inone);
#else
    samplesRead = audio_element_input(self, (char *)sonic->inbuf, BUF_SIZE  * sizeof(short));
#endif
    if (samplesRead > 0) {
        samplesRead = samplesRead / (sonic->sonic_info.channel * sizeof(short));
        esp_sonic_write_to_stream(sonic->sonic_handle, sonic->inbuf, samplesRead );
        do {
            samplesWritten = esp_sonic_read_from_stream(sonic->sonic_handle, sonic->outbuf, BUF_SIZE / sonic->sonic_info.channel);
            if (samplesWritten > 0) {
#ifdef DEBUG_SONIC_ENC_ISSUE
                ret = fwrite(sonic->outbuf, sizeof(short), samplesWritten, outone);
#else
                out_buffer_len = samplesWritten * sonic->sonic_info.channel > BUF_SIZE ? BUF_SIZE : samplesWritten * sonic->sonic_info.channel;
                ret = audio_element_output(self, (char *)sonic->outbuf, out_buffer_len * sizeof(short));
#endif
            }
        } while (samplesWritten > 0);
    }else{
        ret = samplesRead;
    }

    return ret;
}

audio_element_handle_t sonic_init(sonic_cfg_t *config)
{
    sonic_t *sonic = audio_calloc(1, sizeof(sonic_t));
    mem_assert(sonic);
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = sonic_destroy;
    cfg.process = sonic_process;
    cfg.open = sonic_open;
    cfg.close = sonic_close;
    cfg.buffer_len = 0;
    cfg.tag = "sonic";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    audio_element_handle_t el = audio_element_init(&cfg);
    mem_assert(el);
    memcpy(sonic, config, sizeof(sonic_info_t));
    audio_element_setdata(el, sonic);
    audio_element_info_t info = {0};
    audio_element_setinfo(el, &info);
    ESP_LOGD(TAG, "sonic_init");
    return el;
}
