/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * @file  lv_efc.h
 *
 * @brief  LVGL widget that plays an EFC clip, same shape as lv_eaf
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  EFC player object
 */
typedef struct _lv_efc_t lv_efc_t;

/**
 * @brief  LVGL class of the EFC player widget
 */
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_efc_class;

/**
 * @brief  Create an EFC player object
 *
 * @param[in]  parent  Parent object
 *
 * @return
 *       - Pointer  to the new object
 */
lv_obj_t *lv_efc_create(lv_obj_t *parent);

/**
 * @brief  Load a clip
 *
 *         `src` is a POSIX path ("/sdcard/clip.efc"), an LVGL filesystem path
 *         ("S:clip.efc"), or an lv_image_dsc_t whose data is the raw EFC blob.
 *
 * @param[in]  obj  EFC player
 * @param[in]  src  Clip source
 */
void lv_efc_set_src(lv_obj_t *obj, const void *src);

/**
 * @brief  Load a clip from memory
 *
 *         The buffer is copied. The caller may free `data` after this returns.
 *
 * @param[in]  obj   EFC player
 * @param[in]  data  Raw EFC blob
 * @param[in]  len   Length of `data` in bytes
 */
void lv_efc_set_src_data(lv_obj_t *obj, const void *data, size_t len);

/**
 * @brief  Start again from frame 0 and run
 *
 * @param[in]  obj  EFC player
 */
void lv_efc_play(lv_obj_t *obj);

/**
 * @brief  Stop advancing frames
 *
 *         The current frame stays on screen.
 *
 * @param[in]  obj  EFC player
 */
void lv_efc_pause(lv_obj_t *obj);

/**
 * @brief  Continue from the current frame
 *
 * @param[in]  obj  EFC player
 */
void lv_efc_resume(lv_obj_t *obj);

/**
 * @brief  Report whether a clip is loaded
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - true   A clip is open
 *       - false  No clip is open
 */
bool lv_efc_is_loaded(lv_obj_t *obj);

/**
 * @brief  Get the number of frames in the loaded clip
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - Frame  count
 *       - -1     No clip is open
 */
int32_t lv_efc_get_total_frames(lv_obj_t *obj);

/**
 * @brief  Get the index of the frame on screen
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - Current  frame index, starting at 0
 *       - -1       No clip is open
 */
int32_t lv_efc_get_current_frame(lv_obj_t *obj);

/**
 * @brief  Frames decoded per second over the last measurement window
 *
 *         The player runs as fast as decode and the display allow. This is that
 *         achieved rate, not a target.
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - Achieved  frames per second
 */
uint32_t lv_efc_get_fps(lv_obj_t *obj);

/**
 * @brief  Set the delay between frames
 *
 *         A delay of 0 or 1 plays as fast as decode and the display allow.
 *
 * @param[in]  obj       EFC player
 * @param[in]  delay_ms  Delay in milliseconds
 */
void lv_efc_set_frame_delay(lv_obj_t *obj, uint32_t delay_ms);

/**
 * @brief  Get the delay between frames
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - Delay  in milliseconds
 */
uint32_t lv_efc_get_frame_delay(lv_obj_t *obj);

/**
 * @brief  Enable or disable looping
 *
 * @param[in]  obj     EFC player
 * @param[in]  enable  true to wrap to frame 0 after the last frame
 */
void lv_efc_set_loop_enabled(lv_obj_t *obj, bool enable);

/**
 * @brief  Report whether the clip loops
 *
 * @param[in]  obj  EFC player
 *
 * @return
 *       - true   Playback wraps to frame 0
 *       - false  Playback stops on the last frame
 */
bool lv_efc_get_loop_enabled(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
