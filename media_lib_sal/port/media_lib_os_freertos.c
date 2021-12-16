
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

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "media_lib_adapter.h"
#include "media_lib_os_reg.h"

#define RETURN_ON_NULL_HANDLE(h)                                               \
    if (h == NULL) {                                                           \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define TAG "MEDIA_OS"

#if CONFIG_SPIRAM_BOOT_INIT
static void *_malloc_in_heap(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void _free_in_heap(void *buf)
{ 
    free(buf);
}

static void *_calloc_in_heap(size_t elm, size_t size)
{
    size_t total = elm * size;
    void *buf = _malloc_in_heap(total);
    if (buf) {
        memset(buf, 0, total);
    }
    return buf;
}

static void *_relloc_in_heap(void *buf, size_t size)
{
    return heap_caps_realloc(buf, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static char *_strdup_in_heap(const char *str)
{
    if (str) {
        int len = strlen(str) + 1;
        void *buf = _malloc_in_heap(len);
        if (buf) {
            memcpy(buf, str, len);
            return buf;
        }
    }
    return NULL;
}
#endif

#if defined(CONFIG_SPIRAM_BOOT_INIT) &&                                        \
    (CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY)
BaseType_t __attribute__((weak)) xTaskCreateRestrictedPinnedToCore(
    const TaskParameters_t *const pxTaskDefinition, TaskHandle_t *pxCreatedTask,
    const BaseType_t xCoreID)
{
    ESP_LOGE(TAG,
             "Not found right %s.\r\nPlease enter IDF-PATH with \"cd "
             "$IDF_PATH\" and apply the IDF patch with \"git apply "
             "$ADF_PATH/idf_patches/idf_v3.3_freertos.patch\" first\r\n",
             __func__);
    return pdFALSE;
}
static int _thread_create(media_lib_thread_handle_t *handle, const char *name,
                          void(*body)(void *arg), void *arg, uint32_t stack_size,
                          int prio, int core)
{
    StackType_t *task_stack = (StackType_t *)_calloc_in_heap(1, stack_size);
    do {
        TaskParameters_t xRegParameters = {.pvTaskCode = body,
                                           .pcName = name,
                                           .usStackDepth = stack_size,
                                           .pvParameters = arg,
                                           .uxPriority =
                                               prio | portPRIVILEGE_BIT,
                                           .puxStackBuffer = task_stack,
                                           .xRegions = {{
                                               .pvBaseAddress = 0x00,
                                               .ulLengthInBytes = 0x00,
                                               .ulParameters = 0x00,
                                           }}};
        if (xTaskCreateRestrictedPinnedToCore(
                &xRegParameters, (xTaskHandle *)handle, core) != pdPASS) {
            ESP_LOGE(TAG, "Error creating RestrictedPinnedToCore %s", name);
            break;
        }
        return ESP_OK;
    } while (0);
    if (task_stack) {
        _free_in_heap(task_stack);
    }
    return ESP_FAIL;
}
#else
static int _thread_create(media_lib_thread_handle_t *handle, const char *name,
                          void(*body)(void *arg), void *arg, uint32_t stack_size,
                          int prio, int core)
{
    if (xTaskCreatePinnedToCore(body, name, stack_size, arg, prio,
                                (xTaskHandle *)handle, core) != pdPASS) {
        ESP_LOGE(TAG, "Fail to create thread %s\n", name);
        return ESP_FAIL;
    }
    return ESP_OK;
}
#endif

static void _thread_destroy(media_lib_thread_handle_t handle)
{
    // allow NULL to destroy self
    vTaskDelete((xTaskHandle)handle);
}

static bool _thread_set_priority(media_lib_thread_handle_t handle, int prio)
{
    vTaskPrioritySet((xTaskHandle)handle, prio);
    return true;
}

static void _thread_sleep(uint32_t ms)
{
    vTaskDelay(ms / portTICK_RATE_MS);
}

static int _sema_create(media_lib_sema_handle_t *sema)
{
    if (sema) {
        *sema = (media_lib_sema_handle_t)xSemaphoreCreateCounting(1, 0);
        if (*sema != NULL) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

static int _mutex_create(media_lib_mutex_handle_t *mutex)
{
    if (mutex) {
        *mutex = (media_lib_mutex_handle_t)xSemaphoreCreateMutex();
        if (*mutex != NULL) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}
static int _os_lock_timeout(void *lock, uint32_t timeout)
{
    RETURN_ON_NULL_HANDLE(lock);
    if (timeout != portMAX_DELAY) {
        timeout /= portTICK_PERIOD_MS;
    }
    return xSemaphoreTake((QueueHandle_t)lock, timeout) ? ESP_OK : ESP_FAIL;
}

static int _mutex_lock_timeout(media_lib_mutex_handle_t mutex, uint32_t timeout)
{
    return _os_lock_timeout(mutex, timeout);
}

static int _sema_lock_timeout(media_lib_sema_handle_t sema, uint32_t timeout)
{
    return _os_lock_timeout(sema, timeout);
}

static int _os_unlock(void *lock)
{
    RETURN_ON_NULL_HANDLE(lock);
    xSemaphoreGive((QueueHandle_t)lock);
    return ESP_OK;
}

static int _mutex_unlock(media_lib_mutex_handle_t mutex)
{
    return _os_unlock(mutex);
}

static int _sema_unlock(media_lib_sema_handle_t sema)
{
    return _os_unlock(sema);
}

static int _os_lock_destroy(void *lock)
{
    RETURN_ON_NULL_HANDLE(lock);
    vSemaphoreDelete((QueueHandle_t)lock);
    return ESP_OK;
}

static int _mutex_destroy(media_lib_mutex_handle_t mutex)
{
    return _os_lock_destroy(mutex);
}

static int _sema_destroy(media_lib_sema_handle_t sema)
{
    return _os_lock_destroy(sema);
}

static int _enter_critical(void)
{
    return ESP_OK;
}

static int _leave_critical(void)
{ 
    return ESP_OK;
}

static int _event_group_create(media_lib_event_grp_handle_t *group)
{
    RETURN_ON_NULL_HANDLE(group);
    *group = (media_lib_event_grp_handle_t)xEventGroupCreate();
    return ESP_OK;
}

static uint32_t _event_group_set_bits(media_lib_event_grp_handle_t group, uint32_t bits)
{
    RETURN_ON_NULL_HANDLE(group);
    return (uint32_t)xEventGroupSetBits((EventGroupHandle_t)group, bits);
}

static uint32_t _event_group_clr_bits(media_lib_event_grp_handle_t group, uint32_t bits)
{
    RETURN_ON_NULL_HANDLE(group);
    return (uint32_t)xEventGroupClearBits((EventGroupHandle_t)group, bits);
}

static uint32_t _event_group_wait_bits(media_lib_event_grp_handle_t group,
                                       uint32_t bits, uint32_t timeout)
{
    RETURN_ON_NULL_HANDLE(group);
    if (timeout != portMAX_DELAY) {
        timeout /= portTICK_PERIOD_MS;
    }
    return (uint32_t)xEventGroupWaitBits((EventGroupHandle_t)group, bits, false,
                                         true, timeout);
}

static int _event_group_destroy(media_lib_event_grp_handle_t group)
{
    RETURN_ON_NULL_HANDLE(group);
    vEventGroupDelete((EventGroupHandle_t)group);
    return ESP_OK;
}

esp_err_t media_lib_add_default_os_adapter(void)
{
    media_lib_os_t os_lib = {
#if CONFIG_SPIRAM_BOOT_INIT
        .malloc = _malloc_in_heap,
        .free = _free_in_heap,
        .calloc = _calloc_in_heap,
        .realloc = _relloc_in_heap,
        .strdup = _strdup_in_heap,
#else
        .malloc = malloc,
        .free = free,
        .calloc = calloc,
        .realloc = realloc,
        .strdup = strdup,
#endif

        .thread_create = _thread_create,
        .thread_destroy = _thread_destroy,
        .thread_set_prio = _thread_set_priority,
        .thread_sleep = _thread_sleep,

        .sema_create = _sema_create,
        .sema_lock   = _sema_lock_timeout,
        .sema_unlock = _sema_unlock,
        .sema_destroy = _sema_destroy,

        .mutex_create = _mutex_create,
        .mutex_lock =   _mutex_lock_timeout,
        .mutex_unlock = _mutex_unlock,
        .mutex_destroy = _mutex_destroy,

        .enter_critical = _enter_critical,
        .leave_critical = _leave_critical,

        .group_create = _event_group_create,
        .group_set_bits = _event_group_set_bits,
        .group_clr_bits = _event_group_clr_bits,
        .group_wait_bits = _event_group_wait_bits,
        .group_destroy = _event_group_destroy,
    };
    return media_lib_os_register(&os_lib);
}
