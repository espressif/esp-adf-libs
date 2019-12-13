// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "esp_log.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "wav_decoder.h"
#include "wav_head.h"

static const char *TAG = "WAV_DECODER";
#define HEAD_SIZE (44)
typedef struct wav_decoder {
    bool parsed_header;
    bool is_opened;
} wav_decoder_t;

static esp_err_t _wav_decoder_destroy(audio_element_handle_t self)
{
    wav_decoder_t *wav = (wav_decoder_t *)audio_element_getdata(self);
    audio_free(wav);
    return ESP_OK;
}
static esp_err_t _wav_decoder_open(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "_wav_decoder_open");
    wav_decoder_t *wav = (wav_decoder_t *)audio_element_getdata(self);
    if (wav == NULL) {
        ESP_LOGE(TAG, "wav_codec_get failed, line %d", __LINE__);
        return ESP_FAIL;
    }
    audio_element_info_t info = {0};
    audio_element_getinfo(self, &info);
    if (info.byte_pos == 0) {
        audio_element_info_t wav_info = {0};
        audio_element_getinfo(self, &wav_info);
        wav_info.codec_fmt = AUDIO_CODEC_WAV;
        audio_element_setinfo(self, &wav_info);
        wav->parsed_header = false;
        wav->is_opened = true;
    }
    return ESP_OK;
}

static esp_err_t _wav_decoder_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "_wav_decoder_close");
    if (AEL_STATE_PAUSED != audio_element_get_state(self)) {
        audio_element_report_pos(self);
        audio_element_info_t info = {0};
        audio_element_getinfo(self, &info);
        info.byte_pos = 0;
        info.total_bytes = 0;
        audio_element_setinfo(self, &info);
        wav_decoder_t *wav = (wav_decoder_t *)audio_element_getdata(self);
        wav->is_opened = false;
    }
    return ESP_OK;
}

static int _wav_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    wav_decoder_t *wav = (wav_decoder_t *)audio_element_getdata(self);
    if (wav == NULL) {
        ESP_LOGE(TAG, "wav_codec_get failed, line %d", __LINE__);
        return ESP_FAIL;
    }
    audio_element_info_t audio_info = { 0 };
    wav_info_t info;
    int r_size = audio_element_input(self, in_buffer, in_len);
    int out_len = r_size;
    if (r_size > 0) {
        audio_element_getinfo(self, &audio_info);
        if (!wav->parsed_header) {
            if (wav_head_parser((const uint8_t *)in_buffer, r_size, &info) != ESP_OK) {
                ESP_LOGE(TAG, "Error parse wav header");
                return AEL_PROCESS_FAIL;
            }
            wav->parsed_header = true;
            int remain_data = r_size - info.dataShift;

            audio_info.sample_rates = info.sampleRate;
            audio_info.channels = info.channels;
            audio_info.bits = info.bits;
            audio_info.total_bytes = info.dataSize;
            audio_info.byte_pos = remain_data;
            audio_info.bps = info.sampleRate * info.channels * info.bits;

            audio_element_setinfo(self, &audio_info);
            audio_element_report_info(self);
            if (remain_data > 0) {
                audio_element_output(self, in_buffer + info.dataShift, remain_data);
                return remain_data;
            }
            return r_size;
        }
        if (audio_info.byte_pos + r_size >= audio_info.total_bytes) {
            out_len = audio_info.total_bytes - audio_info.byte_pos;
        }
        out_len = audio_element_output(self, in_buffer, out_len);
        audio_info.byte_pos += out_len;
        audio_element_setinfo(self, &audio_info);
    }
    if (out_len != r_size) {
        return ESP_OK;
    }
    return out_len;
}

esp_err_t wav_decoder_get_pos(audio_element_handle_t self, void *in_data, int in_size, void *out_data, int *out_size)
{
    wav_decoder_t *codec = (wav_decoder_t *)audio_element_getdata(self);
    if (codec == NULL) {
        ESP_LOGE(TAG, "codec handle is null, decoder may be has been stopped");
        return ESP_FAIL;
    }
    audio_element_info_t wav_info = {0};
    audio_element_getinfo(self, &wav_info);
    int time = *(int *)in_data;
    int pos = (time * wav_info.bps >> 3) + HEAD_SIZE;
    *(int *)out_data = pos;
    *out_size = sizeof(pos);
    return ESP_OK;
}

audio_element_handle_t wav_decoder_init(wav_decoder_cfg_t *config)
{
    wav_decoder_t *wav = audio_calloc(1, sizeof(wav_decoder_t));
    mem_assert(wav);
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = _wav_decoder_destroy;
    cfg.process = _wav_decoder_process;
    cfg.open = _wav_decoder_open;
    cfg.close = _wav_decoder_close;
    cfg.seek = wav_decoder_get_pos;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    if (cfg.task_stack == 0) {
        cfg.task_stack = WAV_DECODER_TASK_STACK;
    }
    cfg.tag = "wav";

    audio_element_handle_t el = audio_element_init(&cfg);
    mem_assert(el);
    audio_element_setdata(el, wav);
    ESP_LOGD(TAG, "wav_decoder_init");
    return el;
}
