/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_heap_caps.h"
#if CONFIG_ESP_EFC_PERF_LOG
#include "esp_log.h"
#include "esp_timer.h"
#endif
#if CONFIG_SOC_PPA_SUPPORTED
#include "driver/ppa.h"
#endif
#include "lv_efc_private.h"
#include "efc_codec.h"
#include "src/core/lv_obj_class_private.h"
#include "src/misc/cache/instance/lv_image_cache.h"

#define MY_CLASS  (&lv_efc_class)

/* Do not advance faster than LVGL can present frames. Advancing a short loop
 * multiple times between refreshes can make LVGL repeatedly draw the same
 * frame (for example, frame 0 of a four-frame clip). */
#define EFC_FAST_MS         LV_DEF_REFR_PERIOD
#define EFC_PPA_SCALE_FRAG  16
#define EFC_PPA_ALIGN       128

#if CONFIG_ESP_EFC_PERF_LOG
static const char *TAG = "LV_EFC";
#endif

static void lv_efc_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_efc_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void next_frame_cb(lv_timer_t *t);
static bool load_blob(lv_efc_t *efc, const uint8_t *data, size_t len);
static bool decode_frame(lv_efc_t *efc);
static void release_clip(lv_efc_t *efc);
static void note_fps(lv_efc_t *efc);
#if CONFIG_ESP_EFC_PERF_LOG
static void reset_perf(lv_efc_t *efc);
static void note_perf(lv_efc_t *efc, int64_t decode_us, int64_t present_us, int64_t publish_us);
#endif
static bool present(lv_obj_t *obj);
static void publish(lv_obj_t *obj, bool force_src, bool changed);

const lv_obj_class_t lv_efc_class = {
    .constructor_cb = lv_efc_constructor,
    .destructor_cb  = lv_efc_destructor,
    .instance_size  = sizeof(lv_efc_t),
    .base_class     = &lv_image_class,
    .name           = "lv_efc",
};

static void *efc_malloc(size_t n)
{
    void *p = NULL;
#if CONFIG_SPIRAM
    p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
    if (!p) {
        p = malloc(n);
    }
    return p;
}

static bool read_file(const char *path, uint8_t **out, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long n = ftell(f);
    if (n <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }
    uint8_t *buf = efc_malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return false;
    }
    fclose(f);
    *out = buf;
    *out_len = (size_t)n;
    return true;
}

static bool read_lv_fs(const char *path, uint8_t **out, size_t *out_len)
{
    lv_fs_file_t f;
    if (lv_fs_open(&f, path, LV_FS_MODE_RD) != LV_FS_RES_OK) {
        return false;
    }
    uint32_t file_size = 0;
    if (lv_fs_seek(&f, 0, LV_FS_SEEK_END) != LV_FS_RES_OK ||
        lv_fs_tell(&f, &file_size) != LV_FS_RES_OK ||
        lv_fs_seek(&f, 0, LV_FS_SEEK_SET) != LV_FS_RES_OK ||
        file_size == 0) {
        lv_fs_close(&f);
        return false;
    }
    uint8_t *buf = efc_malloc(file_size);
    uint32_t got = 0;
    lv_fs_res_t res = buf ? lv_fs_read(&f, buf, file_size, &got) : LV_FS_RES_UNKNOWN;
    lv_fs_close(&f);
    if (!buf || res != LV_FS_RES_OK || got != file_size) {
        free(buf);
        return false;
    }
    *out = buf;
    *out_len = file_size;
    return true;
}

lv_obj_t *lv_efc_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

static void show_loaded(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    present(obj);
    publish(obj, true, true);
    efc->last_call = lv_tick_get();
    efc->fps_stamp = efc->last_call;
    efc->fps_frames = 0;
#if CONFIG_ESP_EFC_PERF_LOG
    reset_perf(efc);
#endif
    lv_timer_resume(efc->timer);
    lv_timer_reset(efc->timer);
}

void lv_efc_set_src(lv_obj_t *obj, const void *src)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (!src) {
        return;
    }
    release_clip(efc);

    uint8_t *owned = NULL;
    size_t len = 0;
    if (lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t *dsc = src;
        owned = efc_malloc(dsc->data_size);
        if (!owned) {
            return;
        }
        memcpy(owned, dsc->data, dsc->data_size);
        len = dsc->data_size;
    } else {
        const char *path = src;
        bool ok = (path[0] == '/') ? read_file(path, &owned, &len) : read_lv_fs(path, &owned, &len);
        if (!ok) {
            LV_LOG_ERROR("efc: cannot read %s", path);
            return;
        }
    }
    efc->blob = owned;
    efc->blob_len = len;
    if (!load_blob(efc, owned, len)) {
        /* load_blob already frees the blob when it got as far as a decoder. */
        free(efc->blob);
        efc->blob = NULL;
        LV_LOG_WARN("efc: source is not a playable clip");
        return;
    }
    show_loaded(obj);
}

void lv_efc_set_src_data(lv_obj_t *obj, const void *data, size_t len)
{
    if (!data || len == 0) {
        return;
    }
    lv_image_dsc_t dsc;
    memset(&dsc, 0, sizeof(dsc));
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_RAW;
    dsc.header.w = 1;
    dsc.header.h = 1;
    dsc.data_size = len;
    dsc.data = data;
    lv_efc_set_src(obj, &dsc);
}

void lv_efc_play(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (!efc->dec) {
        return;
    }
    efc->current_frame = 0;
    decode_frame(efc);
    efc->fps_stamp = lv_tick_get();
    efc->fps_frames = 0;
    efc->last_call = efc->fps_stamp;
#if CONFIG_ESP_EFC_PERF_LOG
    reset_perf(efc);
#endif
    publish(obj, false, present(obj));
    lv_timer_resume(efc->timer);
    lv_timer_reset(efc->timer);
}

void lv_efc_pause(lv_obj_t *obj)
{
    lv_timer_pause(((lv_efc_t *)obj)->timer);
}

void lv_efc_resume(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (!efc->dec) {
        return;
    }
    efc->fps_stamp = lv_tick_get();
    efc->fps_frames = 0;
    efc->last_call = efc->fps_stamp;
#if CONFIG_ESP_EFC_PERF_LOG
    reset_perf(efc);
#endif
    lv_timer_resume(efc->timer);
}

bool lv_efc_is_loaded(lv_obj_t *obj)
{
    return ((lv_efc_t *)obj)->dec != NULL;
}

int32_t lv_efc_get_total_frames(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    return efc->dec ? efc->total_frames : -1;
}

int32_t lv_efc_get_current_frame(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    return efc->dec ? efc->current_frame : -1;
}

uint32_t lv_efc_get_fps(lv_obj_t *obj)
{
    return ((lv_efc_t *)obj)->fps;
}

void lv_efc_set_frame_delay(lv_obj_t *obj, uint32_t delay_ms)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    efc->frame_delay = delay_ms;
    lv_timer_set_period(efc->timer, delay_ms ? delay_ms : EFC_FAST_MS);
}

uint32_t lv_efc_get_frame_delay(lv_obj_t *obj)
{
    return ((lv_efc_t *)obj)->frame_delay;
}

void lv_efc_set_loop_enabled(lv_obj_t *obj, bool enable)
{
    ((lv_efc_t *)obj)->loop_enabled = enable;
}

bool lv_efc_get_loop_enabled(lv_obj_t *obj)
{
    return ((lv_efc_t *)obj)->loop_enabled;
}

#if CONFIG_SOC_PPA_SUPPORTED
static void release_ppa(lv_efc_t *efc)
{
    if (efc->ppa_srm) {
        ppa_unregister_client(efc->ppa_srm);
        efc->ppa_srm = NULL;
    }
}
#endif

static void lv_efc_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_efc_t *efc = (lv_efc_t *)obj;
    efc->dec = NULL;
    efc->blob = NULL;
    efc->frame = NULL;
    efc->decode_pixels = NULL;
    efc->decode_stride = 0;
    efc->view = NULL;
    efc->x_idx = NULL;
#if CONFIG_SOC_PPA_SUPPORTED
    efc->ppa_srm = NULL;
#endif
    efc->loop_enabled = true;
    efc->frame_delay = EFC_FAST_MS;
    efc->timer = lv_timer_create(next_frame_cb, EFC_FAST_MS, obj);
    lv_timer_pause(efc->timer);
}

static void release_clip(lv_efc_t *efc)
{
    if (efc->imgdsc.data) {
        lv_image_cache_drop(&efc->imgdsc);
    }
#if CONFIG_SOC_PPA_SUPPORTED
    release_ppa(efc);
#endif
    esp_efc_dec_close(efc->dec);
    efc->dec = NULL;
    free(efc->frame);
    efc->frame = NULL;
    efc->decode_pixels = NULL;
    efc->decode_stride = 0;
    free(efc->view);
    efc->view = NULL;
    efc->view_cap = 0;
    efc->view_w = 0;
    efc->view_h = 0;
    free(efc->x_idx);
    efc->x_idx = NULL;
    efc->x_idx_dst_w = 0;
    free(efc->blob);
    efc->blob = NULL;
    efc->imgdsc.data = NULL;
    efc->total_frames = 0;
    efc->current_frame = 0;
    efc->use_alpha = false;
}

static void lv_efc_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_efc_t *efc = (lv_efc_t *)obj;
    release_clip(efc);
    lv_timer_delete(efc->timer);
}

static void note_fps(lv_efc_t *efc)
{
    efc->fps_frames++;
    uint32_t elaps = lv_tick_elaps(efc->fps_stamp);
    if (elaps >= 500 && efc->fps_frames) {
        efc->fps = (uint32_t)((uint64_t)efc->fps_frames * 1000u / elaps);
        efc->fps_frames = 0;
        efc->fps_stamp = lv_tick_get();
    }
}

#if CONFIG_ESP_EFC_PERF_LOG
static void reset_perf(lv_efc_t *efc)
{
    efc->perf_stamp_us = esp_timer_get_time();
    efc->perf_frames = 0;
    efc->perf_decode_us = 0;
    efc->perf_present_us = 0;
    efc->perf_publish_us = 0;
    efc->perf_changed_cols = 0;
    efc->perf_changed_rows = 0;
}

static void note_perf(lv_efc_t *efc, int64_t decode_us, int64_t present_us, int64_t publish_us)
{
    efc->perf_frames++;
    efc->perf_decode_us += (uint32_t)decode_us;
    efc->perf_present_us += (uint32_t)present_us;
    efc->perf_publish_us += (uint32_t)publish_us;
    efc->perf_changed_cols += (uint32_t)(efc->changed_x_end - efc->changed_x_start);
    efc->perf_changed_rows += (uint32_t)(efc->changed_y_end - efc->changed_y_start);

    int64_t elapsed_us = esp_timer_get_time() - efc->perf_stamp_us;
    if (elapsed_us < 1000000 || !efc->perf_frames) {
        return;
    }
    uint32_t frames = efc->perf_frames;
    uint32_t callback_us = efc->perf_decode_us + efc->perf_present_us + efc->perf_publish_us;
    uint32_t outside_us = elapsed_us > callback_us ? (uint32_t)(elapsed_us - callback_us) : 0;
    ESP_LOGI(TAG, "perf alpha=%d view=%dx%d fps=%u changed=%lux%lu avg_us decode=%lu "
                  "prepare=%lu publish=%lu outside_cb=%lu",
             efc->use_alpha, efc->view_w, efc->view_h, efc->fps,
             (unsigned long)(efc->perf_changed_cols / frames),
             (unsigned long)(efc->perf_changed_rows / frames),
             (unsigned long)(efc->perf_decode_us / frames),
             (unsigned long)(efc->perf_present_us / frames),
             (unsigned long)(efc->perf_publish_us / frames),
             (unsigned long)(outside_us / frames));
    reset_perf(efc);
}
#endif

static void next_frame_cb(lv_timer_t *t)
{
    lv_obj_t *obj = lv_timer_get_user_data(t);
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (!efc->dec) {
        return;
    }
    if (efc->frame_delay > 1 && lv_tick_elaps(efc->last_call) < efc->frame_delay) {
        return;
    }
    efc->last_call = lv_tick_get();

    int next = efc->current_frame + 1;
    if (next >= efc->total_frames) {
        if (!efc->loop_enabled) {
            lv_obj_send_event(obj, LV_EVENT_READY, NULL);
            lv_timer_pause(t);
            return;
        }
        next = 0;
    }
    efc->current_frame = next;
#if CONFIG_ESP_EFC_PERF_LOG
    int64_t mark = esp_timer_get_time();
#endif
    if (!decode_frame(efc)) {
        lv_timer_pause(t);
        return;
    }
#if CONFIG_ESP_EFC_PERF_LOG
    int64_t decode_us = esp_timer_get_time() - mark;
    mark = esp_timer_get_time();
#endif
    bool changed = present(obj);
#if CONFIG_ESP_EFC_PERF_LOG
    int64_t present_us = esp_timer_get_time() - mark;
    mark = esp_timer_get_time();
#endif
    note_fps(efc);
    publish(obj, false, changed);
#if CONFIG_ESP_EFC_PERF_LOG
    note_perf(efc, decode_us, present_us, esp_timer_get_time() - mark);
#endif
}

static bool load_blob(lv_efc_t *efc, const uint8_t *data, size_t len)
{
    if (!data || len < 8 || memcmp(data, "EFC1", 4) != 0) {
        return false;
    }
    if (esp_efc_dec_open(data, len, &efc->dec) != ESP_EFC_OK) {
        return false;
    }
    esp_efc_dec_info_t info;
    if (esp_efc_dec_get_info(efc->dec, &info) != ESP_EFC_OK) {
        esp_efc_dec_close(efc->dec);
        efc->dec = NULL;
        return false;
    }
    efc->width = info.width;
    efc->height = info.height;
    efc->total_frames = info.frame_count;
    efc->current_frame = 0;
    if (efc->width <= 0 || efc->height <= 0 || efc->total_frames <= 0) {
        esp_efc_dec_close(efc->dec);
        efc->dec = NULL;
        return false;
    }

    size_t color_bytes = (size_t)efc->width * (size_t)efc->height * 2;
    size_t alloc_bytes = color_bytes + (size_t)efc->width * (size_t)efc->height;
    efc->frame = efc_malloc(alloc_bytes);
    if (!efc->frame) {
        esp_efc_dec_close(efc->dec);
        efc->dec = NULL;
        return false;
    }
    /* Frame zero discovers the optional color key. Alpha is generated only at
     * the final display resolution by present(), avoiding a redundant full-size
     * alpha pass when the image is downscaled. */
    if (esp_efc_dec_frame(efc->dec, 0, (uint16_t *)efc->frame) != ESP_EFC_OK) {
        release_clip(efc);
        return false;
    }
    esp_efc_dec_get_info(efc->dec, &info);
    efc->use_alpha = info.color_key_index >= 0;
    efc->color_key = info.color_key;
    efc->changed_x_start = info.changed_x_start;
    efc->changed_x_end = info.changed_x_end;
    efc->changed_y_start = info.changed_y_start;
    efc->changed_y_end = info.changed_y_end;
    efc->frame_bytes = efc->use_alpha ? alloc_bytes : color_bytes;
    efc->decode_pixels = (const uint16_t *)efc->frame;
    efc->decode_stride = efc->width;

    memset(&efc->imgdsc, 0, sizeof(efc->imgdsc));
    efc->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    efc->imgdsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE;
    efc->imgdsc.header.cf = efc->use_alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565;
    efc->imgdsc.header.w = efc->width;
    efc->imgdsc.header.h = efc->height;
    efc->imgdsc.header.stride = efc->width * 2;
    efc->imgdsc.data = efc->frame;
    efc->imgdsc.data_size = efc->frame_bytes;
    return true;
}

static bool decode_frame(lv_efc_t *efc)
{
    esp_efc_err_t rc;
    if (efc->view_w > 0 && efc_dec_is_fixed4(efc->dec)) {
        rc = efc_dec_frame_fixed4_direct(efc->dec, efc->current_frame,
                                         &efc->decode_pixels, &efc->decode_stride);
    } else {
        rc = esp_efc_dec_frame(efc->dec, efc->current_frame, (uint16_t *)efc->frame);
        efc->decode_pixels = (const uint16_t *)efc->frame;
        efc->decode_stride = efc->width;
    }
    if (rc != ESP_EFC_OK) {
        LV_LOG_ERROR("efc: frame %d failed (%d)", efc->current_frame, (int)rc);
        return false;
    }
    esp_efc_dec_info_t info = {0};
    if (esp_efc_dec_get_info(efc->dec, &info) == ESP_EFC_OK) {
        efc->changed_x_start = info.changed_x_start;
        efc->changed_x_end = info.changed_x_end;
        efc->changed_y_start = info.changed_y_start;
        efc->changed_y_end = info.changed_y_end;
    }
    return true;
}

static bool fit_downscale(int src_w, int src_h, int box_w, int box_h,
                          int *dst_w, int *dst_h, float *scale)
{
    if (box_w <= 1 || box_h <= 1 || src_w <= 0 || src_h <= 0) {
        return false;
    }
    if (src_w <= box_w && src_h <= box_h) {
        return false;
    }
    float s = (float)box_w / (float)src_w;
    float sy = (float)box_h / (float)src_h;
    if (sy < s) {
        s = sy;
    }
    if (s >= 1.f) {
        return false;
    }
    if (s < 1.f / (float)EFC_PPA_SCALE_FRAG) {
        s = 1.f / (float)EFC_PPA_SCALE_FRAG;
    }
    int s_int = (int)s;
    int s_frag = (int)(s * (float)EFC_PPA_SCALE_FRAG) & (EFC_PPA_SCALE_FRAG - 1);
    for (;;) {
        int w = s_int * src_w + (s_frag * src_w) / EFC_PPA_SCALE_FRAG;
        int h = s_int * src_h + (s_frag * src_h) / EFC_PPA_SCALE_FRAG;
        if (w >= 1 && h >= 1 && w <= box_w && h <= box_h) {
            *dst_w = w;
            *dst_h = h;
            *scale = (float)s_int + (float)s_frag / (float)EFC_PPA_SCALE_FRAG;
            return true;
        }
        if (s_frag > 0) {
            s_frag--;
        } else if (s_int > 0) {
            s_int--;
            s_frag = EFC_PPA_SCALE_FRAG - 1;
        } else {
            return false;
        }
    }
}

#if defined(ESP_EFC_USE_SIMD) && (defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S3))
void esp_efc_nn_row_rgb565(uint16_t *dst, const uint16_t *src, const uint16_t *x_idx, int dst_w);
#else
static void esp_efc_nn_row_rgb565(uint16_t *dst, const uint16_t *src, const uint16_t *x_idx, int dst_w)
{
    int x = 0;
    for (; x + 3 < dst_w; x += 4) {
        dst[x + 0] = src[x_idx[x + 0]];
        dst[x + 1] = src[x_idx[x + 1]];
        dst[x + 2] = src[x_idx[x + 2]];
        dst[x + 3] = src[x_idx[x + 3]];
    }
    for (; x < dst_w; x++) {
        dst[x] = src[x_idx[x]];
    }
}
#endif

static bool ensure_x_idx(lv_efc_t *efc, int dst_w)
{
    if (efc->x_idx && efc->x_idx_dst_w == dst_w) {
        return true;
    }
    int src_w = efc->width;
    uint16_t *idx = malloc((size_t)dst_w * sizeof(uint16_t));
    if (!idx) {
        return false;
    }
    uint32_t step = ((uint32_t)src_w << 16) / (uint32_t)dst_w;
    uint32_t sx = step >> 1;
    for (int x = 0; x < dst_w; x++) {
        uint32_t col = sx >> 16;
        if (col >= (uint32_t)src_w) {
            col = (uint32_t)src_w - 1;
        }
        idx[x] = (uint16_t)col;
        sx += step;
    }
    free(efc->x_idx);
    efc->x_idx = idx;
    efc->x_idx_dst_w = dst_w;
    return true;
}

static void nn_rgb565_rect(const uint16_t *src, int src_w, int src_h,
                           uint16_t *dst, int dst_w, int dst_h, const uint16_t *x_idx,
                           int dst_x_start, int dst_x_end, int dst_y_start, int dst_y_end)
{
    uint32_t y_step = ((uint32_t)src_h << 16) / (uint32_t)dst_h;
    uint32_t sy = (y_step >> 1) + (uint32_t)dst_y_start * y_step;
    for (int y = dst_y_start; y < dst_y_end; y++) {
        int row = (int)(sy >> 16);
        if (row >= src_h) {
            row = src_h - 1;
        }
        esp_efc_nn_row_rgb565(dst + (size_t)y * (size_t)dst_w + dst_x_start,
                              src + (size_t)row * (size_t)src_w, x_idx + dst_x_start,
                              dst_x_end - dst_x_start);
        sy += y_step;
    }
}

static void changed_dst_axis(int src_size, int dst_size, int changed_start, int changed_end,
                             int *dst_start, int *dst_end)
{
    if (changed_start >= changed_end) {
        *dst_start = 0;
        *dst_end = 0;
        return;
    }
    uint32_t step = ((uint32_t)src_size << 16) / (uint32_t)dst_size;
    uint32_t pos = step >> 1;
    int first = dst_size;
    int last = 0;
    for (int dst = 0; dst < dst_size; dst++, pos += step) {
        int src = (int)(pos >> 16);
        if (src >= changed_start && src < changed_end) {
            if (first == dst_size) {
                first = dst;
            }
            last = dst + 1;
        }
    }
    *dst_start = first == dst_size ? 0 : first;
    *dst_end = last;
}

static void fill_alpha_scalar(const uint16_t *color, uint8_t *alpha, size_t pixels, uint16_t color_key)
{
    while (pixels && ((uintptr_t)alpha & 3u)) {
        *alpha++ = *color++ == color_key ? 0 : 0xFF;
        pixels--;
    }
    while (pixels >= 4) {
        uint32_t a0 = (uint32_t) - (color[0] != color_key) & 0xFFu;
        uint32_t a1 = (uint32_t) - (color[1] != color_key) & 0xFFu;
        uint32_t a2 = (uint32_t) - (color[2] != color_key) & 0xFFu;
        uint32_t a3 = (uint32_t) - (color[3] != color_key) & 0xFFu;
        *(uint32_t *)alpha = a0 | (a1 << 8) | (a2 << 16) | (a3 << 24);
        color += 4;
        alpha += 4;
        pixels -= 4;
    }
    while (pixels--) {
        *alpha++ = *color++ == color_key ? 0 : 0xFF;
    }
}

#if defined(ESP_EFC_USE_SIMD) && \
    (defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S3))
void esp_efc_fill_alpha_simd(const uint16_t *color, uint8_t *alpha, int pixels, uint16_t color_key);
#endif

static void fill_alpha(const uint16_t *color, uint8_t *alpha, size_t pixels, uint16_t color_key)
{
#if defined(ESP_EFC_USE_SIMD) && \
    (defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S3))
    size_t head = (16u - ((uintptr_t)alpha & 15u)) & 15u;
    if (head > pixels) {
        head = pixels;
    }
    fill_alpha_scalar(color, alpha, head, color_key);
    color += head;
    alpha += head;
    pixels -= head;

    size_t bulk = pixels & ~(size_t)15u;
    if (bulk) {
        esp_efc_fill_alpha_simd(color, alpha, (int)bulk, color_key);
        color += bulk;
        alpha += bulk;
        pixels -= bulk;
    }
#endif
    fill_alpha_scalar(color, alpha, pixels, color_key);
}

static bool ensure_view(lv_efc_t *efc, size_t bytes)
{
    size_t cap = (bytes + (EFC_PPA_ALIGN - 1)) & ~(size_t)(EFC_PPA_ALIGN - 1);
    if (efc->view && efc->view_cap >= cap) {
        return true;
    }
    free(efc->view);
    efc->view = NULL;
    efc->view_cap = 0;
    void *p = NULL;
#if CONFIG_SPIRAM
    p = heap_caps_aligned_alloc(EFC_PPA_ALIGN, cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
    if (!p) {
        p = heap_caps_aligned_alloc(EFC_PPA_ALIGN, cap, MALLOC_CAP_8BIT);
    }
    if (!p) {
        return false;
    }
    efc->view = p;
    efc->view_cap = cap;
    return true;
}

#if CONFIG_SOC_PPA_SUPPORTED
static int ppa_window_alignment(int scale_num)
{
    int a = scale_num;
    int b = EFC_PPA_SCALE_FRAG;
    while (b) {
        int remainder = a % b;
        a = b;
        b = remainder;
    }
    return EFC_PPA_SCALE_FRAG / a;
}

static bool ppa_scale_rgb565(lv_efc_t *efc, int dst_w, int dst_h, float scale, bool partial)
{
    if (!efc->ppa_srm) {
        ppa_client_config_t cfg = {
            .oper_type = PPA_OPERATION_SRM,
        };
        if (ppa_register_client(&cfg, &efc->ppa_srm) != ESP_OK) {
            return false;
        }
    }
    int src_x_start = 0;
    int src_x_end = efc->width;
    int src_y_start = 0;
    int src_y_end = efc->height;
    int dst_x_start = 0;
    int dst_y_start = 0;
    if (partial) {
        int scale_num = (int)(scale * EFC_PPA_SCALE_FRAG);
        int alignment = ppa_window_alignment(scale_num);
        src_x_start = (efc->changed_x_start / alignment) * alignment;
        src_y_start = (efc->changed_y_start / alignment) * alignment;
        src_x_end = ((efc->changed_x_end + alignment - 1) / alignment) * alignment;
        src_y_end = ((efc->changed_y_end + alignment - 1) / alignment) * alignment;
        if (src_x_end > efc->width) {
            src_x_end = efc->width;
        }
        if (src_y_end > efc->height) {
            src_y_end = efc->height;
        }
        dst_x_start = src_x_start * scale_num / EFC_PPA_SCALE_FRAG;
        dst_y_start = src_y_start * scale_num / EFC_PPA_SCALE_FRAG;
    }
    ppa_srm_oper_config_t op = {
        .in = {
            .buffer = (void *)efc->decode_pixels,
            .pic_w = (uint32_t)efc->decode_stride,
            .pic_h = (uint32_t)efc->height,
            .block_w = (uint32_t)(src_x_end - src_x_start),
            .block_h = (uint32_t)(src_y_end - src_y_start),
            .block_offset_x = (uint32_t)src_x_start,
            .block_offset_y = (uint32_t)src_y_start,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer = efc->view,
            .buffer_size = (uint32_t)efc->view_cap,
            .pic_w = (uint32_t)dst_w,
            .pic_h = (uint32_t)dst_h,
            .block_offset_x = (uint32_t)dst_x_start,
            .block_offset_y = (uint32_t)dst_y_start,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x = scale,
        .scale_y = scale,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };
    if (ppa_do_scale_rotate_mirror(efc->ppa_srm, &op) != ESP_OK) {
        release_ppa(efc);
        return false;
    }
    return true;
}
#endif

static bool scale_into_view(lv_efc_t *efc, int dst_w, int dst_h, float scale)
{
    size_t color_bytes = (size_t)dst_w * (size_t)dst_h * 2;
    size_t bytes = color_bytes + (efc->use_alpha ? (size_t)dst_w * (size_t)dst_h : 0);
    if (!ensure_view(efc, bytes)) {
        return false;
    }
    int dst_x_start = 0;
    int dst_x_end = dst_w;
    int dst_y_start = 0;
    int dst_y_end = dst_h;
    bool can_update_rect = efc->view_w == dst_w && efc->view_h == dst_h;
    if (can_update_rect) {
        changed_dst_axis(efc->width, dst_w, efc->changed_x_start, efc->changed_x_end,
                         &dst_x_start, &dst_x_end);
        changed_dst_axis(efc->height, dst_h, efc->changed_y_start, efc->changed_y_end,
                         &dst_y_start, &dst_y_end);
        if (dst_x_start == dst_x_end || dst_y_start == dst_y_end) {
            return true;
        }
    }
#if CONFIG_SOC_PPA_SUPPORTED
    bool hw = ppa_scale_rgb565(efc, dst_w, dst_h, scale, can_update_rect);
#else
    bool hw = false;
    (void)scale;
#endif
    if (!hw && !ensure_x_idx(efc, dst_w)) {
        return false;
    }
    if (!hw) {
        nn_rgb565_rect(efc->decode_pixels, efc->decode_stride, efc->height,
                       (uint16_t *)efc->view, dst_w, dst_h, efc->x_idx,
                       dst_x_start, dst_x_end, dst_y_start, dst_y_end);
    }
    if (efc->use_alpha) {
        for (int y = dst_y_start; y < dst_y_end; y++) {
            size_t pixel_start = (size_t)y * (size_t)dst_w + dst_x_start;
            fill_alpha((const uint16_t *)efc->view + pixel_start,
                       efc->view + color_bytes + pixel_start,
                       (size_t)(dst_x_end - dst_x_start), efc->color_key);
        }
    }
    return true;
}

static void bind_dsc(lv_efc_t *efc, const uint8_t *data, int w, int h, size_t bytes)
{
    efc->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    efc->imgdsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE;
    efc->imgdsc.header.cf = efc->use_alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565;
    efc->imgdsc.header.w = w;
    efc->imgdsc.header.h = h;
    efc->imgdsc.header.stride = w * 2;
    efc->imgdsc.data = data;
    efc->imgdsc.data_size = bytes;
}

static bool present(lv_obj_t *obj)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (!efc->frame) {
        return false;
    }
    int dst_w = efc->width;
    int dst_h = efc->height;
    float scale = 1.f;
    const uint8_t *data = efc->frame;
    size_t bytes = efc->frame_bytes;
    bool fitted = fit_downscale(efc->width, efc->height,
                                lv_obj_get_width(obj), lv_obj_get_height(obj),
                                &dst_w, &dst_h, &scale);
#if CONFIG_SOC_PPA_SUPPORTED
    if (!fitted) {
        release_ppa(efc);
    }
#endif
    if (fitted && scale_into_view(efc, dst_w, dst_h, scale)) {
        data = efc->view;
        bytes = (size_t)dst_w * (size_t)dst_h * 2;
        if (efc->use_alpha) {
            bytes += (size_t)dst_w * (size_t)dst_h;
        }
        efc->view_w = dst_w;
        efc->view_h = dst_h;
    } else {
        dst_w = efc->width;
        dst_h = efc->height;
        efc->view_w = 0;
        efc->view_h = 0;
        if (efc->decode_pixels != (const uint16_t *)efc->frame) {
            size_t row_bytes = (size_t)(efc->changed_x_end - efc->changed_x_start) * 2;
            for (int y = efc->changed_y_start; y < efc->changed_y_end; y++) {
                memcpy(efc->frame + ((size_t)y * (size_t)efc->width + efc->changed_x_start) * 2,
                       efc->decode_pixels + (size_t)y * (size_t)efc->decode_stride + efc->changed_x_start,
                       row_bytes);
            }
            efc->decode_pixels = (const uint16_t *)efc->frame;
            efc->decode_stride = efc->width;
        }
        if (efc->use_alpha) {
            size_t pixels = (size_t)efc->width * (size_t)efc->height;
            for (int y = efc->changed_y_start; y < efc->changed_y_end; y++) {
                size_t pixel_start = (size_t)y * (size_t)efc->width + efc->changed_x_start;
                fill_alpha((const uint16_t *)efc->frame + pixel_start,
                           efc->frame + pixels * 2 + pixel_start,
                           (size_t)(efc->changed_x_end - efc->changed_x_start), efc->color_key);
            }
        }
    }
    bool changed = efc->imgdsc.data != data || efc->imgdsc.header.w != (uint32_t)dst_w || efc->imgdsc.header.h != (uint32_t)dst_h;
    bind_dsc(efc, data, dst_w, dst_h, bytes);
    return changed;
}

static void invalidate_shown(lv_obj_t *obj, const lv_efc_t *efc)
{
    int32_t iw = (int32_t)efc->imgdsc.header.w;
    int32_t ih = (int32_t)efc->imgdsc.header.h;
    int32_t ow = lv_obj_get_width(obj);
    int32_t oh = lv_obj_get_height(obj);
    if (iw <= 0 || ih <= 0 || iw > ow || ih > oh) {
        lv_obj_invalidate(obj);
        return;
    }
    int dirty_x_start, dirty_x_end;
    int dirty_y_start, dirty_y_end;
    changed_dst_axis(efc->width, iw, efc->changed_x_start, efc->changed_x_end,
                     &dirty_x_start, &dirty_x_end);
    changed_dst_axis(efc->height, ih, efc->changed_y_start, efc->changed_y_end,
                     &dirty_y_start, &dirty_y_end);
    if (dirty_x_start == dirty_x_end || dirty_y_start == dirty_y_end) {
        return;
    }
    lv_area_t area;
    int32_t x = obj->coords.x1 + (ow - iw) / 2;
    int32_t y = obj->coords.y1 + (oh - ih) / 2;
    area.x1 = x + dirty_x_start;
    area.y1 = y + dirty_y_start;
    area.x2 = x + dirty_x_end - 1;
    area.y2 = y + dirty_y_end - 1;
    lv_obj_invalidate_area(obj, &area);
}

static void publish(lv_obj_t *obj, bool force_src, bool changed)
{
    lv_efc_t *efc = (lv_efc_t *)obj;
    if (force_src || changed) {
        lv_image_cache_drop(&efc->imgdsc);
        lv_image_set_src(obj, &efc->imgdsc);
    } else {
        invalidate_shown(obj, efc);
    }
}
