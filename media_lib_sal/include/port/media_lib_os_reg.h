/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_OS_REG_H
#define MEDIA_LIB_OS_REG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void *(*__media_lib_os_malloc)(size_t size);
typedef void (*__media_lib_os_free)(void *buf);
typedef void *(*__media_lib_os_calloc)(size_t num, size_t size);
typedef void *(*__media_lib_os_realloc)(void *buf, size_t size);
typedef char *(*__media_lib_os_strdup)(const char *str);
typedef void *(*__media_lib_os_caps_malloc_align)(size_t align, size_t size, int caps);
typedef int (*__media_lib_os_stack_frame)(void **addr, int n);

typedef void *media_lib_thread_handle_t;
typedef int (*__media_lib_os_thread_create)(media_lib_thread_handle_t *handle, const char *name, void (*body)(void *arg), void *arg,
                                            uint32_t stack_size, int prio, int core);
typedef void (*__media_lib_os_thread_destroy)(media_lib_thread_handle_t handle);
typedef bool (*__media_lib_os_thread_set_priority)(media_lib_thread_handle_t handle, int prio);
typedef void (*__media_lib_os_thread_sleep)(uint32_t ms);

typedef void *media_lib_sema_handle_t;
typedef int (*__media_lib_os_sema_create)(media_lib_sema_handle_t *sema);
typedef int (*__media_lib_os_sema_lock)(media_lib_sema_handle_t sema, uint32_t timeout);
typedef int (*__media_lib_os_sema_unlock)(media_lib_sema_handle_t sema);
typedef int (*__media_lib_os_sema_destroy)(media_lib_sema_handle_t sema);

typedef void *media_lib_mutex_handle_t;
typedef int (*__media_lib_os_mutex_create)(media_lib_mutex_handle_t *mutex);
typedef int (*__media_lib_os_mutex_lock)(media_lib_mutex_handle_t mutex, uint32_t timeout);
typedef int (*__media_lib_os_mutex_unlock)(media_lib_mutex_handle_t mutex);
typedef int (*__media_lib_os_mutex_destroy)(media_lib_mutex_handle_t mutex);

typedef int (*__media_lib_os_enter_critical_section)();
typedef int (*__media_lib_os_leave_critical_section)();

typedef void *media_lib_event_grp_handle_t;
typedef int (*__media_lib_os_event_group_create)(media_lib_event_grp_handle_t *event_group);
typedef uint32_t (*__media_lib_os_event_group_set_bits)(media_lib_event_grp_handle_t event_group, uint32_t bits);
typedef uint32_t (*__media_lib_os_event_group_clr_bits)(media_lib_event_grp_handle_t event_group, uint32_t bits);
typedef uint32_t (*__media_lib_os_event_group_wait_bits)(media_lib_event_grp_handle_t event_group, uint32_t bits, uint32_t timeout);
typedef int (*__media_lib_os_event_group_destroy)(media_lib_event_grp_handle_t event_group);

/**
 * @brief  Struct for OS related wrapper functions
 */
typedef struct {
    __media_lib_os_malloc             malloc;             /*!< Malloc wrapper */
    __media_lib_os_free               free;               /*!< Free wrapper */
    __media_lib_os_calloc             calloc;             /*!< Calloc wrapper */
    __media_lib_os_realloc            realloc;            /*!< Realloc wrapper */
    __media_lib_os_strdup             strdup;             /*!< Strdup wrapper */
    __media_lib_os_caps_malloc_align  caps_malloc_align;  /*!< Malloc align by capabilities */
    __media_lib_os_stack_frame        get_stack_frame;    /*!< Stack frame */

    __media_lib_os_thread_create        thread_create;    /*!< Thread create wrapper */
    __media_lib_os_thread_destroy       thread_destroy;   /*!< Thread destroy wrapper */
    __media_lib_os_thread_set_priority  thread_set_prio;  /*!< Set thread priority wrapper */
    __media_lib_os_thread_sleep         thread_sleep;     /*!< Thread sleep wrapper */

    __media_lib_os_sema_create   sema_create;   /*!< Sema create wrapper */
    __media_lib_os_sema_lock     sema_lock;     /*!< Sema lock wrapper */
    __media_lib_os_sema_unlock   sema_unlock;   /*!< Sema unlock wrapper */
    __media_lib_os_sema_destroy  sema_destroy;  /*!< Sema destroy wrapper */

    __media_lib_os_mutex_create   mutex_create;   /*!< Mutex create wrapper */
    __media_lib_os_mutex_lock     mutex_lock;     /*!< Mutex lock wrapper */
    __media_lib_os_mutex_unlock   mutex_unlock;   /*!< Mutex unlock wrapper */
    __media_lib_os_mutex_destroy  mutex_destroy;  /*!< Mutex destroy wrapper */

    __media_lib_os_enter_critical_section  enter_critical;  /*!< Enter critical wrapper */
    __media_lib_os_leave_critical_section  leave_critical;  /*!< Leave critical wrapper */

    __media_lib_os_event_group_create     group_create;     /*!< Event group create wrapper */
    __media_lib_os_event_group_set_bits   group_set_bits;   /*!< Event group set bits wrapper */
    __media_lib_os_event_group_clr_bits   group_clr_bits;   /*!< Event group clear bits  wrapper */
    __media_lib_os_event_group_wait_bits  group_wait_bits;  /*!< Event group wait for bits wrapper */
    __media_lib_os_event_group_destroy    group_destroy;    /*!< Event group destroy wrapper */
} media_lib_os_t;

/**
 * @brief  Register OS related wrapper functions for media library
 *
 * @param  os_lib  OS wrapper function lists
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Some members of OS lib not set
 */
esp_err_t media_lib_os_register(media_lib_os_t *os_lib);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_OS_REG_H */
