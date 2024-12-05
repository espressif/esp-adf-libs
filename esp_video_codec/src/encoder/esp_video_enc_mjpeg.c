
/**
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

#include "esp_video_enc_mjpeg.h"
#include "esp_video_enc_reg.h"
#include "video_codec_utils.h"
#include "driver/jpeg_encode.h"
#include "video_enc_hw_caps.h"
#include "esp_log.h"

#define TAG "ENC_MJPEG"

#define DEFAULT_ENC_TIME_OUT      100
#define DEFAULT_ENC_IMAGE_QUALITY 45

typedef struct {
    jpeg_encoder_handle_t jpeg_enc;
    jpeg_encode_cfg_t     jpeg_enc_cfg;
} hw_mjpeg_t;

static esp_video_codec_pixel_fmt_t hw_mjpeg_inputs[] = VIDEO_HW_ENC_MJPEG_IN_FMTS;

static esp_vc_err_t hw_mjpeg_get_caps(esp_video_enc_caps_t *caps)
{
    caps->in_frame_align = video_codec_get_align_size(VIDEO_HW_ENC_MJPEG_IN_FRAME_ALIGN);
    caps->out_frame_align = video_codec_get_align_size(VIDEO_HW_ENC_MJPEG_OUT_FRAME_ALIGN);;
    caps->in_fmts = hw_mjpeg_inputs;
    caps->in_fmt_num = ELEMENTS_OF(hw_mjpeg_inputs);
    caps->set_caps = ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_QUALITY);
    caps->typical_res.width = VIDEO_HW_ENC_MJPEG_TYPICAL_WIDTH;
    caps->typical_res.height = VIDEO_HW_ENC_MJPEG_TYPICAL_HEIGHT;
    caps->typical_fps = VIDEO_HW_ENC_MJPEG_TYPICAL_FPS;
    caps->max_res.width = VIDEO_HW_ENC_MJPEG_MAX_WIDTH;
    caps->max_res.height = VIDEO_HW_ENC_MJPEG_MAX_HEIGHT;
    return ESP_VC_ERR_OK;
}

static jpeg_dec_output_format_t get_jpeg_pixel_format(esp_video_codec_pixel_fmt_t fmt)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_BGR888:
            return JPEG_DECODE_OUT_FORMAT_RGB888;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
            return JPEG_DECODE_OUT_FORMAT_RGB565;
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422:
            return JPEG_DECODE_OUT_FORMAT_YUV422;
        default:
            return 0;
    }
}

static jpeg_down_sampling_type_t get_down_sampling(esp_video_codec_chroma_subsampling_t subsampling)
{
    switch (subsampling) {
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_444:
            return JPEG_DOWN_SAMPLING_YUV444;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422:
            return JPEG_DOWN_SAMPLING_YUV422;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_GRAY:
            return JPEG_DOWN_SAMPLING_GRAY;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_420:
        default:
            return JPEG_DOWN_SAMPLING_YUV420;
    }
}

static esp_vc_err_t hw_mjpeg_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->in_fmt, hw_mjpeg_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported in format %s", esp_video_codec_get_pixel_fmt_str(cfg->in_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    hw_mjpeg_t *enc = video_codec_calloc_struct(hw_mjpeg_t);
    VIDEO_CODEC_MEM_CHECK(enc);
    int time_out = cfg->fps ? DEFAULT_ENC_TIME_OUT : 2000 / cfg->fps;
    jpeg_encode_engine_cfg_t encode_eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = time_out,
    };
    jpeg_new_encoder_engine(&encode_eng_cfg, &enc->jpeg_enc);
    if (enc->jpeg_enc == NULL) {
        ESP_LOGE(TAG, "Failed to create jpeg encoder engine");
        esp_video_codec_free(enc);
        return ESP_VC_ERR_NO_MEMORY;
    }
    enc->jpeg_enc_cfg.src_type = get_jpeg_pixel_format(cfg->in_fmt);
    enc->jpeg_enc_cfg.sub_sample = JPEG_DOWN_SAMPLING_YUV422;
    enc->jpeg_enc_cfg.image_quality = DEFAULT_ENC_IMAGE_QUALITY;
    enc->jpeg_enc_cfg.width = cfg->resolution.width;
    enc->jpeg_enc_cfg.height = cfg->resolution.height;

    *handle = enc;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_set(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size)
{
    hw_mjpeg_t *enc = (hw_mjpeg_t *)h;
    switch (type) {
        case ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING: {
            enc->jpeg_enc_cfg.sub_sample = get_down_sampling(*(esp_video_codec_chroma_subsampling_t *)data);
            return ESP_VC_ERR_OK;
        }
        case ESP_VIDEO_ENC_SET_TYPE_QUALITY: {
            uint8_t *quality = (uint8_t *)data;
            enc->jpeg_enc_cfg.image_quality = *quality;
            return ESP_VC_ERR_OK;
        }
        default:
            break;
    }
    return ESP_VC_ERR_NOT_SUPPORTED;
}

static esp_vc_err_t hw_mjpeg_encode(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame,
                           esp_video_enc_out_frame_t *out_frame)
{
    hw_mjpeg_t *enc = (hw_mjpeg_t *)handle;
    esp_err_t ret = jpeg_encoder_process(enc->jpeg_enc, &enc->jpeg_enc_cfg,
                                         in_frame->data, in_frame->size, out_frame->data,
                                         out_frame->size, &out_frame->encoded_size);
    if (ret != ESP_OK) {
        return ESP_VC_ERR_FAIL;
    }
    in_frame->consumed = in_frame->size;
    out_frame->pts = in_frame->pts;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_close(esp_video_enc_handle_t handle)
{
    hw_mjpeg_t *enc = (hw_mjpeg_t *)handle;
    if (enc->jpeg_enc) {
        jpeg_del_encoder_engine(enc->jpeg_enc);
        enc->jpeg_enc = NULL;
    }
    esp_video_codec_free(enc);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_enc_register_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_ENC_HW_MJPEG_TAG,
    };
    static esp_video_enc_ops_t hw_mjpeg_ops = {
        .get_caps = hw_mjpeg_get_caps,
        .open = hw_mjpeg_open,
        .set = hw_mjpeg_set,
        .encode = hw_mjpeg_encode,
        .close = hw_mjpeg_close,
    };
    return esp_video_enc_register(&desc, &hw_mjpeg_ops);
}

esp_vc_err_t esp_video_enc_unregister_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_ENC_HW_MJPEG_TAG,
    };
    return esp_video_enc_unregister(&desc);
}
