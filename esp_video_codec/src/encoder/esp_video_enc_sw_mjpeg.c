
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
#include "esp_jpeg_common.h"
#include "esp_jpeg_enc.h"
#include "esp_log.h"

#define TAG "VENC_SW_MJPEG"

#define TYPICAL_ENCODE_WIDTH   800
#define TYPICAL_ENCODE_HEIGHT  480
#define TYPICAL_ENCODE_FPS     10
#define TYPICAL_FPS_REFER_FREQ 240
#define DEFAULT_IMAGE_QUALITY  60

typedef struct {
    jpeg_enc_handle_t enc_handle;
    jpeg_enc_config_t enc_cfg;
} sw_mjpeg_enc_t;

// TODO add other format support
static esp_video_codec_pixel_fmt_t sw_mjpeg_inputs[] = {
    ESP_VIDEO_CODEC_PIXEL_FMT_YUV422,
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB888,
};

static jpeg_pixel_format_t get_jpeg_pixel_format(esp_video_codec_pixel_fmt_t fmt)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888:
            return JPEG_PIXEL_FORMAT_RGB888;
        case ESP_VIDEO_CODEC_PIXEL_FMT_YUV422:
            return JPEG_PIXEL_FORMAT_YCbYCr;
        default:
            return 0;
    }
}

static esp_vc_err_t sw_mjpeg_get_caps(esp_video_enc_caps_t *caps)
{
    caps->in_frame_align = 16;
    caps->out_frame_align = 1;
    caps->in_fmts = sw_mjpeg_inputs;
    caps->in_fmt_num = ELEMENTS_OF(sw_mjpeg_inputs);
    caps->set_caps = ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_ROTATE) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_QUALITY);
    caps->typical_res.width = TYPICAL_ENCODE_WIDTH;
    caps->typical_res.height = TYPICAL_ENCODE_HEIGHT;
    caps->typical_fps = video_codec_calc_typical_fps(TYPICAL_ENCODE_FPS, TYPICAL_FPS_REFER_FREQ);
    caps->max_res.width = 0;
    caps->max_res.height = 0;
    return 0;
}

static jpeg_subsampling_t get_chroma_subsampling(esp_video_codec_chroma_subsampling_t subsampling)
{
    switch (subsampling) {
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_444:
            return JPEG_SUBSAMPLE_444;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422:
            return JPEG_SUBSAMPLE_422;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_GRAY:
            return JPEG_SUBSAMPLE_GRAY;
        case ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_420:
        default:
            return JPEG_SUBSAMPLE_420;
    }
}

static esp_vc_err_t open_jpeg(sw_mjpeg_enc_t *enc)
{
    jpeg_error_t ret = jpeg_enc_open(&enc->enc_cfg, &enc->enc_handle);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "Fail to open jpeg encoder ret %d", ret);
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->in_fmt, sw_mjpeg_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported in format %s", esp_video_codec_get_pixel_fmt_str(cfg->in_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    sw_mjpeg_enc_t *enc = video_codec_calloc_struct(sw_mjpeg_enc_t);
    VIDEO_CODEC_MEM_CHECK(enc);
    jpeg_enc_config_t jpeg_enc_cfg = DEFAULT_JPEG_ENC_CONFIG();
    jpeg_enc_cfg.src_type = get_jpeg_pixel_format(cfg->in_fmt);
    jpeg_enc_cfg.subsampling = JPEG_SUBSAMPLE_420;
    jpeg_enc_cfg.quality = DEFAULT_IMAGE_QUALITY;
    jpeg_enc_cfg.width = cfg->resolution.width;
    jpeg_enc_cfg.height = cfg->resolution.height;
    jpeg_enc_cfg.rotate = JPEG_ROTATE_0D;
    jpeg_enc_cfg.task_enable = false;
    enc->enc_cfg = jpeg_enc_cfg;
    *handle = (esp_video_enc_handle_t)enc;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_set(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size)
{
    sw_mjpeg_enc_t *enc = (sw_mjpeg_enc_t *)h;
    if (enc->enc_handle && type != ESP_VIDEO_ENC_SET_TYPE_QUALITY) {
        return ESP_VC_ERR_INVALID_STATE;
    }
    switch (type) {
        case ESP_VIDEO_ENC_SET_TYPE_ROTATE: {
            uint32_t degree = *(uint32_t *)data;
            jpeg_rotate_t rotate = (jpeg_rotate_t)degree / 90;
            if (rotate > 3 || (uint32_t)rotate * 90 != degree) {
                ESP_LOGE(TAG, "Invalid rotate degree %d", (int)degree);
                return ESP_VC_ERR_INVALID_ARG;
            }
            enc->enc_cfg.rotate = rotate;
            break;
        }
        case ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING: {
            esp_video_codec_chroma_subsampling_t *subsampling = (esp_video_codec_chroma_subsampling_t *)data;
            enc->enc_cfg.subsampling = get_chroma_subsampling(*subsampling);
            break;
        }

        case ESP_VIDEO_ENC_SET_TYPE_QUALITY: {
            uint8_t *quality = (uint8_t *)data;
            jpeg_enc_set_quality(enc->enc_handle, *quality);
            break;
        }
        default:
            return ESP_VC_ERR_NOT_SUPPORTED;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_encode(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame)
{
    sw_mjpeg_enc_t *enc = (sw_mjpeg_enc_t *)handle;
    if (enc->enc_handle == NULL) {
        esp_vc_err_t ret = open_jpeg(enc);
        if (ret != ESP_VC_ERR_OK) {
            return ret;
        }
    }
    int out_size = 0;
    jpeg_error_t ret = jpeg_enc_process(enc->enc_handle, in_frame->data, in_frame->size, out_frame->data,
                                        out_frame->size, &out_size);
    if (ret != JPEG_ERR_OK) {
        return ESP_VC_ERR_FAIL;
    }
    in_frame->consumed = in_frame->size;
    out_frame->encoded_size = out_size;
    out_frame->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_I;
    out_frame->pts = in_frame->pts;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_close(esp_video_enc_handle_t handle)
{
    sw_mjpeg_enc_t *enc = (sw_mjpeg_enc_t *)handle;
    if (enc->enc_handle) {
        jpeg_enc_close(enc->enc_handle);
        enc->enc_handle = NULL;
    }
    esp_video_codec_free(enc);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_enc_register_sw_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .codec_cc = ESP_VIDEO_ENC_SW_MJPEG_TAG,
    };
    const static esp_video_enc_ops_t sw_mjpeg_ops = {
        .get_caps = sw_mjpeg_get_caps,
        .open = sw_mjpeg_open,
        .set = sw_mjpeg_set,
        .encode = sw_mjpeg_encode,
        .close = sw_mjpeg_close,
    };
    return esp_video_enc_register(&desc, &sw_mjpeg_ops);
}

esp_vc_err_t esp_video_enc_unregister_sw_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .codec_cc = ESP_VIDEO_ENC_SW_MJPEG_TAG,
    };
    return esp_video_enc_unregister(&desc);
}
