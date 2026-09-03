/**
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2026 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_video_dec.h"
#include "video_codec_utils.h"

#define TAG  "DEC_SW_PARSE"

#define H264_RBSP_MAX  128

#define H264_PARSE_ENABLED   (CONFIG_VIDEO_DECODER_SW_H264_SUPPORT || CONFIG_VIDEO_ENCODER_HW_H264_SUPPORT || CONFIG_VIDEO_ENCODER_SW_H264_SUPPORT)
#define MJPEG_PARSE_ENABLED  (CONFIG_VIDEO_DECODER_SW_MJPEG_SUPPORT || CONFIG_VIDEO_DECODER_HW_MJPEG_SUPPORT || CONFIG_VIDEO_ENCODER_SW_MJPEG_SUPPORT || CONFIG_VIDEO_ENCODER_HW_MJPEG_SUPPORT)

typedef esp_vc_err_t (*sw_parse_fn_t)(const esp_video_dec_in_frame_t *in, esp_video_dec_parsed_info_t *info);

typedef struct {
    esp_video_codec_type_t  codec_type;
    sw_parse_fn_t           parse;
} sw_parse_item_t;

typedef struct {
    const uint8_t *data;
    uint32_t       size;
    uint32_t       bit_pos;
} bs_t;

#if MJPEG_PARSE_ENABLED

static inline uint32_t rd16be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 8) | p[1];
}

static esp_vc_err_t parse_jpeg(const esp_video_dec_in_frame_t *in, esp_video_dec_parsed_info_t *info)
{
    const uint8_t *p = in->data;
    const uint8_t *end = p + in->size;
    if (in->size < 4 || p[0] != 0xFF || p[1] != 0xD8) {
        return ESP_VC_ERR_WRONG_DATA;
    }
    p += 2;
    while (p + 4 <= end) {
        if (*p != 0xFF) {
            p++;
            continue;
        }
        while (p < end && *p == 0xFF) {
            p++;
        }
        if (p >= end) {
            break;
        }
        uint8_t marker = *p++;
        if (marker == 0xD9 || marker == 0xDA) {
            break;
        }
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7) || marker == 0xD8) {
            continue;
        }
        if (p + 2 > end) {
            break;
        }
        uint32_t len = rd16be(p);
        if (len < 2 || p + len > end) {
            return ESP_VC_ERR_WRONG_DATA;
        }
        bool is_sof = (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC);
        if (is_sof) {
            if (len < 8) {
                return ESP_VC_ERR_WRONG_DATA;
            }
            uint32_t height = rd16be(p + 3);
            uint32_t width = rd16be(p + 5);
            uint8_t nf = p[7];
            if (width == 0 || height == 0) {
                return ESP_VC_ERR_WRONG_DATA;
            }
            info->res.width = width;
            info->res.height = height;
            info->codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG;
            info->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_I;
            if (nf == 1) {
                info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_GRAY;
            } else if (nf >= 3 && len >= (uint32_t)(8 + 3 * nf)) {
                uint8_t h = p[9] >> 4;
                uint8_t v = p[9] & 0x0F;
                if (h == 2 && v == 2) {
                    info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_420;
                } else if (h == 2 && v == 1) {
                    info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422;
                } else {
                    info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_444;
                }
            }
            return ESP_VC_ERR_OK;
        }
        p += len;
    }
    return ESP_VC_ERR_WRONG_DATA;
}
#endif

#if H264_PARSE_ENABLED
static uint32_t bs_u(bs_t *bs, int n)
{
    uint32_t v = 0;
    while (n-- > 0) {
        uint32_t byte = bs->bit_pos >> 3;
        if (byte >= bs->size) {
            return 0;
        }
        v = (v << 1) | ((bs->data[byte] >> (7 - (bs->bit_pos & 7))) & 1u);
        bs->bit_pos++;
    }
    return v;
}

static uint32_t bs_ue(bs_t *bs)
{
    int z = 0;
    while (((bs->bit_pos >> 3) < bs->size) && bs_u(bs, 1) == 0 && z < 31) {
        z++;
    }
    return z ? (((1u << z) - 1u) + bs_u(bs, z)) : 0;
}

static int32_t bs_se(bs_t *bs)
{
    uint32_t v = bs_ue(bs);
    return (v & 1u) ? (int32_t)((v + 1u) >> 1) : -(int32_t)(v >> 1);
}

static uint32_t rbsp_from_nal(const uint8_t *nal, uint32_t nal_size, uint8_t *out, uint32_t out_max)
{
    uint32_t o = 0;
    for (uint32_t i = 0; i < nal_size && o < out_max; i++) {
        if (i + 2 < nal_size && nal[i] == 0 && nal[i + 1] == 0 && nal[i + 2] == 3) {
            if (o + 2 > out_max) {
                break;
            }
            out[o++] = 0;
            out[o++] = 0;
            i += 2;
            continue;
        }
        out[o++] = nal[i];
    }
    return o;
}

static const uint8_t *find_start_code(const uint8_t *p, const uint8_t *end, int *sc_len)
{
    while (p + 3 <= end) {
        if (p[0] == 0 && p[1] == 0) {
            if (p[2] == 1) {
                *sc_len = 3;
                return p;
            }
            if (p + 4 <= end && p[2] == 0 && p[3] == 1) {
                *sc_len = 4;
                return p;
            }
        }
        p++;
    }
    return NULL;
}

static void parse_vui_fps(bs_t *bs, esp_video_dec_parsed_info_t *info)
{
    if (bs_u(bs, 1)) {  /* aspect_ratio_info_present_flag */
        uint32_t aspect = bs_u(bs, 8);
        if (aspect == 255) {
            bs_u(bs, 16);
            bs_u(bs, 16);
        }
    }
    if (bs_u(bs, 1)) {  /* overscan_info_present_flag */
        bs_u(bs, 1);
    }
    if (bs_u(bs, 1)) {  /* video_signal_type_present_flag */
        bs_u(bs, 3);
        if (bs_u(bs, 1)) {
            bs_u(bs, 8);
            bs_u(bs, 8);
            bs_u(bs, 8);
        }
    }
    if (bs_u(bs, 1)) {  /* chroma_loc_info_present_flag */
        bs_ue(bs);
        bs_ue(bs);
    }
    if (bs_u(bs, 1)) {  /* timing_info_present_flag */
        uint32_t num_units = bs_u(bs, 32);
        uint32_t time_scale = bs_u(bs, 32);
        bs_u(bs, 1);  /* fixed_frame_rate_flag */
        /* H.264: fps = time_scale / (2 * num_units_in_tick) for progressive */
        if (num_units > 0 && time_scale > 0) {
            uint32_t fps = time_scale / (2U * num_units);
            if (fps > 0 && fps <= 255) {
                info->fps = (uint8_t)fps;
            }
        }
    }
}

static esp_vc_err_t parse_sps_rbsp(const uint8_t *rbsp, uint32_t size, esp_video_dec_parsed_info_t *info)
{
    if (size < 4) {
        return ESP_VC_ERR_WRONG_DATA;
    }
    bs_t bs = {.data = rbsp, .size = size, .bit_pos = 0};
    uint8_t profile_idc = (uint8_t)bs_u(&bs, 8);
    bs_u(&bs, 8);
    bs_u(&bs, 8);
    bs_ue(&bs);

    uint32_t chroma_format_idc = 1;
    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128 || profile_idc == 138 || profile_idc == 139 || profile_idc == 134 ||
        profile_idc == 135) {
        chroma_format_idc = bs_ue(&bs);
        if (chroma_format_idc == 3) {
            bs_u(&bs, 1);
        }
        bs_ue(&bs);
        bs_ue(&bs);
        bs_u(&bs, 1);
        if (bs_u(&bs, 1)) {
            int n = (chroma_format_idc != 3) ? 8 : 12;
            for (int i = 0; i < n; i++) {
                if (!bs_u(&bs, 1)) {
                    continue;
                }
                int last = 8, next = 8;
                int list_size = (i < 6) ? 16 : 64;
                for (int j = 0; j < list_size; j++) {
                    if (next) {
                        next = (last + bs_se(&bs) + 256) % 256;
                    }
                    last = next ? next : last;
                }
            }
        }
    }

    bs_ue(&bs);
    uint32_t poc_type = bs_ue(&bs);
    if (poc_type == 0) {
        bs_ue(&bs);
    } else if (poc_type == 1) {
        bs_u(&bs, 1);
        bs_se(&bs);
        bs_se(&bs);
        uint32_t n = bs_ue(&bs);
        for (uint32_t i = 0; i < n && i < 255; i++) {
            bs_se(&bs);
        }
    }
    bs_ue(&bs);
    bs_u(&bs, 1);
    uint32_t pic_w = bs_ue(&bs) + 1;
    uint32_t pic_h = bs_ue(&bs) + 1;
    uint32_t frame_mbs_only = bs_u(&bs, 1);
    if (!frame_mbs_only) {
        bs_u(&bs, 1);
    }
    bs_u(&bs, 1);
    uint32_t crop_l = 0, crop_r = 0, crop_t = 0, crop_b = 0;
    if (bs_u(&bs, 1)) {
        crop_l = bs_ue(&bs);
        crop_r = bs_ue(&bs);
        crop_t = bs_ue(&bs);
        crop_b = bs_ue(&bs);
    }
    if (bs_u(&bs, 1)) {  /* vui_parameters_present_flag */
        parse_vui_fps(&bs, info);
    }

    uint32_t width = pic_w * 16;
    uint32_t height = (2 - frame_mbs_only) * pic_h * 16;
    uint32_t sub_w = (chroma_format_idc == 1 || chroma_format_idc == 2) ? 2 : 1;
    uint32_t sub_h = (chroma_format_idc == 1) ? 2 : 1;
    if (!frame_mbs_only) {
        sub_h *= 2;
    }
    width -= (crop_l + crop_r) * sub_w;
    height -= (crop_t + crop_b) * sub_h;
    if (width == 0 || height == 0) {
        return ESP_VC_ERR_WRONG_DATA;
    }

    info->res.width = width;
    info->res.height = height;
    info->codec_type = ESP_VIDEO_CODEC_TYPE_H264;
    if (chroma_format_idc == 0) {
        info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_GRAY;
    } else if (chroma_format_idc == 2) {
        info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422;
    } else if (chroma_format_idc == 3) {
        info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_444;
    } else {
        info->chroma_subsampling = ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_420;
    }
    return ESP_VC_ERR_OK;
}

static void parse_slice_type(const uint8_t *rbsp, uint32_t size, uint8_t nal_type, esp_video_dec_parsed_info_t *info)
{
    if (size == 0) {
        return;
    }
    if (nal_type == 5) {
        info->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_IDR;
        return;
    }
    bs_t bs = {.data = rbsp, .size = size, .bit_pos = 0};
    bs_ue(&bs);
    uint32_t slice_type = bs_ue(&bs) % 5;
    if (slice_type == 2 || slice_type == 4) {
        info->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_I;
    } else if (slice_type == 0 || slice_type == 3) {
        info->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_P;
    } else if (slice_type == 1) {
        info->frame_type = ESP_VIDEO_CODEC_FRAME_TYPE_B;
    }
}

static esp_vc_err_t parse_h264(const esp_video_dec_in_frame_t *in, esp_video_dec_parsed_info_t *info)
{
    const uint8_t *p = in->data;
    const uint8_t *end = p + in->size;
    bool got_info = false;
    uint8_t rbsp[H264_RBSP_MAX];

    while (p < end) {
        int sc_len = 0;
        const uint8_t *sc = find_start_code(p, end, &sc_len);
        if (sc == NULL) {
            break;
        }
        const uint8_t *nal = sc + sc_len;
        int next_sc_len = 0;
        const uint8_t *next = find_start_code(nal, end, &next_sc_len);
        const uint8_t *nal_end = next ? next : end;
        if (nal >= nal_end) {
            p = nal_end;
            continue;
        }
        uint8_t nal_type = nal[0] & 0x1F;
        uint32_t rbsp_size = rbsp_from_nal(nal + 1, (uint32_t)(nal_end - nal - 1), rbsp, sizeof(rbsp));
        if (nal_type == 7) {
            if (parse_sps_rbsp(rbsp, rbsp_size, info) == ESP_VC_ERR_OK) {
                got_info = true;
            }
        } else if (nal_type == 1 || nal_type == 5) {
            info->codec_type = ESP_VIDEO_CODEC_TYPE_H264;
            parse_slice_type(rbsp, rbsp_size, nal_type, info);
            if (info->frame_type != ESP_VIDEO_CODEC_FRAME_TYPE_NONE) {
                got_info = true;
            }
        }
        p = nal_end;
    }
    return got_info ? ESP_VC_ERR_OK : ESP_VC_ERR_WRONG_DATA;
}
#endif

#if MJPEG_PARSE_ENABLED || H264_PARSE_ENABLED
static const sw_parse_item_t s_builtin[] = {
#if MJPEG_PARSE_ENABLED
    {ESP_VIDEO_CODEC_TYPE_MJPEG, parse_jpeg},
#endif
#if H264_PARSE_ENABLED
    {ESP_VIDEO_CODEC_TYPE_H264, parse_h264},
#endif
};
#endif

esp_vc_err_t esp_video_dec_sw_parse(const esp_video_dec_in_frame_t *in_frame, esp_video_dec_parsed_info_t *info)
{
    VIDEO_CODEC_ARG_CHECK(in_frame == NULL || info == NULL || in_frame->data == NULL || in_frame->size == 0);
    memset(info, 0, sizeof(*info));

#if MJPEG_PARSE_ENABLED || H264_PARSE_ENABLED
    for (uint32_t i = 0; i < ELEMENTS_OF(s_builtin); i++) {
        if (s_builtin[i].parse(in_frame, info) == ESP_VC_ERR_OK) {
            if (info->codec_type == ESP_VIDEO_CODEC_TYPE_NONE) {
                info->codec_type = s_builtin[i].codec_type;
            }
            return ESP_VC_ERR_OK;
        }
        memset(info, 0, sizeof(*info));
    }
    return ESP_VC_ERR_WRONG_DATA;
#else
    return ESP_VC_ERR_NOT_SUPPORTED;
#endif
}
