/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD.>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_log.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "pcm_decoder.h"
#include "audio_error.h"

static const char *TAG = "PCM_DECODER";
typedef struct pcm_decoder {
    bool reserved;
} pcm_decoder_t;

static esp_err_t _pcm_decoder_destroy(audio_element_handle_t self)
{
    pcm_decoder_t *pcm = (pcm_decoder_t *)audio_element_getdata(self);
    audio_free(pcm);
    return ESP_OK;
}

static esp_err_t _pcm_decoder_open(audio_element_handle_t self)
{
    return ESP_OK;
}

static esp_err_t _pcm_decoder_close(audio_element_handle_t self)
{
    if (AEL_STATE_PAUSED != audio_element_get_state(self)) {
        audio_element_report_pos(self);
        audio_element_info_t info = {0};
        audio_element_getinfo(self, &info);
        info.byte_pos = 0;
        info.total_bytes = 0;
        audio_element_setinfo(self, &info);
    }
    return ESP_OK;
}

static int _pcm_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    audio_element_info_t audio_info = { 0 };
    int r_size = audio_element_input(self, in_buffer, in_len);
    int out_len = r_size;
    if (r_size > 0) {
        out_len = audio_element_output(self, in_buffer, out_len);
        audio_element_getinfo(self, &audio_info);
        audio_info.byte_pos += out_len;
        audio_element_setinfo(self, &audio_info);
    }
    if (out_len != r_size) {
        return ESP_OK;
    }
    return out_len;
}

esp_err_t pcm_decoder_get_pos(audio_element_handle_t self, void *in_data, int in_size, void *out_data, int *out_size)
{
    ESP_LOGI(TAG, "pcm_decoder_get_pos");
    *out_size = 0;
    return ESP_OK;
}

audio_element_handle_t pcm_decoder_init(pcm_decoder_cfg_t *config)
{
    pcm_decoder_t *pcm = audio_calloc(1, sizeof(pcm_decoder_t));
    AUDIO_MEM_CHECK(TAG, pcm, {return NULL;});
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = _pcm_decoder_destroy;
    cfg.process = _pcm_decoder_process;
    cfg.open = _pcm_decoder_open;
    cfg.close = _pcm_decoder_close;
    cfg.seek = pcm_decoder_get_pos;
    cfg.task_stack = PCM_DECODER_TASK_STACK;
    if (config) {
        if (config->task_stack) {
            cfg.task_stack = config->task_stack;
        }
        cfg.task_prio = config->task_prio;
        cfg.task_core = config->task_core;
        cfg.out_rb_size = config->out_rb_size;
    }
    cfg.tag = "pcm";

    audio_element_handle_t el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {audio_free(pcm); return NULL;});
    audio_element_setdata(el, pcm);
    ESP_LOGD(TAG, "pcm_decoder_init");
    return el;
}
