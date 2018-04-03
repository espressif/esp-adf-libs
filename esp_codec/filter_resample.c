// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "esp_log.h"

#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "resample.h"
#include "filter_resample.h"

static const char *TAG = "RSP_FILTER";

#define RESAMPLING_POINT_NUM          512

typedef struct rsp_filter {
    int                     src_rate;
    int                     src_ch;
    int                     dest_rate;
    int                     dest_ch;
    audio_codec_type_t      type;
    esp_resample_handle_t   rsp_hd;
    char                    *out_buf;
    char                    *in_buf;
    int                     in_buf_size;
    int                     in_offset;
} rsp_filter_t;

static esp_err_t rsp_filter_destroy(audio_element_handle_t self)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    audio_free(filter);
    return ESP_OK;
}

static esp_err_t rsp_filter_open(audio_element_handle_t self)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    int out_buf_size = 0;
    int resample_mode = 0;
    if (filter->type == AUDIO_CODEC_TYPE_DECODER) {
        audio_element_info_t info = {0};
        audio_element_getinfo(self, &info);
        if (info.sample_rates && info.channels) {
            filter->src_rate = info.sample_rates;
            filter->src_ch = info.channels;
        }
        resample_mode = 0;
        filter->rsp_hd = resample_open(PCM_INOUT_NUM_RESTRICT, filter->src_rate,
                                       filter->dest_rate, filter->src_ch,
                                       filter->dest_ch, 0, resample_mode, &filter->in_buf_size, &out_buf_size);
        ESP_LOGI(TAG, "rsp_filter_open, decoder, src:%d,%d,dest:%d,%d,out_pcm:%d", filter->src_rate, filter->src_ch,
                 filter->dest_rate, filter->dest_ch, out_buf_size);
        filter->out_buf = audio_malloc(out_buf_size * sizeof(short) * filter->dest_ch);
        if (NULL == filter->out_buf) {
            ESP_LOGE(TAG, "Allocate memory failed,line:%d", __LINE__);
            return ESP_ERR_NO_MEM;
        }
        filter->in_buf_size = filter->in_buf_size * sizeof(short) * filter->src_ch;
        filter->in_buf = audio_malloc(filter->in_buf_size);
        if (NULL == filter->in_buf) {
            ESP_LOGE(TAG, "Allocate memory failed,line:%d", __LINE__);
            return ESP_ERR_NO_MEM;
        }
    } else if (filter->type == AUDIO_CODEC_TYPE_ENCODER) {
        resample_mode = 1;
        out_buf_size = RESAMPLING_POINT_NUM;
        filter->rsp_hd = resample_open(PCM_INOUT_NUM_RESTRICT, filter->src_rate,
                                       filter->dest_rate, filter->src_ch,
                                       filter->dest_ch, 0, resample_mode, &filter->in_buf_size, &out_buf_size);
        ESP_LOGI(TAG, "rsp_filter_open, encoder, src:%d,%d,dest:%d,%d,in_pcm:%d", filter->src_rate, filter->src_ch,
                 filter->dest_rate, filter->dest_ch, filter->in_buf_size);
        filter->out_buf = audio_malloc(out_buf_size * sizeof(short) * filter->dest_ch);
        if (NULL == filter->out_buf) {
            ESP_LOGE(TAG, "Allocate memory failed,line:%d", __LINE__);
            return ESP_ERR_NO_MEM;
        }
        filter->in_buf_size = filter->in_buf_size * sizeof(short) * filter->src_ch;
        filter->in_buf = audio_malloc(filter->in_buf_size);
        if (NULL == filter->in_buf) {
            ESP_LOGE(TAG, "Allocate memory failed,line:%d", __LINE__);
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

static esp_err_t rsp_filter_close(audio_element_handle_t self)
{
    ESP_LOGI(TAG, "rsp_filter_close");
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    resample_close(filter->rsp_hd);
    if (filter->out_buf) {
        audio_free(filter->out_buf);
    }
    if (filter->in_buf) {
        audio_free(filter->in_buf);
    }
    return ESP_OK;
}

static int rsp_filter_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    rsp_filter_t *filter = (rsp_filter_t *)audio_element_getdata(self);
    int out_len = -1;
    int out_pcm_sz = 0;
    int r_size = 0;
    if (filter->type == AUDIO_CODEC_TYPE_DECODER) {
        r_size = audio_element_input(self, filter->in_buf, filter->in_buf_size);
        if (r_size > 0) {
            out_len =  resample_process(filter->rsp_hd, (short *)filter->in_buf, (short *)filter->out_buf,
                                        filter->in_buf_size / sizeof(short) / filter->src_ch, &out_pcm_sz);
            ESP_LOGD(TAG, "DECODER,out:%d,out_pcm_sz:%d,in_len:%d", out_len, out_pcm_sz, in_len);
        }
    } else {
        r_size = audio_element_input(self, &filter->in_buf[filter->in_offset], filter->in_buf_size - filter->in_offset);
        out_pcm_sz = RESAMPLING_POINT_NUM;
        if (r_size > 0) {
            out_len =  resample_process(filter->rsp_hd, (short *)filter->in_buf, (short *)filter->out_buf,
                                        filter->in_buf_size / sizeof(short) / filter->src_ch, &out_pcm_sz);
            filter->in_offset = filter->in_buf_size - out_len * filter->src_ch * sizeof(short);
            memmove(filter->in_buf, &filter->in_buf[out_len * filter->src_ch * sizeof(short)], filter->in_offset);
            ESP_LOGD(TAG, "ENCODER,out:%d,out_pcm_sz:%d,in_len:%d,offset:%d",
                     out_len, out_pcm_sz, filter->in_buf_size, filter->in_offset);
        }
    }
    if (r_size > 0) {
        out_len = audio_element_output(self, (char *)filter->out_buf, out_pcm_sz * sizeof(short) * filter->dest_ch);
    } else {
        out_len = r_size;
    }
    return out_len;
}

audio_element_handle_t rsp_filter_init(rsp_filter_cfg_t *config)
{
    rsp_filter_t *filter = audio_calloc(1, sizeof(rsp_filter_t));
    mem_assert(filter);
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = rsp_filter_destroy;
    cfg.process = rsp_filter_process;
    cfg.open = rsp_filter_open;
    cfg.close = rsp_filter_close;
    cfg.buffer_len = 0;
    cfg.tag ="resample";
    audio_element_handle_t el = audio_element_init(&cfg);
    mem_assert(el);
    memcpy(filter, config, sizeof(rsp_filter_cfg_t));
    if (filter->type == AUDIO_CODEC_TYPE_DECODER) {
        filter->in_buf_size = RESAMPLING_POINT_NUM;
    }
    audio_element_setdata(el, filter);
    audio_element_info_t info = {0};
    audio_element_setinfo(el, &info);
    ESP_LOGD(TAG, "rsp_filter_init");
    return el;
}
