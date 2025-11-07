/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_log.h"
#include "media_lib_adapter.h"
#include "media_lib_mem_trace.h"

#define TAG "MEDIA_ADAPTER"

#ifdef CONFIG_MEDIA_LIB_MEM_AUTO_TRACE
static void add_memory_trace(void)
{
    media_lib_mem_trace_cfg_t trace_cfg = {0};
#ifdef CONFIG_MEDIA_LIB_MEM_TRACE_MODULE
    trace_cfg.trace_type |= MEDIA_LIB_MEM_TRACE_MODULE_USAGE;
#endif
#ifdef CONFIG_MEDIA_LIB_MEM_TRACE_LEAKAGE
    trace_cfg.trace_type |= MEDIA_LIB_MEM_TRACE_LEAK;
#endif
#ifdef CONFIG_MEDIA_LIB_MEM_TRACE_SAVE_HISTORY
    trace_cfg.trace_type |= MEDIA_LIB_MEM_TRACE_SAVE_HISTORY;
    trace_cfg.save_cache_size = CONFIG_MEDIA_LIB_MEM_SAVE_CACHE_SIZE;
    trace_cfg.save_path = CONFIG_MEDIA_LIB_MEM_TRACE_SAVE_PATH;
#endif
    trace_cfg.stack_depth = CONFIG_MEDIA_LIB_MEM_TRACE_DEPTH;
    trace_cfg.record_num = CONFIG_MEDIA_LIB_MEM_TRACE_NUM;
    media_lib_start_mem_trace(&trace_cfg);
}
#endif

esp_err_t media_lib_add_default_adapter(void)
{
    esp_err_t ret;
    ret = media_lib_add_default_os_adapter();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to add os lib");
    }
#ifdef CONFIG_MEDIA_LIB_CRYPT_ENABLE
    ret = media_lib_add_default_crypt_adapter();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to add crypt lib");
    }
#endif
#ifdef CONFIG_MEDIA_LIB_SOCKET_ENABLE
    ret = media_lib_add_default_socket_adapter();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to add socket lib");
    }
#endif
#ifdef CONFIG_MEDIA_LIB_TLS_ENABLE
    ret = media_lib_add_default_tls_adapter();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to add tls lib");
    }
#endif
#ifdef CONFIG_MEDIA_LIB_NETIF_ENABLE
    ret = media_lib_add_default_netif_adapter();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to add netif lib");
    }
#endif
#ifdef CONFIG_MEDIA_LIB_MEM_AUTO_TRACE
    add_memory_trace();
#endif
    return ret;
}
