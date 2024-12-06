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

#include "esp_video_dec_mjpeg.h"
#include "esp_video_dec_reg.h"
#include "video_codec_utils.h"
#include "driver/jpeg_decode.h"
#include "video_dec_hw_caps.h"
#include "esp_log.h"

#define TAG "HW_JPEG_DEC"

#define DEFAULT_DECODE_TIMEOUT 100

typedef struct {
    jpeg_decoder_handle_t      dec_handle;
    jpeg_decode_cfg_t          dec_cfg;
    jpeg_decode_picture_info_t frame_info;
    uint32_t                   expect_out_size;
    bool                       header_parsed;
} hw_mjpeg_t;

static esp_video_codec_pixel_fmt_t hw_mjpeg_inputs[] = VIDEO_HW_DEC_MJPEG_OUT_FMTS;

static jpeg_dec_output_format_t get_jpeg_pixel_format(esp_video_codec_pixel_fmt_t fmt)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888:
            return JPEG_DECODE_OUT_FORMAT_RGB888;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE:
            return JPEG_DECODE_OUT_FORMAT_RGB565;
        case ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422:
            return JPEG_DECODE_OUT_FORMAT_YUV422;
        default:
            return 0;
    }
}

static esp_vc_err_t hw_mjpeg_dec_get_caps(esp_video_dec_caps_t *caps)
{
    caps->in_frame_align = video_codec_get_align_size(VIDEO_HW_DEC_MJPEG_IN_FRAME_ALIGN);
    caps->out_frame_align = video_codec_get_align_size(VIDEO_HW_DEC_MJPEG_OUT_FRAME_ALIGN);;
    caps->out_fmts = hw_mjpeg_inputs;
    caps->out_fmt_num = ELEMENTS_OF(hw_mjpeg_inputs);
    caps->typical_res.width = VIDEO_HW_DEC_MJPEG_TYPICAL_WIDTH;
    caps->typical_res.height = VIDEO_HW_DEC_MJPEG_TYPICAL_HEIGHT;
    caps->typical_fps = VIDEO_HW_DEC_MJPEG_TYPICAL_FPS;
    caps->max_res.width = VIDEO_HW_DEC_MJPEG_MAX_WIDTH;
    caps->max_res.height = VIDEO_HW_DEC_MJPEG_MAX_HEIGHT;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_dec_open(esp_video_dec_cfg_t *cfg, esp_video_dec_handle_t *h)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->out_fmt, hw_mjpeg_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported out format %s", esp_video_codec_get_pixel_fmt_str(cfg->out_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    hw_mjpeg_t *dec = video_codec_calloc_struct(hw_mjpeg_t);
    VIDEO_CODEC_MEM_CHECK(dec);
    jpeg_decode_engine_cfg_t eng_cfg = {
        .timeout_ms = DEFAULT_DECODE_TIMEOUT,
    };
    esp_err_t ret = jpeg_new_decoder_engine(&eng_cfg, &dec->dec_handle);
    if (dec->dec_handle == NULL) {
        ESP_LOGE(TAG, "Fail to create jpeg decoder ret %d", ret);
        esp_video_codec_free(dec);
        return ESP_VC_ERR_NO_MEMORY;
    }
    dec->dec_cfg.output_format = get_jpeg_pixel_format(cfg->out_fmt);
    if (cfg->out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE || cfg->out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_RGB888) {
        dec->dec_cfg.rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_RGB;
    } else {
        dec->dec_cfg.rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR;
    }
    *h = dec;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t get_pixel_bits(jpeg_dec_output_format_t fmt)
{
    if (fmt == JPEG_DECODE_OUT_FORMAT_RGB888 || fmt == JPEG_DECODE_OUT_FORMAT_YUV444) {
        return 24;
    } else if (fmt == JPEG_DECODE_OUT_FORMAT_YUV420) {
        return 12;
    }
    return 16;
}

static esp_vc_err_t parse_header(hw_mjpeg_t *dec, esp_video_dec_in_frame_t *in_frame)
{
    if (ESP_OK != jpeg_decoder_get_info(in_frame->data, in_frame->size, &dec->frame_info)) {
        return ESP_VC_ERR_WRONG_DATA;
    }
    dec->header_parsed = true;
    dec->frame_info.width = ALIGN_UP(dec->frame_info.width, 16);
    uint8_t bits = get_pixel_bits(dec->dec_cfg.output_format);
    dec->expect_out_size = dec->frame_info.width * dec->frame_info.height * bits / 8;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_dec_decode(esp_video_dec_handle_t h, esp_video_dec_in_frame_t *in_frame, esp_video_dec_out_frame_t *out_frame)
{
    hw_mjpeg_t *dec = (hw_mjpeg_t *)h;
    esp_vc_err_t ret = ESP_VC_ERR_OK;
    if (dec->header_parsed == false) {
        ret = parse_header(dec, in_frame);
        if (ret != ESP_VC_ERR_OK) {
            return ret;
        }
    }
    ret = jpeg_decoder_process(dec->dec_handle, &dec->dec_cfg,
                               in_frame->data, in_frame->size,
                               out_frame->data, out_frame->size, &out_frame->decoded_size);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_ARG && out_frame->size < out_frame->decoded_size) {
            return ESP_VC_ERR_BUF_NOT_ENOUGH;
        }
        return ESP_VC_ERR_FAIL;
    }
    // Update frame information if width or height changed
    if (out_frame->decoded_size != dec->expect_out_size) {
        parse_header(dec, in_frame);
    }
    in_frame->consumed = in_frame->size;
    out_frame->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_I;
    out_frame->pts = in_frame->pts;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_dec_get_frame_info(esp_video_dec_handle_t h, esp_video_codec_frame_info_t *info)
{
    hw_mjpeg_t *dec = (hw_mjpeg_t *)h;
    info->res.width = dec->frame_info.width;
    info->res.height = dec->frame_info.height;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_mjpeg_dec_set(esp_video_dec_handle_t h, esp_video_dec_set_type_t type, void *data, uint32_t size)
{
    return ESP_VC_ERR_NOT_SUPPORTED;
}

static esp_vc_err_t hw_mjpeg_dec_close(esp_video_dec_handle_t h)
{
    hw_mjpeg_t *dec = (hw_mjpeg_t *)h;
    if (dec->dec_handle) {
        jpeg_del_decoder_engine(dec->dec_handle);
        dec->dec_handle = NULL;
    }
    esp_video_codec_free(dec);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_dec_register_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_DEC_HW_MJPEG_TAG,
    };
    const static esp_video_dec_ops_t hw_mjpeg_ops = {
        .get_caps = hw_mjpeg_dec_get_caps,
        .open = hw_mjpeg_dec_open,
        .decode = hw_mjpeg_dec_decode,
        .get_frame_info = hw_mjpeg_dec_get_frame_info,
        .set = hw_mjpeg_dec_set,
        .close = hw_mjpeg_dec_close,
    };
    return esp_video_dec_register(&desc, &hw_mjpeg_ops);
}

esp_vc_err_t esp_video_dec_unregister_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_DEC_HW_MJPEG_TAG,
    };
    return esp_video_dec_unregister(&desc);
}
