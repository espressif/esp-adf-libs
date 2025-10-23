/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_MEM_HIS_H
#define MEDIA_LIB_MEM_HIS_H

#include "media_lib_mem_trace.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Start memory history trace
 * @param  cfg  Memory trace configuration
 * @return
 *       - ESP_MEDIA_ERR_FAIL  Not resource
 *       - ESP_MEDIA_ERR_OK    On success
 */
int media_lib_start_mem_his(media_lib_mem_trace_cfg_t *cfg);

/**
 * @brief  Add malloc action to history
 * @param  addr       Memory address
 * @param  size       Memory size
 * @param  stack_num  Call stack number to record
 * @param  stack      Stack frames to record
 * @param  flag       Flag to record
 */
void media_lib_add_mem_malloc_his(void *addr, int size, int stack_num, void *stack, uint8_t flag);

/**
 * @brief  Add free action to history
 * @param  addr  Memory address
 */
void media_lib_add_mem_free_his(void *addr);

/**
 * @brief  Stop memory history trace
 */
void media_lib_stop_mem_his(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_MEM_HIS_H */
