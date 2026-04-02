/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_console.h"
#ifndef CONFIG_HEAP_TRACING_OFF
#include "esp_heap_trace.h"
#endif  /* CONFIG_HEAP_TRACING_OFF */
#include "esp_heap_caps.h"
#include "esp_log.h"

#define NUM_RECORDS  500
#define TAG          "HLS_DEBUG"

static int assert_cli(int argc, char **argv)
{
    *(int *)0 = 0;
    return 0;
}

int init_console(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "esp>";
    repl_config.task_stack_size = 10 * 1024;
    repl_config.task_priority = 22;
    repl_config.max_cmdline_length = 1024;
    // install console REPL environment
#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usbjtag_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config, &repl_config, &repl));
#endif  /* CONFIG_ESP_CONSOLE_UART */
    esp_console_cmd_t cmds[] = {
        {
            .command = "assert",
            .help = "Assert system\r\n",
            .func = assert_cli,
        },

    };
    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    return 0;
}

void heap_leak_trace(bool start)
{
#if (defined CONFIG_IDF_TARGET_ESP32S3) && (!defined CONFIG_HEAP_TRACING_OFF)
    static heap_trace_record_t *trace_record;
    if (trace_record == NULL) {
        trace_record = heap_caps_malloc(NUM_RECORDS * sizeof(heap_trace_record_t), MALLOC_CAP_SPIRAM);
        heap_trace_init_standalone(trace_record, NUM_RECORDS);
    }
    if (trace_record == NULL) {
        ESP_LOGE(TAG, "No memory to start trace");
        return;
    }
    static bool started = false;
    if (start) {
        ESP_LOGI(TAG, "Start to trace");
        if (started == false) {
            heap_trace_start(HEAP_TRACE_LEAKS);
            started = true;
        } else {
            heap_trace_resume();
        }
    } else {
        heap_trace_alloc_pause();
        heap_trace_dump();
    }
#endif  /* (defined CONFIG_IDF_TARGET_ESP32S3) && (!defined CONFIG_HEAP_TRACING_OFF) */
}
