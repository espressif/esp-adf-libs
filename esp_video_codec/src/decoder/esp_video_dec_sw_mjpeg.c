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
#include "esp_jpeg_common.h"
#include "esp_jpeg_dec.h"
#include "esp_log.h"

#define TAG "SW_MJPEG_DEC"

#define MAX_SUPPORTED_WIDTH  800
#define MAX_SUPPORTED_HEIGHT 480

typedef struct {
    jpeg_dec_handle_t      dec_handle;
    jpeg_dec_config_t      dec_cfg;
    jpeg_dec_header_info_t jpeg_info;
    bool                   opened;
} sw_mjpeg_t;

static esp_video_codec_pixel_fmt_t sw_mjpeg[] = {
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB888,
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE,
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE,
};

static jpeg_pixel_format_t get_jpeg_pixel_format(esp_video_codec_pixel_fmt_t fmt)
{
    switch (fmt) {
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB888:
            return JPEG_PIXEL_FORMAT_RGB888;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE:
            return JPEG_PIXEL_FORMAT_RGB565_LE;
        case ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_BE:
            return JPEG_PIXEL_FORMAT_RGB565_BE;
        default:
            return 0;
    }
}

static esp_vc_err_t sw_mjpeg_dec_get_caps(esp_video_dec_caps_t *caps)
{
    caps->in_frame_align = 1;
    caps->out_frame_align = 16;
    caps->out_fmts = sw_mjpeg;
    caps->out_fmt_num = sizeof(sw_mjpeg) / sizeof(sw_mjpeg[0]);
    caps->set_caps = ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_ROTATE) |
                     ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_RESIZE) |
                     ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_CROP)   |
                     ESP_VIDEO_DEC_CAPS(ESP_VIDEO_DEC_SET_TYPE_BLOCK_DECODE);
    caps->typical_res.width = MAX_SUPPORTED_WIDTH;
    caps->typical_res.height = MAX_SUPPORTED_HEIGHT;
    caps->typical_fps = 10;
    caps->max_res.width = MAX_SUPPORTED_WIDTH;
    caps->max_res.height = MAX_SUPPORTED_HEIGHT;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t open_jpeg_dec(sw_mjpeg_t *dec)
{
    jpeg_error_t ret = jpeg_dec_open(&dec->dec_cfg, &dec->dec_handle);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "Fail to open JPEG decoder ret %d", ret);
        return ESP_VC_ERR_NO_MEMORY;
    }
    dec->opened = true;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_dec_open(esp_video_dec_cfg_t *cfg, esp_video_dec_handle_t *h)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->out_fmt, sw_mjpeg) == false) {
        ESP_LOGE(TAG, "Unsupported out format %s", esp_video_codec_get_pixel_fmt_str(cfg->out_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    sw_mjpeg_t *dec = video_codec_calloc_struct(sw_mjpeg_t);
    VIDEO_CODEC_MEM_CHECK(dec);
    jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
    config.output_type = get_jpeg_pixel_format(cfg->out_fmt);
    dec->dec_cfg = config;
    *h = dec;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_dec_decode(esp_video_dec_handle_t h, esp_video_dec_in_frame_t *in_frame, esp_video_dec_out_frame_t *out_frame)
{
    sw_mjpeg_t *dec = (sw_mjpeg_t *)h;
    if (in_frame->size == 0) {
        return ESP_VC_ERR_OK;
    }
    if (dec->opened == false) {
        esp_vc_err_t ret = open_jpeg_dec(dec);
        if (ret != ESP_VC_ERR_OK) {
            return ret;
        }
    }
    jpeg_dec_io_t jpeg_io = {
        .inbuf = in_frame->data,
        .inbuf_len = in_frame->size,
        .outbuf = out_frame->data,
    };
    // Always parse header firstly
    jpeg_error_t ret = jpeg_dec_parse_header(dec->dec_handle, &jpeg_io, &dec->jpeg_info);
    if (ret != JPEG_ERR_OK) {
        return ESP_VC_ERR_WRONG_DATA;
    }
    int loop_count = 0;
    jpeg_dec_get_process_count(dec->dec_handle, &loop_count);
    int out_frame_size = 0;
    jpeg_dec_get_outbuf_len(dec->dec_handle, &out_frame_size);
    // TODO try to get frame size
    if (out_frame->size < out_frame_size * loop_count) {
        ESP_LOGW(TAG, "Decode output need %d", out_frame_size * loop_count);
        return ESP_VC_ERR_BUF_NOT_ENOUGH;
    }
    for (int i = 0; i < loop_count; i++) {
        ret = jpeg_dec_process(dec->dec_handle, &jpeg_io);
        if (ret != JPEG_ERR_OK) {
            ESP_LOGE(TAG, "Fail to decode jpeg ret %d", ret);
            return ESP_VC_ERR_FAIL;
        }
        jpeg_io.inbuf += jpeg_io.inbuf_len - jpeg_io.inbuf_remain;
        jpeg_io.inbuf_len = jpeg_io.inbuf_remain;
    }
    // WORKAROUND to fix jpeg decoder bug
    if (jpeg_io.inbuf_remain < 4) {
        jpeg_io.inbuf_remain = 0;
    }
    in_frame->consumed = in_frame->size - jpeg_io.inbuf_remain;
    out_frame->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_I;
    out_frame->pts = in_frame->pts;
    out_frame->decoded_size = jpeg_io.out_size;
    return ESP_VC_ERR_OK;
}
static esp_vc_err_t sw_mjpeg_dec_get_frame_info(esp_video_dec_handle_t h, esp_video_codec_frame_info_t *info)
{
    sw_mjpeg_t *dec = (sw_mjpeg_t *)h;
    info->res.width = dec->jpeg_info.width;
    info->res.height = dec->jpeg_info.height;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t sw_mjpeg_dec_set(esp_video_dec_handle_t h, esp_video_dec_set_type_t type, void *data, uint32_t size)
{
    sw_mjpeg_t *dec = (sw_mjpeg_t *)h;
    if (dec->opened) {
        ESP_LOGW(TAG, "Not support set during decoding");
        return ESP_VC_ERR_INVALID_STATE;
    }
    switch (type) {
        case ESP_VIDEO_DEC_SET_TYPE_CROP: {
            esp_video_codec_rect_t *crop = (esp_video_codec_rect_t *)data;
            if (crop->x != 0 || crop->y != 0) {
                ESP_LOGW(TAG, "Only support crop top left corner");
                return ESP_VC_ERR_INVALID_ARG;
            }
            dec->dec_cfg.clipper.width = crop->width;
            dec->dec_cfg.clipper.height = crop->height;
            return ESP_VC_ERR_OK;
        }

        case ESP_VIDEO_DEC_SET_TYPE_RESIZE: {
            esp_video_codec_rect_t *resize = (esp_video_codec_rect_t *)data;
            if (resize->x != 0 || resize->y != 0) {
                ESP_LOGW(TAG, "Only support crop top left corner");
                return ESP_VC_ERR_INVALID_ARG;
            }
            dec->dec_cfg.scale.width = resize->width;
            dec->dec_cfg.scale.height = resize->height;
            return ESP_VC_ERR_OK;
        }

        case ESP_VIDEO_DEC_SET_TYPE_BLOCK_DECODE: {
            dec->dec_cfg.block_enable = *(bool *)data;
            return ESP_VC_ERR_OK;
        }
        case ESP_VIDEO_DEC_SET_TYPE_ROTATE: {
            uint32_t degree = *(uint32_t *)data;
            uint32_t enum_degree = degree / 90;
            if (enum_degree * 90 != degree || enum_degree > 3) {
                return ESP_VC_ERR_INVALID_ARG;
            }
            dec->dec_cfg.rotate = (jpeg_rotate_t)enum_degree;
            return ESP_VC_ERR_OK;
        }
        default:
            break;
    }
    return ESP_VC_ERR_NOT_SUPPORTED;
}

static esp_vc_err_t sw_mjpeg_dec_close(esp_video_dec_handle_t h)
{
    sw_mjpeg_t *dec = (sw_mjpeg_t *)h;
    if (dec->dec_handle) {
        jpeg_dec_close(dec->dec_handle);
        dec->dec_handle = NULL;
    }
    esp_video_codec_free(dec);
    return ESP_VC_ERR_OK;
}

esp_vc_err_t esp_video_dec_register_sw_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .codec_cc = ESP_VIDEO_DEC_SW_MJPEG_TAG,
    };
    const static esp_video_dec_ops_t sw_mjpeg_ops = {
        .get_caps = sw_mjpeg_dec_get_caps,
        .open = sw_mjpeg_dec_open,
        .decode = sw_mjpeg_dec_decode,
        .get_frame_info = sw_mjpeg_dec_get_frame_info,
        .set = sw_mjpeg_dec_set,
        .close = sw_mjpeg_dec_close,
    };
    return esp_video_dec_register(&desc, &sw_mjpeg_ops);
}

esp_vc_err_t esp_video_dec_unregister_sw_mjpeg(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .codec_cc = ESP_VIDEO_DEC_SW_MJPEG_TAG,
    };
    return esp_video_dec_unregister(&desc);
}