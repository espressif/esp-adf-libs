/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2021 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in
 * which case, it is free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdarg.h>
#include "media_lib_os_reg.h"
#include "media_lib_common.h"
#include "media_lib_os.h"

static media_lib_os_t media_os_lib;

esp_err_t media_lib_os_register(media_lib_os_t *os_lib)
{
    MEDIA_LIB_DEFAULT_INSTALLER(os_lib, &media_os_lib, media_lib_os_t);
}

void *media_lib_malloc(size_t size)
{
    if (media_os_lib.malloc) {
        return media_os_lib.malloc(size);
    }
    return NULL;
}

void media_lib_free(void *buf)
{
    if (media_os_lib.free) {
        media_os_lib.free(buf);
    }
}

void *media_lib_calloc(size_t num, size_t size)
{
    if (media_os_lib.calloc) {
        return media_os_lib.calloc(num, size);
    }
    return NULL;
}

void *media_lib_realloc(void *buf, size_t size)
{
    if (media_os_lib.realloc) {
        return media_os_lib.realloc(buf, size);
    }
    return NULL;
}

char *media_lib_strdup(const char *str)
{
    if (media_os_lib.strdup) {
        return media_os_lib.strdup(str);
    }
    return NULL;
}

int media_lib_asprintf(char **str, const char *fmt, ...)
{
    int size;
    va_list args, back_args;
    va_start(args, fmt);
    va_copy(back_args, args);
    size = vsnprintf(NULL, 0, fmt, back_args);
    va_end(back_args);
    if (size <= 0) {
        *str = NULL;
        return -1;
    }
    *str = (char *)media_lib_malloc(size + 1);
    size = vsprintf(*str, fmt, args);
    va_end(args);
    return size;
}

int media_lib_thread_create(media_lib_thread_handle_t *handle, const char *name,
                            void(*body)(void *arg), void *arg,
                            uint32_t stack_size, int prio, int core)
{
    if (media_os_lib.thread_create) {
        return media_os_lib.thread_create(handle, name, body, arg, stack_size,
                                          prio, core);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

void media_lib_thread_destroy(media_lib_thread_handle_t handle)
{
    if (media_os_lib.thread_destroy) {
        media_os_lib.thread_destroy(handle);
    }
}

bool media_lib_thread_set_priority(media_lib_thread_handle_t handle, int prio)
{
    if (media_os_lib.thread_set_prio) {
        return media_os_lib.thread_set_prio(handle, prio);
    }
    return false;
}

void media_lib_thread_sleep(uint32_t ms)
{
    if (media_os_lib.thread_sleep) {
        media_os_lib.thread_sleep(ms);
    }
}

int media_lib_sema_create(media_lib_sema_handle_t *sema)
{
    if (media_os_lib.sema_create) {
        return media_os_lib.sema_create(sema);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_sema_lock(media_lib_sema_handle_t sema, uint32_t timeout)
{
    if (media_os_lib.sema_lock) {
        return media_os_lib.sema_lock(sema, timeout);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_sema_unlock(media_lib_sema_handle_t sema)
{
    if (media_os_lib.sema_unlock) {
        return media_os_lib.sema_unlock(sema);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_sema_destroy(media_lib_sema_handle_t sema)
{
    if (media_os_lib.sema_destroy) {
        return media_os_lib.sema_destroy(sema);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_mutex_create(media_lib_mutex_handle_t *mutex)
{
    if (media_os_lib.mutex_create) {
        return media_os_lib.mutex_create(mutex);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_mutex_lock(media_lib_mutex_handle_t mutex, uint32_t timeout)
{
    if (media_os_lib.mutex_lock) {
        return media_os_lib.mutex_lock(mutex, timeout);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_mutex_unlock(media_lib_mutex_handle_t mutex) 
{
    if (media_os_lib.mutex_unlock) {
        return media_os_lib.mutex_unlock(mutex);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_mutex_destroy(media_lib_mutex_handle_t mutex)
{
    if (media_os_lib.mutex_destroy) {
        return media_os_lib.mutex_destroy(mutex);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_enter_critical_section()
{
    if (media_os_lib.leave_critical) {
        return media_os_lib.leave_critical();
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_leave_critical_section()
{
    if (media_os_lib.leave_critical) {
        return media_os_lib.leave_critical();
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_event_group_create(media_lib_event_grp_handle_t *event_group)
{
    if (media_os_lib.group_create) {
        return media_os_lib.group_create(event_group);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

uint32_t media_lib_event_group_set_bits(media_lib_event_grp_handle_t event_group, uint32_t bits)
{
    if (media_os_lib.group_set_bits) {
        return media_os_lib.group_set_bits(event_group, bits);
    }
    return 0;
}

uint32_t media_lib_event_group_clr_bits(media_lib_event_grp_handle_t event_group, uint32_t bits) {
    if (media_os_lib.group_clr_bits) {
        return media_os_lib.group_clr_bits(event_group, bits);
    }
    return 0;
}

uint32_t media_lib_event_group_wait_bits(media_lib_event_grp_handle_t event_group,
                                uint32_t bits, uint32_t timeout)
{
    if (media_os_lib.group_wait_bits) {
        return media_os_lib.group_wait_bits(event_group, bits, timeout);
    }
    return 0;
}

int media_lib_event_group_destroy(media_lib_event_grp_handle_t event_group)
{
    if (media_os_lib.group_destroy) {
        return media_os_lib.group_destroy(event_group);
    }
    return ESP_ERR_NOT_SUPPORTED;
}
