/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "media_lib_err.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "unity.h"
#include "unity_test_runner.h"
#include "unity_test_utils_memory.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_heap_trace.h"

#define MAX_LEAK_TRACE_RECORDS     (512)
#define TEST_MEMORY_LEAK_THRESHOLD (500)
#define WIFI_AP_SSID               "media_lib_sal_test"
#define WIFI_AP_CHANNEL            1
#define WIFI_AP_MAX_CONN           4
#define TAG                        "SAL_TEST"

static void trace_for_leak(bool start)
{
#if defined(CONFIG_IDF_TARGET_ESP32S3) && !(defined(CONFIG_HEAP_TRACING_OFF))
    static heap_trace_record_t *trace_record;
    if (trace_record == NULL) {
        trace_record = heap_caps_malloc(MAX_LEAK_TRACE_RECORDS * sizeof(heap_trace_record_t), MALLOC_CAP_SPIRAM);
        if (trace_record == NULL) {
            return;
        }
        heap_trace_init_standalone(trace_record, MAX_LEAK_TRACE_RECORDS);
    }
    if (start) {
        heap_trace_start(HEAP_TRACE_LEAKS);
    } else {
        heap_trace_stop();
        heap_trace_dump();
        memset(trace_record, 0, MAX_LEAK_TRACE_RECORDS * sizeof(heap_trace_record_t));
    }
#endif  /* defined(CONFIG_IDF_TARGET_ESP32S3) && !(defined(CONFIG_HEAP_TRACING_OFF)) */
}

void setUp(void)
{
    unity_utils_record_free_mem();
    trace_for_leak(true);
}

void tearDown(void)
{
    trace_for_leak(false);
    unity_utils_evaluate_leaks_direct(TEST_MEMORY_LEAK_THRESHOLD);
}

esp_err_t start_wifi_ap(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "nvs init failed");
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init), TAG, "wifi init failed");

    wifi_config_t wifi = {0};
    strlcpy((char *)wifi.ap.ssid, WIFI_AP_SSID, sizeof(wifi.ap.ssid));
    wifi.ap.ssid_len = strlen(WIFI_AP_SSID);
    wifi.ap.channel = WIFI_AP_CHANNEL;
    wifi.ap.max_connection = WIFI_AP_MAX_CONN;
    wifi.ap.authmode = WIFI_AUTH_OPEN;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "set mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi), TAG, "set config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start failed");
    return ESP_OK;
}

void app_main()
{
    /***    _____ _____ ____ _____   __  __ _____ ____ ___    _      _     ___ ____    ____    _    _
     *     |_   _| ____/ ___|_   _| |  \/  | ____|  _ \_ _|  / \    | |   |_ _| __ )  / ___|  / \  | |
     *       | | |  _| \___ \ | |   | |\/| |  _| | | | | |  / _ \   | |    | ||  _ \  \___ \ / _ \ | |
     *       | | | |___ ___) || |   | |  | | |___| |_| | | / ___ \  | |___ | || |_) |  ___) / ___ \| |___
     *       |_| |_____|____/ |_|   |_|  |_|_____|____/___/_/   \_\ |_____|___|____/  |____/_/   \_\_____|
     *
     */
    printf(" _____ _____ ____ _____   __  __ _____ ____ ___    _      _     ___ ____    ____    _    _ \n");
    printf("|_   _| ____/ ___|_   _| |  \\/  | ____|  _ \\_ _|  / \\    | |   |_ _| __ )  / ___|  / \\  | | \n");
    printf("  | | |  _| \\___ \\ | |   | |\\/| |  _| | | | | |  / _ \\   | |    | ||  _ \\  \\___ \\ / _ \\ | | \n");
    printf("  | | | |___ ___) || |   | |  | | |___| |_| | | / ___ \\  | |___ | || |_) |  ___) / ___ \\| |___ \n");
    printf("  |_| |_____|____/ |_|   |_|  |_|_____|____/___/_/   \\_\\ |_____|___|____/  |____/_/   \\_\\_____| \n");

    esp_err_t ret = media_lib_add_default_adapter();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = start_wifi_ap();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    // Pre-trace to allocate memory
    trace_for_leak(true);

    unity_run_menu();
}
