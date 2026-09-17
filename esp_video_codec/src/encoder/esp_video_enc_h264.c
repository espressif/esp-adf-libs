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

#include <sdkconfig.h>
#include "esp_video_enc.h"
#include "esp_video_enc_h264.h"
#include "esp_video_enc_reg.h"
#include "video_codec_utils.h"
#include "esp_h264_enc_single_hw.h"
#include "video_enc_hw_caps.h"
#include "esp_log.h"

#if CONFIG_VIDEO_ENCODER_HW_H264_DUAL_SUPPORT
#include <string.h>
#include "esp_h264_enc_dual_hw.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#endif

#define TAG "HW_H264_ENC"


#if CONFIG_VIDEO_ENCODER_HW_H264_DUAL_SUPPORT

#define HW_H264_MAX_INST             (2)
#define HW_H264_DUAL_SYNC_TIMEOUT_MS (1000)
#define HW_H264_PEER_SLOT(slot)      (1 - (slot))
#define HW_H264_EVT_OPEN_SYNC        (1 << 0)
#define HW_H264_EVT_DONE(slot)       (1 << (1 + (slot)))
#define HW_H264_EVT_ALL              (HW_H264_EVT_OPEN_SYNC | HW_H264_EVT_DONE(0) | HW_H264_EVT_DONE(1))

typedef struct hw_h264_s hw_h264_t;

typedef struct {
    SemaphoreHandle_t          mutex;
    EventGroupHandle_t         evt;
    hw_h264_t                 *inst[HW_H264_MAX_INST];
    esp_vc_err_t               process_ret[HW_H264_MAX_INST];
    esp_h264_enc_handle_t      single_handle;
    esp_h264_enc_dual_handle_t dual_handle;
} hw_h264_mgr_t;

struct hw_h264_s {
    uint8_t                     slot;
    esp_h264_enc_param_handle_t param_handle;
    esp_h264_enc_cfg_t          enc_cfg;
    esp_video_enc_in_frame_t   *pending_in;
    esp_video_enc_out_frame_t  *pending_out;
    bool                        frame_ready;
};

static hw_h264_mgr_t *s_mgr;
static int            s_mgr_users;
static bool           s_dual_sync;

#endif

static esp_video_codec_pixel_fmt_t h264_inputs[] = {
#if CONFIG_ESP_REV_MIN_FULL >= 300
    ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE,
#endif
    ESP_VIDEO_CODEC_PIXEL_FMT_O_UYY_E_VYY,
#if CONFIG_ESP_REV_MIN_FULL >= 300
    ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422,
    ESP_VIDEO_CODEC_PIXEL_FMT_BGR888,
#endif
};

static esp_video_codec_frame_type_t get_codec_frame_type(esp_h264_frame_type_t frame_type)
{
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

static esp_vc_err_t hw_h264_get_caps(esp_video_enc_caps_t *caps)
{
    caps->in_frame_align = video_codec_get_align_size(VIDEO_HW_ENC_H264_IN_FRAME_ALIGN);
    caps->out_frame_align = video_codec_get_align_size(VIDEO_HW_ENC_H264_OUT_FRAME_ALIGN);;
    caps->in_fmts = h264_inputs;
    caps->in_fmt_num = ELEMENTS_OF(h264_inputs);
    caps->set_caps = ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_BITRATE) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_QP) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_FPS) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_GOP) |
                     ESP_VIDEO_ENC_CAPS(ESP_VIDEO_ENC_SET_TYPE_FORCE_IDR);
    caps->typical_res.width = VIDEO_HW_ENC_H264_TYPICAL_WIDTH;
    caps->typical_res.height = VIDEO_HW_ENC_H264_TYPICAL_HEIGHT;
    caps->typical_fps = VIDEO_HW_ENC_H264_TYPICAL_FPS;
    caps->max_res.width = VIDEO_HW_ENC_H264_MAX_WIDTH;
    caps->max_res.height = VIDEO_HW_ENC_H264_MAX_HEIGHT;
    return ESP_VC_ERR_OK;
}

#if CONFIG_VIDEO_ENCODER_HW_H264_DUAL_SUPPORT
static hw_h264_mgr_t *mgr_load(void)
{
    return __atomic_load_n(&s_mgr, __ATOMIC_ACQUIRE);
}

static void mgr_lock(hw_h264_mgr_t *mgr)
{
    xSemaphoreTake(mgr->mutex, portMAX_DELAY);
}

static void mgr_unlock(hw_h264_mgr_t *mgr)
{
    xSemaphoreGive(mgr->mutex);
}

static hw_h264_mgr_t *mgr_enter(void)
{
    __atomic_add_fetch(&s_mgr_users, 1, __ATOMIC_ACQ_REL);
    hw_h264_mgr_t *mgr = mgr_load();
    if (mgr == NULL) {
        __atomic_sub_fetch(&s_mgr_users, 1, __ATOMIC_RELEASE);
        return NULL;
    }
    return mgr;
}

static void mgr_leave(void)
{
    __atomic_sub_fetch(&s_mgr_users, 1, __ATOMIC_RELEASE);
}

static void clear_param_handles(hw_h264_mgr_t *mgr)
{
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        if (mgr->inst[i]) {
            mgr->inst[i]->param_handle = NULL;
        }
    }
}

static void close_single_hw(hw_h264_mgr_t *mgr)
{
    if (mgr->single_handle == NULL) {
        return;
    }
    esp_h264_enc_close(mgr->single_handle);
    esp_h264_enc_del(mgr->single_handle);
    mgr->single_handle = NULL;
    clear_param_handles(mgr);
}

static void close_dual_hw(hw_h264_mgr_t *mgr)
{
    if (mgr->dual_handle == NULL) {
        return;
    }
    esp_h264_enc_dual_close(mgr->dual_handle);
    esp_h264_enc_dual_del(mgr->dual_handle);
    mgr->dual_handle = NULL;
    clear_param_handles(mgr);
}

static hw_h264_t *get_alive_inst(hw_h264_mgr_t *mgr)
{
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        if (mgr->inst[i]) {
            return mgr->inst[i];
        }
    }
    return NULL;
}

static int inst_num(hw_h264_mgr_t *mgr)
{
    int n = 0;
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        if (mgr->inst[i]) {
            n++;
        }
    }
    return n;
}

static void mgr_destroy_res(hw_h264_mgr_t *mgr)
{
    if (mgr == NULL) {
        return;
    }
    close_single_hw(mgr);
    close_dual_hw(mgr);
    if (mgr->mutex) {
        vSemaphoreDelete(mgr->mutex);
    }
    if (mgr->evt) {
        vEventGroupDelete(mgr->evt);
    }
    esp_video_codec_free(mgr);
}

static hw_h264_mgr_t *mgr_alloc(void)
{
    hw_h264_mgr_t *mgr = video_codec_calloc_struct(hw_h264_mgr_t);
    if (mgr == NULL) {
        return NULL;
    }
    mgr->mutex = xSemaphoreCreateMutex();
    mgr->evt = xEventGroupCreate();
    if (mgr->mutex == NULL || mgr->evt == NULL) {
        mgr_destroy_res(mgr);
        return NULL;
    }
    return mgr;
}

static hw_h264_mgr_t *mgr_enter_create(void)
{
    __atomic_add_fetch(&s_mgr_users, 1, __ATOMIC_ACQ_REL);
    hw_h264_mgr_t *mgr = mgr_load();
    if (mgr) {
        return mgr;
    }
    mgr = mgr_alloc();
    if (mgr == NULL) {
        __atomic_sub_fetch(&s_mgr_users, 1, __ATOMIC_RELEASE);
        return NULL;
    }
    hw_h264_mgr_t *expected = NULL;
    if (!__atomic_compare_exchange_n(&s_mgr, &expected, mgr, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        mgr_destroy_res(mgr);
        mgr = mgr_load();
        if (mgr == NULL) {
            __atomic_sub_fetch(&s_mgr_users, 1, __ATOMIC_RELEASE);
            return NULL;
        }
    }
    return mgr;
}

static void mgr_destroy_last(hw_h264_mgr_t *mgr)
{
    close_single_hw(mgr);
    close_dual_hw(mgr);
    xEventGroupClearBits(mgr->evt, HW_H264_EVT_ALL);
    __atomic_store_n(&s_mgr, NULL, __ATOMIC_RELEASE);
    mgr_unlock(mgr);
    while (__atomic_load_n(&s_mgr_users, __ATOMIC_ACQUIRE) > 1) {
        taskYIELD();
    }
    vSemaphoreDelete(mgr->mutex);
    vEventGroupDelete(mgr->evt);
    esp_video_codec_free(mgr);
}

static esp_vc_err_t map_h264_ret(esp_h264_err_t ret)
{
    if (ret == ESP_H264_ERR_OK) {
        return ESP_VC_ERR_OK;
    }
    if (ret == ESP_H264_ERR_OVERFLOW) {
        return ESP_VC_ERR_BUF_NOT_ENOUGH;
    }
    if (ret == ESP_H264_ERR_MEM) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    if (ret == ESP_H264_ERR_TIMEOUT) {
        return ESP_VC_ERR_TIMEOUT;
    }
    return ESP_VC_ERR_FAIL;
}

static void fill_enc_result(esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame,
                            esp_h264_enc_out_frame_t *enc_out)
{
    in_frame->consumed = in_frame->size;
    out_frame->encoded_size = enc_out->length;
    out_frame->frame_type = get_codec_frame_type(enc_out->frame_type);
    out_frame->pts = enc_out->pts;
    out_frame->dts = enc_out->dts;
}

static esp_vc_err_t open_single_hw(hw_h264_mgr_t *mgr, hw_h264_t *enc)
{
    esp_h264_err_t ret = esp_h264_enc_hw_new(&enc->enc_cfg, &mgr->single_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    ret = esp_h264_enc_open(mgr->single_handle);
    if (ret != ESP_H264_ERR_OK) {
        esp_h264_enc_del(mgr->single_handle);
        mgr->single_handle = NULL;
        return ESP_VC_ERR_INTERNAL_ERROR;
    }
    esp_h264_enc_hw_get_param_hd(mgr->single_handle, (esp_h264_enc_param_hw_handle_t *)&enc->param_handle);
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t open_dual_hw(hw_h264_mgr_t *mgr)
{
    if (mgr->inst[0] == NULL || mgr->inst[1] == NULL) {
        return ESP_VC_ERR_INVALID_STATE;
    }
    if (mgr->inst[0]->enc_cfg.gop != mgr->inst[1]->enc_cfg.gop) {
        ESP_LOGE(TAG, "Dual encode GOP mismatch %d vs %d",
                 (int)mgr->inst[0]->enc_cfg.gop, (int)mgr->inst[1]->enc_cfg.gop);
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    esp_h264_enc_cfg_dual_hw_t cfg = {
        .cfg0 = mgr->inst[0]->enc_cfg,
        .cfg1 = mgr->inst[1]->enc_cfg,
    };
    esp_h264_err_t ret = esp_h264_enc_dual_hw_new(&cfg, &mgr->dual_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    ret = esp_h264_enc_dual_open(mgr->dual_handle);
    if (ret != ESP_H264_ERR_OK) {
        esp_h264_enc_dual_del(mgr->dual_handle);
        mgr->dual_handle = NULL;
        return ESP_VC_ERR_INTERNAL_ERROR;
    }
    esp_h264_enc_param_hw_handle_t param0 = NULL;
    esp_h264_enc_param_hw_handle_t param1 = NULL;
    esp_h264_enc_dual_hw_get_param_hd0(mgr->dual_handle, &param0);
    esp_h264_enc_dual_hw_get_param_hd1(mgr->dual_handle, &param1);
    mgr->inst[0]->param_handle = (esp_h264_enc_param_handle_t)param0;
    mgr->inst[1]->param_handle = (esp_h264_enc_param_handle_t)param1;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t check_open(hw_h264_mgr_t *mgr)
{
    int n = inst_num(mgr);
    if (mgr->single_handle == NULL && mgr->dual_handle == NULL) {
        if (n == 1) {
            hw_h264_t *enc = get_alive_inst(mgr);
            if (enc == NULL) {
                return ESP_VC_ERR_INVALID_STATE;
            }
            return open_single_hw(mgr, enc);
        }
        if (n == 2) {
            return open_dual_hw(mgr);
        }
        return ESP_VC_ERR_INVALID_STATE;
    }
    if (n == 1 && mgr->dual_handle) {
        close_dual_hw(mgr);
        hw_h264_t *enc = get_alive_inst(mgr);
        if (enc == NULL) {
            return ESP_VC_ERR_INVALID_STATE;
        }
        return open_single_hw(mgr, enc);
    }
    if (n == 2 && mgr->single_handle) {
        close_single_hw(mgr);
        return open_dual_hw(mgr);
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t encode_single(hw_h264_mgr_t *mgr, esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame)
{
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
    esp_h264_err_t ret = esp_h264_enc_process(mgr->single_handle, &enc_in, &enc_out);
    if (ret == ESP_H264_ERR_OK) {
        fill_enc_result(in_frame, out_frame, &enc_out);
        return ESP_VC_ERR_OK;
    }
    return map_h264_ret(ret);
}

static esp_vc_err_t encode_dual(hw_h264_mgr_t *mgr)
{
    esp_h264_enc_in_frame_t enc_in[HW_H264_MAX_INST] = { 0 };
    esp_h264_enc_out_frame_t enc_out[HW_H264_MAX_INST] = { 0 };
    esp_h264_enc_in_frame_t *enc_in_ptr[HW_H264_MAX_INST] = { &enc_in[0], &enc_in[1] };
    esp_h264_enc_out_frame_t *enc_out_ptr[HW_H264_MAX_INST] = { &enc_out[0], &enc_out[1] };
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        hw_h264_t *inst = mgr->inst[i];
        if (inst == NULL || inst->pending_in == NULL || inst->pending_out == NULL) {
            return ESP_VC_ERR_INVALID_STATE;
        }
        enc_in[i].raw_data.buffer = inst->pending_in->data;
        enc_in[i].raw_data.len = inst->pending_in->size;
        enc_in[i].pts = inst->pending_in->pts;
        enc_out[i].raw_data.buffer = inst->pending_out->data;
        enc_out[i].raw_data.len = inst->pending_out->size;
    }
    esp_vc_err_t ret = map_h264_ret(esp_h264_enc_dual_process(mgr->dual_handle, enc_in_ptr, enc_out_ptr));
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        hw_h264_t *inst = mgr->inst[i];
        mgr->process_ret[i] = ret;
        if (ret == ESP_VC_ERR_OK) {
            fill_enc_result(inst->pending_in, inst->pending_out, &enc_out[i]);
        }
        inst->frame_ready = false;
        inst->pending_in = NULL;
        inst->pending_out = NULL;
    }
    return ret;
}

static void wakeup_slot(hw_h264_mgr_t *mgr, hw_h264_t *inst)
{
    if (inst == NULL || inst->frame_ready == false) {
        return;
    }
    inst->frame_ready = false;
    inst->pending_in = NULL;
    inst->pending_out = NULL;
    mgr->process_ret[inst->slot] = ESP_VC_ERR_INVALID_STATE;
    xEventGroupSetBits(mgr->evt, HW_H264_EVT_DONE(inst->slot));
}

static void wakeup_waiters_on_close(hw_h264_mgr_t *mgr, hw_h264_t *enc)
{
    wakeup_slot(mgr, enc);
    wakeup_slot(mgr, mgr->inst[HW_H264_PEER_SLOT(enc->slot)]);
}

static void hw_h264_fill_cfg(hw_h264_t *enc, esp_video_enc_cfg_t *cfg)
{
    enc->enc_cfg = (esp_h264_enc_cfg_t) {
        .pic_type = cfg->in_fmt,
        .gop = cfg->fps * 2,
        .fps = cfg->fps,
        .res = {
            .width = cfg->resolution.width,
            .height = cfg->resolution.height,
        },
        .rc = {
            .bitrate = cfg->resolution.width * cfg->resolution.height * cfg->fps / 20,
            .qp_min = 25,
            .qp_max = 35,
        }
    };
}

static inline bool alloc_empty_slot(hw_h264_mgr_t *mgr, uint8_t *slot)
{
    for (int i = 0; i < HW_H264_MAX_INST; i++) {
        if (mgr->inst[i] == NULL) {
            *slot = i;
            return true;
        }
    }
    return false;
}

static esp_vc_err_t hw_h264_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle)
{
    if (cfg == NULL || handle == NULL) {
        return ESP_VC_ERR_INVALID_ARG;
    }
    if (IS_SUPPORTED_VIDEO_FMT(cfg->in_fmt, h264_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported in format %s", esp_video_codec_get_pixel_fmt_str(cfg->in_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    hw_h264_t *enc = video_codec_calloc_struct(hw_h264_t);
    if (enc == NULL) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    hw_h264_mgr_t *mgr = NULL;
    bool locked = false;
    esp_vc_err_t ret = ESP_VC_ERR_OK;

    hw_h264_fill_cfg(enc, cfg);
    *handle = NULL;
    do {
        while (1) {
            mgr = mgr_enter_create();
            if (mgr == NULL) {
                ret = ESP_VC_ERR_NO_MEMORY;
                break;
            }
            mgr_lock(mgr);
            locked = true;
            if (mgr_load() == mgr) {
                break;
            }
            mgr_unlock(mgr);
            locked = false;
            mgr_leave();
            mgr = NULL;
        }
        if (ret != ESP_VC_ERR_OK) {
            break;
        }
        if (!alloc_empty_slot(mgr, &enc->slot)) {
            ESP_LOGE(TAG, "Hardware H264 encoder only supports %d instances", HW_H264_MAX_INST);
            ret = ESP_VC_ERR_NOT_SUPPORTED;
            break;
        }
        mgr->inst[enc->slot] = enc;
        int n = inst_num(mgr);
        bool wait_peer_open = (n == 1 && __atomic_load_n(&s_dual_sync, __ATOMIC_SEQ_CST));
        if (n == 2) {
            xEventGroupSetBits(mgr->evt, HW_H264_EVT_OPEN_SYNC);
        } else {
            mgr_unlock(mgr);
            if (wait_peer_open) {
                EventBits_t bits = xEventGroupWaitBits(mgr->evt, HW_H264_EVT_OPEN_SYNC, pdTRUE, pdTRUE,
                                                       pdMS_TO_TICKS(HW_H264_DUAL_SYNC_TIMEOUT_MS));
                if ((bits & HW_H264_EVT_OPEN_SYNC) == 0) {
                    ESP_LOGW(TAG, "Wait peer encoder open timeout, continue as single");
                }
            }
            mgr_lock(mgr);
            if (mgr_load() != mgr || mgr->inst[enc->slot] != enc) {
                ret = ESP_VC_ERR_INVALID_STATE;
                break;
            }
        }
        *handle = (esp_video_enc_handle_t)enc;
    } while (0);
    if (locked) {
        mgr_unlock(mgr);
    }
    if (mgr) {
        mgr_leave();
    }
    if (*handle == NULL) {
        esp_video_codec_free(enc);
    }
    return ret;
}

static esp_vc_err_t hw_h264_set(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size)
{
    hw_h264_t *enc = (hw_h264_t *)h;
    esp_h264_err_t h264_ret = ESP_H264_ERR_OK;
    esp_vc_err_t ret = ESP_VC_ERR_OK;
    (void)size;
    hw_h264_mgr_t *mgr = mgr_enter();
    if (mgr == NULL) {
        return ESP_VC_ERR_INVALID_STATE;
    }
    do {
        mgr_lock(mgr);
        if (mgr_load() != mgr || mgr->inst[enc->slot] != enc) {
            ret = ESP_VC_ERR_INVALID_STATE;
            break;
        }
        switch (type) {
            case ESP_VIDEO_ENC_SET_TYPE_QP: {
                if (enc->param_handle) {
                    ret = ESP_VC_ERR_INVALID_STATE;
                    break;
                }
                esp_video_enc_qp_set_t *qp_info = (esp_video_enc_qp_set_t *)data;
                enc->enc_cfg.rc.qp_min = qp_info->min_qp;
                enc->enc_cfg.rc.qp_max = qp_info->max_qp;
                break;
            }
            case ESP_VIDEO_ENC_SET_TYPE_BITRATE:
                if (enc->param_handle == NULL) {
                    enc->enc_cfg.rc.bitrate = *(uint32_t *)data;
                    break;
                }
                h264_ret = esp_h264_enc_set_bitrate(enc->param_handle, *(uint32_t *)data);
                break;
            case ESP_VIDEO_ENC_SET_TYPE_FPS:
                if (enc->param_handle == NULL) {
                    enc->enc_cfg.fps = (uint8_t) * (uint32_t *)data;
                    break;
                }
                h264_ret = esp_h264_enc_set_fps(enc->param_handle, (uint8_t) * (uint32_t *)data);
                break;
            case ESP_VIDEO_ENC_SET_TYPE_GOP:
                if (mgr->dual_handle) {
                    ESP_LOGE(TAG, "Not allow to set GOP when dual encode is running");
                    ret = ESP_VC_ERR_NOT_SUPPORTED;
                    break;
                }
                enc->enc_cfg.gop = (uint8_t) * (uint32_t *)data;
                if (enc->param_handle) {
                    h264_ret = esp_h264_enc_set_gop(enc->param_handle, enc->enc_cfg.gop);
                }
                break;
            case ESP_VIDEO_ENC_SET_TYPE_FORCE_IDR:
                if (enc->param_handle) {
                    h264_ret = esp_h264_enc_force_idr(enc->param_handle);
                }
                break;
            default:
                ret = ESP_VC_ERR_NOT_SUPPORTED;
                break;
        }
        if (ret == ESP_VC_ERR_OK && h264_ret != ESP_H264_ERR_OK) {
            ret = ESP_VC_ERR_NOT_SUPPORTED;
        }
    } while (0);
    mgr_unlock(mgr);
    mgr_leave();
    return ret;
}

static esp_vc_err_t hw_h264_encode(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame)
{
    hw_h264_t *enc = (hw_h264_t *)handle;
    uint8_t slot = enc->slot;
    bool locked = false;
    esp_vc_err_t ret = ESP_VC_ERR_INVALID_STATE;
    hw_h264_mgr_t *mgr = mgr_enter();
    if (mgr == NULL) {
        return ret;
    }
    do {
        mgr_lock(mgr);
        locked = true;
        if (mgr_load() != mgr || mgr->inst[slot] != enc) {
            break;
        }
        ret = check_open(mgr);
        if (ret != ESP_VC_ERR_OK) {
            break;
        }
        if (mgr->inst[HW_H264_PEER_SLOT(slot)] == NULL) {
            ret = encode_single(mgr, in_frame, out_frame);
            break;
        }

        enc->pending_in = in_frame;
        enc->pending_out = out_frame;
        enc->frame_ready = true;
        hw_h264_t *peer = mgr->inst[HW_H264_PEER_SLOT(slot)];
        if (peer && peer->frame_ready) {
            ret = encode_dual(mgr);
            xEventGroupSetBits(mgr->evt, HW_H264_EVT_DONE(peer->slot));
            break;
        }

        mgr_unlock(mgr);
        locked = false;
        EventBits_t bits = xEventGroupWaitBits(mgr->evt, HW_H264_EVT_DONE(slot), pdTRUE, pdTRUE,
                                               pdMS_TO_TICKS(HW_H264_DUAL_SYNC_TIMEOUT_MS));
        if (bits & HW_H264_EVT_DONE(slot)) {
            ret = mgr->process_ret[slot];
            break;
        }
        mgr_lock(mgr);
        locked = true;
        if (mgr_load() != mgr || mgr->inst[slot] != enc) {
            ret = mgr->process_ret[slot];
            break;
        }
        enc = mgr->inst[slot];
        if (enc->frame_ready == false) {
            xEventGroupClearBits(mgr->evt, HW_H264_EVT_DONE(slot));
            ret = mgr->process_ret[slot];
            break;
        }
        enc->frame_ready = false;
        enc->pending_in = NULL;
        enc->pending_out = NULL;
        ESP_LOGW(TAG, "Wait peer frame timeout");
        ret = ESP_VC_ERR_TIMEOUT;
    } while (0);
    if (locked) {
        mgr_unlock(mgr);
    }
    mgr_leave();
    return ret;
}

static esp_vc_err_t hw_h264_close(esp_video_enc_handle_t handle)
{
    hw_h264_t *enc = (hw_h264_t *)handle;
    bool locked = false;
    hw_h264_mgr_t *mgr = mgr_enter();
    do {
        if (mgr == NULL) {
            break;
        }
        mgr_lock(mgr);
        locked = true;
        if (mgr_load() != mgr || mgr->inst[enc->slot] != enc) {
            break;
        }
        wakeup_waiters_on_close(mgr, enc);
        mgr->inst[enc->slot] = NULL;
        int n = inst_num(mgr);
        if (n == 1) {
            close_dual_hw(mgr);
        } else if (n == 0) {
            mgr_destroy_last(mgr);
            locked = false;
        }
    } while (0);
    if (locked) {
        mgr_unlock(mgr);
    }
    if (mgr) {
        mgr_leave();
    }
    esp_video_codec_free(enc);
    return ESP_VC_ERR_OK;
}

void esp_video_enc_hw_dual_with_sync(bool enable)
{
    __atomic_store_n(&s_dual_sync, enable, __ATOMIC_SEQ_CST);
}

#else /* !CONFIG_VIDEO_ENCODER_HW_H264_DUAL_SUPPORT */

typedef struct {
    esp_h264_enc_handle_t       enc_handle;
    esp_h264_enc_param_handle_t param_handle;
    esp_h264_enc_cfg_t          enc_cfg;
} hw_h264_t;

static esp_vc_err_t open_h264(hw_h264_t *enc)
{
    esp_h264_err_t ret = esp_h264_enc_hw_new(&enc->enc_cfg, &enc->enc_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NO_MEMORY;
    }
    ret = esp_h264_enc_open(enc->enc_handle);
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_INTERNAL_ERROR;
    }
    esp_h264_enc_hw_get_param_hd(enc->enc_handle, (esp_h264_enc_param_hw_handle_t *)&enc->param_handle);
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_h264_open(esp_video_enc_cfg_t *cfg, esp_video_enc_handle_t *handle)
{
    if (IS_SUPPORTED_VIDEO_FMT(cfg->in_fmt, h264_inputs) == false) {
        ESP_LOGE(TAG, "Unsupported in format %s", esp_video_codec_get_pixel_fmt_str(cfg->in_fmt));
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    hw_h264_t *enc = video_codec_calloc_struct(hw_h264_t);
    VIDEO_CODEC_MEM_CHECK(enc);
    esp_h264_enc_cfg_t enc_cfg = {
        .pic_type = cfg->in_fmt,
        .gop = cfg->fps * 2,
        .fps = cfg->fps,
        .res = {
            .width = cfg->resolution.width,
            .height = cfg->resolution.height,
        },
        .rc = {
            .bitrate = cfg->resolution.width * cfg->resolution.height * cfg->fps / 20,
            .qp_min = 25,
            .qp_max = 35,
        }
    };
    enc->enc_cfg = enc_cfg;
    *handle = (esp_video_enc_handle_t)enc;
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_h264_set(esp_video_enc_handle_t h, esp_video_enc_set_type_t type, void *data, uint32_t size)
{
    hw_h264_t *enc = (hw_h264_t *)h;
    esp_h264_err_t ret = ESP_H264_ERR_OK;
    (void)size;
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
        case ESP_VIDEO_ENC_SET_TYPE_FORCE_IDR:
            if (enc->enc_handle == NULL) {
                return ESP_VC_ERR_OK;
            }
            ret = esp_h264_enc_force_idr(enc->param_handle);
            break;
        default:
            return ESP_VC_ERR_NOT_SUPPORTED;
    }
    if (ret != ESP_H264_ERR_OK) {
        return ESP_VC_ERR_NOT_SUPPORTED;
    }
    return ESP_VC_ERR_OK;
}

static esp_vc_err_t hw_h264_encode(esp_video_enc_handle_t handle, esp_video_enc_in_frame_t *in_frame, esp_video_enc_out_frame_t *out_frame)
{
    hw_h264_t *enc = (hw_h264_t *)handle;
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
    if (ret == ESP_H264_ERR_OK) {
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

static esp_vc_err_t hw_h264_close(esp_video_enc_handle_t handle)
{
    hw_h264_t *enc = (hw_h264_t *)handle;
    if (enc->enc_handle) {
        esp_h264_enc_close(enc->enc_handle);
        esp_h264_enc_del(enc->enc_handle);
        enc->enc_handle = NULL;
    }
    esp_video_codec_free(enc);
    return ESP_VC_ERR_OK;
}

void esp_video_enc_hw_dual_with_sync(bool enable)
{
    (void)enable;
}

#endif /* CONFIG_VIDEO_ENCODER_HW_H264_DUAL_SUPPORT */

esp_vc_err_t esp_video_enc_register_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_ENC_HW_H264_TAG,
    };
    static const esp_video_enc_ops_t h264_ops = {
        .get_caps = hw_h264_get_caps,
        .open = hw_h264_open,
        .set = hw_h264_set,
        .encode = hw_h264_encode,
        .close = hw_h264_close,
    };
    return esp_video_enc_register(&desc, &h264_ops);
}

esp_vc_err_t esp_video_enc_unregister_h264(void)
{
    esp_video_codec_desc_t desc = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_H264,
        .is_hw = true,
        .codec_cc = ESP_VIDEO_ENC_HW_H264_TAG,
    };
    return esp_video_enc_unregister(&desc);
}
