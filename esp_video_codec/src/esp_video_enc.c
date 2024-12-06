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

#include <stdlib.h>
#include "esp_video_enc.h"
#include "esp_video_enc_reg.h"
#include "video_codec_utils.h"
#include "esp_log.h"

#define TAG "VIDEO_ENC"

typedef struct {
    const esp_video_enc_ops_t *ops;
    esp_video_enc_handle_t     enc_handle;
} video_enc_t;

#define VIDEO_ENC_SIMPLE_SET(handle, set_type, param) {                      \
    VIDEO_CODEC_ARG_CHECK(handle == NULL);                                   \
    video_enc_t *enc = handle;                                               \
    return enc->ops->set(enc->enc_handle, set_type, &param, sizeof(param));  \
}

esp_vc_err_t esp_video_enc_query_caps(esp_video_codec_query_t *query, esp_video_enc_caps_t *caps)
{
    VIDEO_CODEC_ARG_CHECK(caps == NULL || query == NULL);
    const esp_video_enc_ops_t *ops = esp_video_enc_get_ops(query);
    if (ops == NULL || ops->get_caps == NULL) {
        return ESP_VC_ERR_NOT_EXISTED;
    }
    return ops->get_caps(caps);
}

esp_vc_err_t esp_video_enc_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle)
{
    VIDEO_CODEC_ARG_CHECK(cfg == NULL || handle == NULL);
     esp_video_codec_query_t query = {
        .codec_type = cfg->codec_type,
        .codec_cc = cfg->codec_cc
    };
    const esp_video_enc_ops_t *ops = esp_video_enc_get_ops(&query);
    if (ops == NULL) {
        ESP_LOGE(TAG, "Encoder %s tag %x not existed", esp_video_codec_get_codec_str(cfg->codec_type), (int)cfg->codec_cc);
        return ESP_VC_ERR_NOT_EXISTED;
    }
    video_enc_t *enc = video_codec_calloc_struct(video_enc_t);
    VIDEO_CODEC_MEM_CHECK(enc);
    enc->ops = ops;
    esp_vc_err_t ret = ops->open(cfg, &enc->enc_handle);
    if (ret != ESP_VC_ERR_OK) {
        esp_video_codec_free(enc);
    } else {
        *handle = (esp_video_enc_handle_t)enc;
    }
    return ret;
}

esp_vc_err_t esp_video_enc_get_frame_align(esp_video_enc_handle_t handle, uint8_t *in_frame_align, uint8_t *out_frame_align)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL || in_frame_align == NULL || out_frame_align == NULL);
    esp_video_enc_caps_t caps = { 0 };
    video_enc_t *enc = handle;
    esp_vc_err_t ret = ESP_VC_ERR_NOT_SUPPORTED;
    if (enc->ops->get_caps) {
        ret = enc->ops->get_caps(&caps);
        if (ret == ESP_VC_ERR_OK) {
            *in_frame_align = caps.in_frame_align;
            *out_frame_align = caps.out_frame_align;
        }
    }
    return ret;
}

esp_vc_err_t esp_video_enc_set_bitrate(esp_video_enc_handle_t handle, uint32_t bitrate)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_BITRATE, bitrate);
}

esp_vc_err_t esp_video_enc_set_qp(esp_video_enc_handle_t handle, uint32_t min_qp, uint32_t max_qp)
{
    if (min_qp > max_qp) {
        return ESP_VC_ERR_INVALID_ARG;
    }
    esp_video_enc_qp_set_t qp_set = {
        .min_qp = min_qp,
        .max_qp = max_qp
    };
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_QP, qp_set);
}

esp_vc_err_t esp_video_enc_set_quality(esp_video_enc_handle_t handle, uint8_t quality)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_QUALITY, quality);
}

esp_vc_err_t esp_video_enc_set_fps(esp_video_enc_handle_t handle, uint32_t fps)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_FPS, fps);
}

esp_vc_err_t esp_video_enc_set_gop(esp_video_enc_handle_t handle, uint32_t gop_frames)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_FPS, gop_frames);
}

esp_vc_err_t esp_video_enc_set_resend_sps_pps(esp_video_enc_handle_t handle, uint32_t gop_num)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_SPS_PPS_RESEND, gop_num);
}

esp_vc_err_t esp_video_enc_set_force_idr(esp_video_enc_handle_t handle)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL);
    video_enc_t *enc = handle;
    return enc->ops->set(enc->enc_handle, ESP_VIDEO_ENC_SET_TYPE_FORCE_IDR, NULL, 0);
}

esp_vc_err_t esp_video_enc_set_chroma_subsampling(esp_video_enc_handle_t handle, esp_video_codec_chroma_subsampling_t subsampling)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_CHROMA_SUBSAMPLING, subsampling);
}

esp_vc_err_t esp_video_enc_set_rotate(esp_video_enc_handle_t handle, uint32_t degree)
{
    VIDEO_ENC_SIMPLE_SET(handle, ESP_VIDEO_ENC_SET_TYPE_ROTATE, degree);
}

esp_vc_err_t esp_video_enc_process(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame,
                                   esp_video_enc_out_frame_t *out_frame)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL || in_frame == NULL || out_frame == NULL);
    video_enc_t *enc = handle;
    return enc->ops->encode(enc->enc_handle, in_frame, out_frame);
}

esp_vc_err_t esp_video_enc_close(esp_video_enc_handle_t handle)
{
    VIDEO_CODEC_ARG_CHECK(handle == NULL);
    video_enc_t *enc = handle;
    esp_vc_err_t ret = enc->ops->close(enc->enc_handle);
    esp_video_codec_free(enc);
    return ret;
}
