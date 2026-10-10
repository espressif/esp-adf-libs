/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * @file  lv_efc_private.h
 *
 * @brief  Private instance data for the LVGL EFC player widget
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sdkconfig.h"
#if CONFIG_SOC_PPA_SUPPORTED
struct ppa_client_t;
#endif
#include "lvgl.h"
#include "src/widgets/image/lv_image_private.h"

#include "esp_efc_codec.h"
#include "lv_efc.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  LVGL EFC player instance
 *
 *         The first member is an lv_image_t so the widget subclasses lv_image.
 *         The frame buffer persists across frames, so rows a frame does not
 *         touch stay as they were.
 */
struct _lv_efc_t {
    lv_image_t            img;            /*!< Base image widget */
    esp_efc_dec_handle_t  dec;            /*!< Open decoder, or NULL when no clip is loaded */
    uint8_t              *blob;           /*!< Owned copy of the EFC file */
    size_t                blob_len;       /*!< Length of `blob` in bytes */
    uint8_t              *frame;          /*!< Color plane, then the optional alpha plane */
    const uint16_t       *decode_pixels;  /*!< Current decoded color plane */
    int                   decode_stride;  /*!< Current decoded color stride in pixels */
    size_t                frame_bytes;    /*!< Bytes LVGL should read from `frame` */
    uint8_t              *view;           /*!< Fitted RGB565 / RGB565A8, or NULL when the frame is shown 1:1 */
    size_t                view_cap;       /*!< Allocated bytes of `view`, cache-line aligned */
    int                   view_w;         /*!< Fitted width; 0 when `view` is not on screen */
    int                   view_h;         /*!< Fitted height; 0 when `view` is not on screen */
    uint16_t             *x_idx;          /*!< Source x for each fitted column, fixed for this scale */
    int                   x_idx_dst_w;    /*!< Width used to build `x_idx`; may differ from `view_w` */
#if CONFIG_SOC_PPA_SUPPORTED
    struct ppa_client_t *ppa_srm;  /*!< Per-widget PPA scale client, or NULL */
#endif
    bool            use_alpha;        /*!< True when the clip has a color key */
    uint16_t        color_key;        /*!< RGB565 value mapped to transparent */
    int             current_frame;    /*!< Index of the frame on screen */
    int             total_frames;     /*!< Number of frames in the clip */
    int             width;            /*!< Frame width in pixels */
    int             height;           /*!< Frame height in pixels */
    int             changed_x_start;  /*!< First source column changed by the last decode */
    int             changed_x_end;    /*!< Exclusive last changed source column */
    int             changed_y_start;  /*!< First source row changed by the last decode */
    int             changed_y_end;    /*!< Exclusive last changed source row */
    uint32_t        frame_delay;      /*!< Delay in ms; 0 or 1 plays as fast as decode allows */
    bool            loop_enabled;     /*!< True to wrap to frame 0 after the last frame */
    lv_timer_t     *timer;            /*!< Advances frames */
    lv_image_dsc_t  imgdsc;           /*!< LVGL image descriptor over `frame` */
    uint32_t        last_call;        /*!< Tick of the last decoded frame */
    uint32_t        fps_stamp;        /*!< Tick when the current FPS window opened */
    uint32_t        fps_frames;       /*!< Frames decoded in the current FPS window */
    uint32_t        fps;              /*!< Achieved frames per second */
#if CONFIG_ESP_EFC_PERF_LOG
    int64_t   perf_stamp_us;      /*!< Start of the current profiling window */
    uint32_t  perf_frames;        /*!< Frames measured in the profiling window */
    uint32_t  perf_decode_us;     /*!< Accumulated codec time */
    uint32_t  perf_present_us;    /*!< Accumulated scale and alpha preparation time */
    uint32_t  perf_publish_us;    /*!< Accumulated image publication time */
    uint32_t  perf_changed_cols;  /*!< Accumulated changed source columns */
    uint32_t  perf_changed_rows;  /*!< Accumulated changed source rows */
#endif
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */
