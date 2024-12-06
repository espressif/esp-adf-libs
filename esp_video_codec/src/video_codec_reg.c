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

#include <stdbool.h>
#include <string.h>
#include "esp_video_dec_reg.h"
#include "esp_video_enc_reg.h"
#include "video_codec_utils.h"
#include "esp_log.h"

#define TAG "VID_CODEC_REG"

typedef struct video_codec_lib_list_t {
    struct video_codec_lib_list_t *next;
    esp_video_codec_desc_t         desc;
    const void                    *ops;
} video_codec_lib_list_t;

static video_codec_lib_list_t* video_dec_libs;
static video_codec_lib_list_t* video_enc_libs;

static video_codec_lib_list_t* lib_exists(video_codec_lib_list_t* libs, esp_video_codec_query_t* query)
{
    video_codec_lib_list_t* valid = NULL;
    while (libs) {
        if (libs->desc.codec_type == query->codec_type) {
            if (query->codec_cc) {
                if (query->codec_cc != libs->desc.codec_cc) {
                    libs = libs->next;
                    continue;
                }
                return libs;
            }
            valid = libs;
            if (libs->desc.is_hw) {
                return libs;
            }
        }
        libs = libs->next;
    }
    if (query->codec_cc == 0) {
        return valid;
    }
    return NULL;
}

static esp_vc_err_t add_libs(video_codec_lib_list_t** libs, esp_video_codec_desc_t *desc, const void* ops)
{
    VIDEO_CODEC_ARG_CHECK(desc->codec_type == ESP_VIDEO_CODEC_TYPE_NONE || ops == NULL);
    esp_video_codec_query_t query = {
        .codec_type = desc->codec_type,
        .codec_cc = desc->codec_cc
    };
    if (lib_exists(*libs, &query) != NULL) {
        ESP_LOGW(TAG, "Codec %s(%x) already existed", esp_video_codec_get_codec_str(desc->codec_type), (int)desc->codec_cc);
        return ESP_VC_ERR_OK;
    }
    video_codec_lib_list_t *item = video_codec_calloc_struct(video_codec_lib_list_t);
    VIDEO_CODEC_MEM_CHECK(item);
    item->desc = *desc;
    item->ops = ops;
    if (*libs == NULL) {
        *libs = item;
    } else {
        item->next = *libs;
        *libs = item;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t remove_libs(video_codec_lib_list_t** libs, esp_video_codec_desc_t *desc)
{
    VIDEO_CODEC_ARG_CHECK(desc->codec_type == ESP_VIDEO_CODEC_TYPE_NONE);
    if (*libs == NULL) {
        return ESP_VC_ERR_NOT_EXISTED;
    }
    video_codec_lib_list_t* pre = NULL;
    video_codec_lib_list_t* cur = *libs;
    while (cur) {
        if (cur->desc.codec_type == desc->codec_type && cur->desc.is_hw == desc->is_hw && cur->desc.codec_cc == desc->codec_cc) {
            if (pre == NULL) {
                *libs = cur->next;
            } else {
                pre->next = cur->next;
            }
            esp_video_codec_free(cur);
            return ESP_VC_ERR_OK;
        }
        pre = cur;
        cur = cur->next;
    }
    return ESP_VC_ERR_NOT_EXISTED;
}

static const void* get_ops(video_codec_lib_list_t* libs, esp_video_codec_query_t *query)
{
    if (libs == NULL) {
        return NULL;
    }
    video_codec_lib_list_t* item = lib_exists(libs, query);
    return item ? item->ops : NULL;
}

static esp_vc_err_t get_codec_desc(video_codec_lib_list_t* item, uint8_t idx, esp_video_codec_desc_t *desc)
{
    VIDEO_CODEC_ARG_CHECK(desc == NULL);
    uint8_t cur_idx = 0;
    while (item) {
        if (cur_idx == idx) {
            *desc = item->desc;
            return ESP_VC_ERR_OK;
        }
        cur_idx++;
        item = item->next;
    }
    return ESP_VC_ERR_NOT_EXISTED;
}

esp_vc_err_t esp_video_dec_register(esp_video_codec_desc_t *desc, const esp_video_dec_ops_t* ops)
{
    return add_libs(&video_dec_libs, desc, ops);
}

esp_vc_err_t esp_video_enc_register(esp_video_codec_desc_t *desc, const esp_video_enc_ops_t* ops)
{
    return add_libs(&video_enc_libs, desc, ops);
}

const esp_video_dec_ops_t* esp_video_dec_get_ops(esp_video_codec_query_t *query)
{
    return (const esp_video_dec_ops_t*)get_ops(video_dec_libs, query);
}

esp_vc_err_t esp_video_dec_get_desc(uint8_t index, esp_video_codec_desc_t *desc)
{
    return get_codec_desc(video_dec_libs, index, desc);
}

esp_vc_err_t esp_video_enc_get_desc(uint8_t index, esp_video_codec_desc_t *desc)
{
    return get_codec_desc(video_enc_libs, index, desc);
}

const esp_video_enc_ops_t* esp_video_enc_get_ops(esp_video_codec_query_t *query)
{
    return (const esp_video_enc_ops_t*)get_ops(video_enc_libs, query);
}

esp_vc_err_t esp_video_dec_unregister(esp_video_codec_desc_t *desc)
{
    return remove_libs(&video_dec_libs, desc);
}

esp_vc_err_t esp_video_enc_unregister(esp_video_codec_desc_t *desc)
{
    return remove_libs(&video_enc_libs, desc);
}
