/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "esp_video_dec_h264.h"
#include "esp_video_dec_reg.h"
#include "video_codec_utils.h"
#include "esp_h264_dec.h"
#include "esp_h264_dec_sw.h"
#include "esp_log.h"

#define TAG "SW_H264_ENC"

#define TYPICAL_DECODE_WIDTH   640
#define TYPICAL_DECODE_HEIGHT  368
#define TYPICAL_DECODE_FPS     7
#define TYPICAL_FPS_REFER_FREQ 240

typedef struct {
    esp_h264_dec_handle_t          dec_handle;
    esp_h264_dec_param_sw_handle_t param_handle;
    uint8_t*                       last_dec_data;
    uint32_t                       last_consume_size;
    esp_h264_dec_out_frame_t       out_frame;
} sw_h264_t;

static esp_video_codec_pixel_fmt_t sw_h264_inputs[] = {
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P,
};

static esp_vc_err_t sw_h264_dec_get_caps(esp_video_dec_caps_t* caps)
{
    caps->in_frame_align = 1;
    caps->out_frame_align = 1;
    caps->out_fmts = sw_h264_inputs;
    caps->out_fmt_num = ELEMENTS_OF(sw_h264_inputs);
    caps->typical_res.width = TYPICAL_DECODE_WIDTH;
    caps->typical_res.height = TYPICAL_DECODE_HEIGHT;
    caps->typical_fps = video_codec_calc_typical_fps(TYPICAL_DECODE_FPS, TYPICAL_FPS_REFER_FREQ);
    caps->max_res.width = 0;
    caps->max_res.height = 0;
    return ESP_VC_ERR_OK;
}

static esp_video_codec_frame_type_t get_codec_frame_type(esp_h264_frame_type_t frame_type) {
    switch (frame_type) {
        case ESP_H264_FRAME_TYPE_IDR:
            return ESP_VIDEO_CODEC_FRAME_TYPE_IDR;
        case ESP_H264_FRAME_TYPE_I:
            return ESP_VIDEO_CODEC_FRAME_TYPE_I;
        case ESP_H264_FRAME_TYPE_P:
            return ESP_VIDEO_CODEC_FRAME_TYPE_P;
        default:
            return ESP_VIDEO_CODEC_FRAME_TYPE_NONE;
    }
}

static esp_vc_err_t sw_h264_dec_open(esp_video_dec_cfg_t* cfg, esp_video_dec_handle_t *h)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->out_fmt, sw_h264_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported out format %s", esp_video_codec_get_pixel_fmt_str(cfg->out_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    sw_h264_t* dec = video_codec_calloc_struct(sw_h264_t);
    VIDEO_CODEC_MEM_CHECK(dec);
    esp_h264_dec_cfg_t h264_cfg = {
        .pic_type = ESP_H264_RAW_FMT_I420,
    };
    do {
        esp_h264_dec_sw_new(&h264_cfg, &dec->dec_handle);
        if (dec->dec_handle == NULL) {
            break;
        }
        if (esp_h264_dec_open(dec->dec_handle) != ESP_H264_ERR_OK) {
            ESP_LOGE(TAG, "Fail to open h264 decoder");
            break;
        }
        esp_h264_dec_sw_get_param_hd(dec->dec_handle, &dec->param_handle);
        *h = (esp_video_dec_handle_t) dec;
        return ESP_VC_ERR_OK;
    } while (0);
    if (dec->dec_handle) {
        esp_h264_dec_del(dec->dec_handle);
    }
    esp_video_codec_free(dec);
    return ESP_VC_ERR_NO_MEMORY;
}

static void inline copy_output_frame(sw_h264_t* dec, esp_video_dec_out_frame_t* out_frame)
{
    memcpy(out_frame->data, dec->out_frame.outbuf, dec->out_frame.out_size);
    out_frame->pts = dec->out_frame.pts;
    out_frame->dts = dec->out_frame.dts;
    out_frame->decoded_size = dec->out_frame.out_size;
    out_frame->frame_type = get_codec_frame_type(dec->out_frame.frame_type);
    dec->out_frame.out_size = 0;
}

static esp_vc_err_t sw_h264_dec_decode(esp_video_dec_handle_t h, esp_video_dec_in_frame_t* in_frame, esp_video_dec_out_frame_t* out_frame)
{
    sw_h264_t* dec = (sw_h264_t*) h;
    // Have cached frame
    if (dec->out_frame.out_size > 0 && dec->last_dec_data == in_frame->data) {
        if (out_frame->size < dec->out_frame.out_size) {
            return ESP_VC_ERR_BUF_NOT_ENOUGH;
        }
        copy_output_frame(dec, out_frame);
        in_frame->consumed = dec->last_consume_size;
        return ESP_VC_ERR_OK;
    }
    esp_h264_dec_in_frame_t dec_in_frame = {
        .pts = in_frame->pts,
        .raw_data = {
            .buffer = in_frame->data,
            .len = in_frame->size,
        }
    };
    esp_h264_err_t ret = esp_h264_dec_process(dec->dec_handle, &dec_in_frame, &dec->out_frame);
    if (ret == ESP_H264_ERR_OVERFLOW) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_FAIL;
    }
    if (dec->out_frame.out_size > 0) {
        if (out_frame->size < dec->out_frame.out_size) {
            dec->last_consume_size = dec_in_frame.consume;
            dec->last_dec_data = dec_in_frame.raw_data.buffer;
            return ESP_VC_ERR_BUF_NOT_ENOUGH;
        }
        copy_output_frame(dec, out_frame);
        in_frame->consumed = dec_in_frame.consume;
    } else {
        out_frame->pts = dec->out_frame.pts;
        out_frame->decoded_size = 0;
        in_frame->consumed = dec_in_frame.consume;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_h264_dec_get_frame_info(esp_video_dec_handle_t h, esp_video_codec_frame_info_t* info)
{
    sw_h264_t* dec = (sw_h264_t*) h;
    esp_h264_resolution_t res = {0};
    esp_h264_dec_get_resolution(dec->param_handle, &res);
    info->res.width = res.width;
    info->res.height = res.height;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_h264_dec_set(esp_video_dec_handle_t h, esp_video_dec_set_type_t type, void* data, uint32_t size)
{
    return ESP_VC_ERR_NOT_SUPPORTED;
}
static esp_vc_err_t sw_h264_dec_close(esp_video_dec_handle_t h)
{
    sw_h264_t* dec = (sw_h264_t*) h;
    if (dec->dec_handle) {
        esp_h264_dec_close(dec->dec_handle);
        esp_h264_dec_del(dec->dec_handle);
        dec->dec_handle = NULL;
    }
    esp_video_codec_free(dec);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_dec_register_sw_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .codec_cc = ESP_VIDEO_DEC_SW_H264_TAG,
    };
    const static esp_video_dec_ops_t sw_h264_ops = {
        .get_caps = sw_h264_dec_get_caps,
        .open = sw_h264_dec_open,
        .decode = sw_h264_dec_decode,
        .get_frame_info = sw_h264_dec_get_frame_info,
        .set = sw_h264_dec_set,
        .close = sw_h264_dec_close,
    };
    return esp_video_dec_register(&desc, &sw_h264_ops);
}

esp_vc_err_t esp_video_dec_unregister_sw_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .codec_cc = ESP_VIDEO_DEC_SW_H264_TAG,
    };
    return esp_video_dec_unregister(&desc);
}