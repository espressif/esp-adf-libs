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

#include "esp_video_dec.h"
#include "esp_video_dec_reg.h"
#include "video_codec_utils.h"
#include "esp_log.h"

#define TAG "VIDEO_DEC"

typedef struct {
    esp_video_dec_handle_t     dec_handle;
    const esp_video_dec_ops_t *ops;
} video_dec_t;

#define VIDEO_DEC_SIMPLE_SET(handle, set_type, param) {                      \
    VIDEO_CODEC_ARG_CHECK(handle);                                           \
    video_dec_t *dec = handle;                                               \
    return dec->ops->set(dec->dec_handle, set_type, &param, sizeof(param));  \
}

esp_vc_err_t esp_video_dec_query_caps(esp_video_codec_query_t *query, esp_video_dec_caps_t *caps)
{
    VIDEO_CODEC_ARG_CHECK(caps == NULL || query == NULL);
    const esp_video_dec_ops_t *ops = esp_video_dec_get_ops(query);
    if (ops == NULL || ops->get_caps == NULL) {
        return ESP_VC_ERR_NOT_EXISTED;
    }
    return ops->get_caps(caps);
}

esp_vc_err_t esp_video_dec_open(esp_video_dec_cfg_t *cfg, esp_video_dec_handle_t *handle)
{
    VIDEO_CODEC_ARG_CHECK(cfg == NULL || handle == NULL);
    esp_video_codec_query_t query = {
        .codec_type = cfg->codec_type,
        .codec_cc = cfg->codec_cc
    };
    const esp_video_dec_ops_t *ops = esp_video_dec_get_ops(&query);
    if (ops == NULL) {
        ESP_LOGE(TAG, "Decoder %s tag %x not existed", esp_video_codec_get_codec_str(cfg->codec_type), (int)cfg->codec_cc);
        return ESP_VC_ERR_NOT_EXISTED;
    }
    video_dec_t *dec = video_codec_calloc_struct(video_dec_t);
    VIDEO_CODEC_MEM_CHECK(dec);
    dec->ops = ops;
    esp_vc_err_t ret = ops->open(cfg, &dec->dec_handle);
    if (ret != ESP_VC_ERR_OK) {
        esp_video_codec_free(dec);
    } else {
        *handle = (esp_video_dec_handle_t)dec;
    }
    return ret;
}

esp_vc_err_t esp_video_dec_get_frame_align(esp_video_dec_handle_t handle, uint8_t *in_frame_align, uint8_t *out_frame_align)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL || in_frame_align == NULL || out_frame_align == NULL);
    esp_video_dec_caps_t caps = { 0 };
    video_dec_t *dec = handle;
    esp_vc_err_t ret = ESP_VC_ERR_NOT_SUPPORTED;
    if (dec->ops->get_caps) {
        ret = dec->ops->get_caps(&caps);
        if (ret == ESP_VC_ERR_OK) {
            *in_frame_align = caps.in_frame_align;
            *out_frame_align = caps.out_frame_align;
        }
    }
    return ret;
}

esp_vc_err_t esp_video_dec_process(esp_video_dec_handle_t handle, esp_video_dec_in_frame_t *in_frame,
                                   esp_video_dec_out_frame_t *out_frame)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL || in_frame == NULL || out_frame == NULL);
    video_dec_t *dec = handle;
    return dec->ops->decode(dec->dec_handle, in_frame, out_frame);
}

esp_vc_err_t esp_video_dec_get_frame_info(esp_video_dec_handle_t handle, esp_video_codec_frame_info_t *info)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL || info == NULL);
    video_dec_t *dec = handle;
    return dec->ops->get_frame_info(dec->dec_handle, info);
}

esp_vc_err_t esp_video_dec_set_rotate(esp_video_dec_handle_t handle, uint32_t degree)
{
    VIDEO_DEC_SIMPLE_SET(handle, ESP_VIDEO_DEC_SET_TYPE_ROTATE, degree);
}

esp_vc_err_t esp_video_dec_set_crop(esp_video_dec_handle_t handle, esp_video_codec_rect_t *rect)
{
    VIDEO_DEC_SIMPLE_SET(handle, ESP_VIDEO_DEC_SET_TYPE_CROP, *rect);
}

esp_vc_err_t esp_video_dec_set_resize(esp_video_dec_handle_t handle, esp_video_codec_resolution_t *dst_res)
{
    VIDEO_DEC_SIMPLE_SET(handle, ESP_VIDEO_DEC_SET_TYPE_RESIZE, *dst_res);
}

esp_vc_err_t esp_video_dec_set_block_decode(esp_video_dec_handle_t handle, bool enable_block_mode)
{
    VIDEO_DEC_SIMPLE_SET(handle, ESP_VIDEO_DEC_SET_TYPE_BLOCK_DECODE, enable_block_mode);
}

esp_vc_err_t esp_video_dec_close(esp_video_dec_handle_t handle)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL);
    video_dec_t *dec = handle;
    esp_vc_err_t ret = dec->ops->close(dec->dec_handle);
    esp_video_codec_free(dec);
    return ret;
}
