
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

#include "esp_video_enc_h264.h"
#include "esp_video_enc_reg.h"
#include "video_codec_utils.h"
#include "esp_h264_enc_single_sw.h"
#include "esp_log.h"

#define TAG "VENC_SW_H264"

#define TYPICAL_ENCODE_WIDTH   640
#define TYPICAL_ENCODE_HEIGHT  480
#define TYPICAL_ENCODE_FPS     10
#define TYPICAL_FPS_REFER_FREQ 240

typedef struct {
    esp_h264_enc_handle_t          enc_handle;
    esp_h264_enc_param_handle_t    param_handle;
    esp_h264_enc_cfg_t             enc_cfg;
} sw_h264_t;

static esp_video_codec_pixel_fmt_t sw_h264_inputs[] = {
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P,
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV422,
};

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

static esp_vc_err_t sw_h264_get_caps(esp_video_enc_caps_t* caps)
{
    caps->in_frame_align = 1;
    caps->out_frame_align = 1;
    caps->in_fmts = sw_h264_inputs;
    caps->in_fmt_num = ELEMENTS_OF(sw_h264_inputs);
    caps->set_caps = ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_BITRATE) |
                    ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_QP) |
                    ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_FPS) |
                    ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_GOP);
    caps->typical_res.width = TYPICAL_ENCODE_WIDTH;
    caps->typical_res.height = TYPICAL_ENCODE_HEIGHT;
    caps->typical_fps = video_codec_calc_typical_fps(TYPICAL_ENCODE_FPS, TYPICAL_FPS_REFER_FREQ);
    caps->max_res.width = 0;
    caps->max_res.height = 0;
    return ESP_VC_ERR_OK;
}

static esp_h264_raw_format_t get_h264_raw_format(esp_video_codec_pixel_fmt_t fmt) {
    if (fmt == ESP_VIDEO_CODEC_PIXEL_FMT_YUV420P) {
        return ESP_H264_RAW_FMT_I420;
    } else if (fmt == ESP_VIDEO_CODEC_PIXEL_FMT_YUV422) {
        return ESP_H264_RAW_FMT_YUYV;
    }
    return ESP_H264_RAW_FMT_I420;
}

static esp_vc_err_t open_h264(sw_h264_t *enc)
{
    esp_h264_err_t ret = esp_h264_enc_sw_new(&enc->enc_cfg, &enc->enc_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    ret = esp_h264_enc_open(enc->enc_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_INTERNAL_ERROR;
    }
    esp_h264_enc_sw_get_param_hd(enc->enc_handle, (esp_h264_enc_param_sw_handle_t *)&enc->param_handle);
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_h264_open(esp_video_enc_cfg_t* cfg, esp_video_enc_handle_t *handle)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->in_fmt, sw_h264_inputs) == false) {
        ESP_LOGE(TAG, "In format %s not supported", esp_video_codec_get_pixel_fmt_str(cfg->in_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    sw_h264_t* enc = video_codec_calloc_struct(sw_h264_t);
    VIDEO_CODEC_MEM_CHECK(enc);
    esp_h264_enc_cfg_t enc_cfg = {
        .pic_type = get_h264_raw_format(cfg->in_fmt),
        .gop = cfg->fps * 2,
        .fps = cfg->fps,
        .res = {
            .width = cfg->resolution.width,
            .height = cfg->resolution.height,
        },
        .rc = {
            .bitrate = cfg->resolution.width * cfg->resolution.height * cfg->fps / 20,
            .qp_min = 25,
            .qp_max = 25,
        }
    };
    enc->enc_cfg = enc_cfg;
    *handle = (esp_video_enc_handle_t)enc;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_h264_set(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size)
{
    sw_h264_t *enc = (sw_h264_t *)h;
    esp_h264_err_t ret = ESP_H264_ERR_OK;
    switch (type) {
        case ESP_VIDEO_ENC_SET_TYPE_QP: {
            if (enc->enc_handle) {
                return ESP_VC_ERR_INVALID_STATE;
            }
            esp_video_enc_qp_set_t *qp_info = (esp_video_enc_qp_set_t *)data;
            enc->enc_cfg.rc.qp_min = qp_info->min_qp;
            enc->enc_cfg.rc.qp_max = qp_info->max_qp;
            break;
        }
        case ESP_VIDEO_ENC_SET_TYPE_BITRATE:
            if (enc->enc_handle == NULL) {
                enc->enc_cfg.rc.bitrate = *(uint32_t *)data;
                return ESP_VC_ERR_OK;
            }
            ret = esp_h264_enc_set_bitrate(enc->param_handle, *(uint32_t *)data);
            break;
        case ESP_VIDEO_ENC_SET_TYPE_FPS:
            if (enc->enc_handle == NULL) {
                enc->enc_cfg.fps = (uint8_t) * (uint32_t *)data;
                return ESP_VC_ERR_OK;
            }
            ret = esp_h264_enc_set_fps(enc->param_handle, (uint8_t) * (uint32_t *)data);
            break;
        case ESP_VIDEO_ENC_SET_TYPE_GOP:
            if (enc->enc_handle == NULL) {
                enc->enc_cfg.gop = (uint8_t) * (uint32_t *)data;
                return ESP_VC_ERR_OK;
            }
            ret = esp_h264_enc_set_gop(enc->param_handle, (uint8_t) * (uint32_t *)data);
            break;
        default:
            return ESP_VC_ERR_NOT_SUPPORTED;
    }
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_h264_encode(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t* in_frame, esp_video_enc_out_frame_t* out_frame)
{
    sw_h264_t* enc = (sw_h264_t*)handle;
    if (enc->enc_handle == NULL) {
        esp_vc_err_t ret = open_h264(enc);
        if (ret != ESP_VC_ERR_OK) {
            return ret;
        }
    }
    esp_h264_enc_in_frame_t enc_in = {
        .raw_data = {
            .buffer = in_frame->data,
            .len = in_frame->size,
        },
        .pts = in_frame->pts,
    };
    esp_h264_enc_out_frame_t enc_out = {
        .raw_data = {
            .buffer = out_frame->data,
            .len = out_frame->size,
        }
    };
    esp_h264_err_t ret = esp_h264_enc_process(enc->enc_handle, &enc_in, &enc_out);
    if (ret == ESP_H264_ERR_OK)  {
        // TODO should add consume in enc_in
        in_frame->consumed = in_frame->size;
        out_frame->encoded_size = enc_out.length;
        out_frame->frame_type = get_codec_frame_type(enc_out.frame_type);
        out_frame->pts = enc_out.pts;
        out_frame->dts = enc_out.dts;
        return ESP_VC_ERR_OK;
    }
    if (ret == ESP_H264_ERR_OVERFLOW) {
        return ESP_VC_ERR_BUF_NOT_ENOUGH;
    }
    return ESP_VC_ERR_FAIL;
}

static esp_vc_err_t sw_h264_close(esp_video_enc_handle_t handle)
{
    sw_h264_t* enc = (sw_h264_t*)handle;
    if (enc->enc_handle) {
        esp_h264_enc_close(enc->enc_handle);
        esp_h264_enc_del(enc->enc_handle);
        enc->enc_handle = NULL;
    }
    esp_video_codec_free(enc);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_enc_register_sw_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .codec_cc = ESP_VIDEO_ENC_SW_H264_TAG,
    };
    static esp_video_enc_ops_t sw_h264_ops = {
        .get_caps = sw_h264_get_caps,
        .open = sw_h264_open,
        .set = sw_h264_set,
        .encode = sw_h264_encode,
        .close = sw_h264_close,
    };
    return esp_video_enc_register(&desc, &sw_h264_ops);
}

esp_vc_err_t esp_video_enc_unregister_sw_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .codec_cc = ESP_VIDEO_ENC_SW_H264_TAG,
    };
    return esp_video_enc_unregister(&desc);
}
