// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "esp_log.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "audio_element.h"
#include "filter_resample.h"
#include "audio_type_def.h"

static const char *TAG = "RSP_FILTER";

#define RESAMPLING_POINT_NUM 512
typedef struct rsp_filter {
    resample_info_t *resample_info;
    unsigned char *out_buf;
    unsigned char *in_buf;
    void *rsp_hd;
    int in_offset;
} rsp_filter_t;

static esp_err_t is_valid_rsp_filter_samplerate(int samplerate)
{
    if (samplerate < 8000
        || samplerate > 48000) {
        ESP_LOGE(TAG, "The sample rate should be within range [8000,48000], here is %d Hz", samplerate);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t is_valid_rsp_filter_channel(int channel)
{
    if (channel != 1 && channel != 2) {
        ESP_LOGE(TAG, "The number of channels should be either 1 or 2, here is %d", channel);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t rsp_filter_set_src_info(audio_element_handle_t self, int rate, int ch)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    if (filter->resample_info->src_rate == rate
        && filter->resample_info->src_ch == ch) {
        return ESP_OK;
    }
    if (is_valid_rsp_filter_samplerate(rate) != ESP_OK
        || is_valid_rsp_filter_channel(ch) != ESP_OK) {
        return ESP_FAIL;
    } else {
        filter->resample_info->src_rate = rate;
        filter->resample_info->src_ch = ch;
        ESP_LOGI(TAG, "reset sample rate of source data : %d, reset channel of source data : %d",
                 filter->resample_info->src_rate, filter->resample_info->src_ch);
    }
    return ESP_OK;
}

static esp_err_t rsp_filter_destroy(audio_element_handle_t self)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    if (filter != NULL
        &&filter->resample_info != NULL) {
        audio_free(filter->resample_info);
    }
    if (filter != NULL) {
        audio_free(filter);
    }
    return ESP_OK;
}

static esp_err_t rsp_filter_open(audio_element_handle_t self)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    resample_info_t *resample_info = filter->resample_info;
    if (resample_info->sample_bits != 16) {
        ESP_LOGE(TAG, "Currently, the only supported bit width is 16 bits.");
        return ESP_FAIL;
    }
    unsigned char p_in[1] = {NULL};
    unsigned char p_out[1] = {NULL};
    filter->in_buf = p_in;
    filter->out_buf = p_out;
    resample_info->max_indata_bytes = RESAMPLING_POINT_NUM * 2 * resample_info->src_ch;
    if (filter->resample_info->mode == RESAMPLE_DECODE_MODE) {
        resample_info->out_len_bytes = 0;
    } else if (filter->resample_info->mode == RESAMPLE_ENCODE_MODE) {
        resample_info->out_len_bytes = RESAMPLING_POINT_NUM / 4;
    }
    filter->in_offset = 0;
    filter->resample_info = resample_info;
    filter->rsp_hd = esp_resample_create((void *)filter->resample_info,
                                         (unsigned char **)&filter->in_buf, (unsigned char **)&filter->out_buf);
    if (filter->rsp_hd == NULL) {
        ESP_LOGE(TAG, "Failed to create the resample handler");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "sample rate of source data : %d, channel of source data : %d, sample rate of destination data : %d, channel of destination data : %d",
             filter->resample_info->src_rate, filter->resample_info->src_ch, filter->resample_info->dest_rate,
             filter->resample_info->dest_ch);
    return ESP_OK;
}

static esp_err_t rsp_filter_close(audio_element_handle_t self)
{
    ESP_LOGI(TAG, "rsp_filter_close");
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    if (filter->rsp_hd != NULL) {
        esp_resample_destroy(filter->rsp_hd);
    }
    return ESP_OK;
}

static int rsp_filter_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    int out_len = -1;
    int read_len = 0;
    int in_bytes_consumed = 0;
    if (filter->resample_info->mode == RESAMPLE_DECODE_MODE) {
        filter->in_offset = RESAMPLING_POINT_NUM * filter->resample_info->src_ch;
        if (filter->in_offset < 128 * filter->resample_info->src_ch) {
            return ESP_FAIL;
        }
        memset(filter->in_buf, 0, filter->in_offset);
        read_len = audio_element_input(self, (char *)filter->in_buf, filter->in_offset);
        if (read_len > 0) {
            in_bytes_consumed = esp_resample_run(filter->rsp_hd, filter->resample_info,
                                                (unsigned char *)filter->in_buf, (unsigned char *)filter->out_buf, filter->in_offset,
                                                &filter->resample_info->out_len_bytes);
            if (in_bytes_consumed < 0) {
                rsp_filter_close(self);
                rsp_filter_open(self);
                return ESP_CODEC_ERR_CONTINUE;
            }
            if (in_bytes_consumed != filter->in_offset) {
                return ESP_FAIL;
            }
        }

    } else {
        if (filter->in_offset < filter->resample_info->max_indata_bytes) {
            if (filter->in_offset > 0) {
                memmove(filter->in_buf, &filter->in_buf[filter->resample_info->max_indata_bytes - filter->in_offset], filter->in_offset);
            }
            read_len = audio_element_input(self, (char *)&filter->in_buf[filter->in_offset],
                                           filter->resample_info->max_indata_bytes - filter->in_offset);
            if (read_len > 0) {
                filter->in_offset += read_len;
            }            
        }
        if (read_len > 0){
            in_bytes_consumed = esp_resample_run(filter->rsp_hd, filter->resample_info,
                                                (unsigned char *)filter->in_buf, (unsigned char *)filter->out_buf, filter->in_offset,
                                                &filter->resample_info->out_len_bytes);
            if (in_bytes_consumed < 0) {
                rsp_filter_close(self);
                rsp_filter_open(self);
                return ESP_CODEC_ERR_CONTINUE;
            }
            filter->in_offset -= in_bytes_consumed;
        }
    }
    if (read_len > 0) {
        out_len = audio_element_output(self, (char *)filter->out_buf, filter->resample_info->out_len_bytes);
    } else {
        out_len = read_len;
    }

    return out_len;
}

audio_element_handle_t rsp_filter_init(rsp_filter_cfg_t *config)
{
    rsp_filter_t *filter = (rsp_filter_t *)calloc(1, sizeof(rsp_filter_t));
    if (filter == NULL) {
        ESP_LOGE(TAG, "The filter failed to allocate memory");
        return NULL;
    }
    resample_info_t *resample_info = calloc(1, sizeof(resample_info_t));
    if (resample_info == NULL) {
        ESP_LOGE(TAG, "The resample_info failed to allocate memory");
        free(filter);
        return NULL;
    }

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = rsp_filter_destroy;
    cfg.process = rsp_filter_process;
    cfg.open = rsp_filter_open;
    cfg.close = rsp_filter_close;
    cfg.buffer_len = 0;
    cfg.tag = "resample";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    audio_element_handle_t el = audio_element_init(&cfg);
    if (el == NULL) {
        free(filter);
        free(resample_info);
        return NULL;
    }
    memcpy(resample_info, config, sizeof(resample_info_t));
    filter->resample_info = resample_info;
    if (filter->resample_info->type == AUDIO_CODEC_TYPE_DECODER) {
        filter->resample_info->mode = RESAMPLE_DECODE_MODE;
        filter->resample_info->max_indata_bytes = RESAMPLING_POINT_NUM;
    } else {
        filter->resample_info->mode = RESAMPLE_ENCODE_MODE;
    }
    audio_element_setdata(el, filter);
    audio_element_info_t info = {0};
    audio_element_setinfo(el, &info);
    ESP_LOGD(TAG, "rsp_filter_init");
    return el;
}
