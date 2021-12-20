/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2021 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifndef MEDIA_LIB_OS_H
#define MEDIA_LIB_OS_H

#include "media_lib_os_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEDIA_LIB_MAX_LOCK_TIME (0xFFFFFFFF)

/**
 * @brief      Wrapper for malloc
 */
void *media_lib_malloc(size_t size);

/**
 * @brief      Wrapper for free
 */
void media_lib_free(void *buf);

/**
 * @brief      Wrapper for calloc
 */
void *media_lib_calloc(size_t num, size_t size);

/**
 * @brief      Wrapper for realloc
 */
void *media_lib_realloc(void *buf, size_t size);

/**
 * @brief      Wrapper for strdup
 */
char *media_lib_strdup(const char *str);

/**
 * @brief      asprintf wrapper
 */
int media_lib_asprintf(char **str, const char *fmt, ...);

/**
 * @brief      Wrapper for thread create
 * @param[out]    handle  thread handle
 * @param         name       thread name
 * @param         body       thread body
 * @param         arg        thread argument
 * @param         stack_size thread stacksize set
 * @param         prio       thread priority
 * @param         core       run in which cpu core
 * 
 * @return        - ESP_OK: on success
 *                - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *                - Others: thread create fail
 */
int media_lib_thread_create(media_lib_thread_handle_t *handle, const char *name, void(*body)(void *arg), void *arg,
                                   uint32_t stack_size, int prio, int core);

/**
 * @brief      Wrapper for thread destroy
 */
void media_lib_thread_destroy(media_lib_thread_handle_t handle);

/**
 * @brief      Wrapper for thread set priority
 * @return     - true:  on success
 *             - false: set priority fail
 */
bool media_lib_thread_set_priority(media_lib_thread_handle_t handle, int prio);

/**
 * @brief      Wrapper for thread sleep
 */
void media_lib_thread_sleep(uint32_t ms);

/**
 * @brief      Wrapper for sema create
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: sema create fail
 */
int media_lib_sema_create(media_lib_sema_handle_t *sema);

/**
 * @brief      Wrapper for sema lock with timeout
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: sema lock fail
 */
int media_lib_sema_lock(media_lib_sema_handle_t sema, uint32_t timeout);

/**
 * @brief      Wrapper for sema unlock
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: sema unlock fail
 */
int media_lib_sema_unlock(media_lib_sema_handle_t sema);

/**
 * @brief      Wrapper for sema destroy
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: mutex destroy fail
 */
int media_lib_sema_destroy(media_lib_sema_handle_t sema);

/**
 * @brief      Wrapper for mutex create
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: mutex create fail
 */
int media_lib_mutex_create(media_lib_mutex_handle_t *mutex);

/**
 * @brief      Wrapper for mutex lock with timeout
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: mutex lock fail
 */
int media_lib_mutex_lock(media_lib_mutex_handle_t mutex, uint32_t timeout);

/**
 * @brief      Wrapper for mutex unlock
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: mutex unlock fail
 */
int media_lib_mutex_unlock(media_lib_mutex_handle_t mutex);

/**
 * @brief      Wrapper for mutex destroy
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: mutex destroy fail
 */
int media_lib_mutex_destroy(media_lib_mutex_handle_t mutex);

/**
 * @brief      Wrapper for enter critical section
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: enter critical section fail
 */
int media_lib_enter_critical_section();

/**
 * @brief      Wrapper for leave critical section
 * @return     - 0: on success
 *             - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *             - Others: leave critical section fail
 */
int media_lib_leave_critical_section();

/**
 * @brief      Wrapper for event group create
 * @param[out] event_group event group handle pointer
 * @return                 - 0:on success
 *                         - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *                         - Others: event group create fail
 */
int media_lib_event_group_create(media_lib_event_grp_handle_t *event_group);

/**
 * @brief     wrapper for event group set bits
 * @param     event_group event group handle
 * @param     bits        set bits mask
 * @return    uint32_t    bits being set currently
 */
uint32_t media_lib_event_group_set_bits(media_lib_event_grp_handle_t event_group, uint32_t bits);

/**
 * @brief      Wrapper for event group clear bits
 * @param     event_group event group handle
 * @param     bits        clear bits mask
 * @return    uint32_t    bits being set currently
 */
uint32_t media_lib_event_group_clr_bits(media_lib_event_grp_handle_t event_group, uint32_t bits);

/**
 * @brief      Wrapper for event group wait bits
 * @param     event_group event group handle
 * @param     bits        waiting for bits which will be set
 * @param     timeout     wait timeout set
 * @return    uint32_t    bits being set currently
 */
uint32_t media_lib_event_group_wait_bits(media_lib_event_grp_handle_t event_group, uint32_t bits, uint32_t timeout);

/**
 * @brief      Wrapper for event group destroy
 * @return                 - 0: on success
 *                         - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *                         - Others: event group destroy fail
 */
int media_lib_event_group_destroy(media_lib_event_grp_handle_t event_group);

#ifdef __cplusplus
}
#endif

#endif
