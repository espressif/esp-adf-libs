/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "esp_log.h"
#include "app_board.h"
#include "app_midi_keyboard.h"
#include "app_usb_hid.h"

static const char *TAG = "USB_HID";

QueueHandle_t app_event_queue = NULL;

typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

typedef struct {
    enum key_state {
        KEY_STATE_PRESSED = 0x00,
        KEY_STATE_RELEASED = 0x01
    } state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;

static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

/**
 * @brief Midi note index mapping
 *
 *    ┌───┬───┐   ┌───┬───┬───┐                               
 *    │ 2 │ 3 │   │ 5 │ 6 │ 7 │                               
 *  ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐                             
 *  │ Q │ W │ E │ R │ T │ Y │ U │                             
 *  └───┴───┴───┴───┴───┴───┴───┘ ┌───┬───┐   ┌───┬───┬───┐   
 *    0 1 2 3 4 ...           11  │ F │ G │   │ J │ K │ L │   
 *                              ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐ 
 *                              │ C │ V │ B │ N │ M │ < │ > │ 
 *                              └───┴───┴───┴───┴───┴───┴───┘ 
 *                               12 13 ...                23  
 */
static const uint8_t keycode2midi_index[57][2] = {
    {0xFF, 0xFF}, /* HID_KEY_NO_PRESS        */
    {0xFF, 0xFF}, /* HID_KEY_ROLLOVER        */
    {0xFF, 0xFF}, /* HID_KEY_POST_FAIL       */
    {0xFF, 0xFF}, /* HID_KEY_ERROR_UNDEFINED */
    {0xFF, 0xFF}, /* HID_KEY_A               */
    {16, 0xFF}, /* HID_KEY_B               */
    {12, 0xFF}, /* HID_KEY_C               */
    {0xFF, 0xFF}, /* HID_KEY_D               */
    {4, 0xFF}, /* HID_KEY_E               */
    {13, 0xFF}, /* HID_KEY_F               */
    {15, 0xFF}, /* HID_KEY_G               */
    {0xFF, 0xFF}, /* HID_KEY_H               */
    {0xFF, 0xFF}, /* HID_KEY_I               */
    {18, 0xFF}, /* HID_KEY_J               */
    {20, 0xFF}, /* HID_KEY_K               */
    {22, 0xFF}, /* HID_KEY_L               */
    {19, 0xFF}, /* HID_KEY_M               */
    {17, 0xFF}, /* HID_KEY_N               */
    {0xFF, 0xFF}, /* HID_KEY_O               */
    {0xFF, 0xFF}, /* HID_KEY_P               */
    {0, 0xFF}, /* HID_KEY_Q               */
    {5, 0xFF}, /* HID_KEY_R               */
    {0xFF, 0xFF}, /* HID_KEY_S               */
    {7, 0xFF}, /* HID_KEY_T               */
    {11, 0xFF}, /* HID_KEY_U               */
    {14, 0xFF}, /* HID_KEY_V               */
    {2, 0xFF}, /* HID_KEY_W               */
    {0xFF, 0xFF}, /* HID_KEY_X               */
    {9, 0xFF}, /* HID_KEY_Y               */
    {0xFF, 0xFF}, /* HID_KEY_Z               */
    {0xFF, 0xFF}, /* HID_KEY_1               */
    {1, 0xFF}, /* HID_KEY_2               */
    {3, 0xFF}, /* HID_KEY_3               */
    {0xFF, 0xFF}, /* HID_KEY_4               */
    {6, 0xFF}, /* HID_KEY_5               */
    {8, 0xFF}, /* HID_KEY_6               */
    {10, 0xFF}, /* HID_KEY_7               */
    {0xFF, 0xFF}, /* HID_KEY_8               */
    {0xFF, 0xFF}, /* HID_KEY_9               */
    {0xFF, 0xFF}, /* HID_KEY_0               */
    {0xFF, 0xFF}, /* HID_KEY_ENTER           */
    {0xFF, 0xFF}, /* HID_KEY_ESC             */
    {0xFF, 0xFF}, /* HID_KEY_DEL             */
    {0xFF, 0xFF}, /* HID_KEY_TAB             */
    {0xFF, 0xFF}, /* HID_KEY_SPACE           */
    {0xFF, 0xFF}, /* HID_KEY_MINUS           */
    {0xFF, 0xFF}, /* HID_KEY_EQUAL           */
    {0xFF, 0xFF}, /* HID_KEY_OPEN_BRACKET    */
    {0xFF, 0xFF}, /* HID_KEY_CLOSE_BRACKET   */
    {0xFF, 0xFF}, /* HID_KEY_BACK_SLASH      */
    {0xFF, 0xFF}, /* HID_KEY_SHARP           */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {0xFF, 0xFF}, /* HID_KEY_COLON           */
    {0xFF, 0xFF}, /* HID_KEY_QUOTE           */
    {0xFF, 0xFF}, /* HID_KEY_TILDE           */
    {21, 0xFF}, /* HID_KEY_LESS            */
    {23, 0xFF}, /* HID_KEY_GREATER         */
    {0xFF, 0xFF} /* HID_KEY_SLASH           */
};

static void hid_print_new_device_report_header(hid_protocol_t proto)
{
    static hid_protocol_t prev_proto_output = -1;

    if (prev_proto_output != proto) {
        prev_proto_output = proto;
        printf("\r\n");
        if (proto == HID_PROTOCOL_MOUSE) {
            printf("Mouse\r\n");
        } else if (proto == HID_PROTOCOL_KEYBOARD) {
            printf("Keyboard\r\n");
        } else {
            printf("Generic\r\n");
        }
        fflush(stdout);
    }
}

void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg
    };

    if (app_event_queue) {
        xQueueSend(app_event_queue, &evt_queue, 0);
    }
}

static inline int key_get_midi_note_index(uint8_t key_code, uint8_t modifier)
{
    if (key_code >= HID_KEY_A && key_code <= HID_KEY_SLASH) {
        return keycode2midi_index[key_code][0];
    }
    return -1;
}

static inline bool key_found(const uint8_t *const src,
                             uint8_t key,
                             unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        if (src[i] == key) {
            return true;
        }
    }
    return false;
}

static void key_event_callback(key_event_t *key_event, void *arg)
{
    unsigned char key_char;
    QueueHandle_t input_queue = (QueueHandle_t)arg;

    hid_print_new_device_report_header(HID_PROTOCOL_KEYBOARD);

    if (KEY_STATE_PRESSED == key_event->state) {
        int key_note = key_get_midi_note_index(key_event->key_code, key_event->modifier);
        if (key_note == 0xFF) {
            return;
        }

        keyboard_input_t input = {
            .type = KEYBOARD_INPUT_USB_HID,
            .value1 = key_note,
            .value2 = 0,
        };
        xQueueSend(input_queue, &input, KEYBOARD_INPUT_WAIT_TICK);    
    }
}

static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length, void *arg)
{
    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = { 0 };
    key_event_t key_event;

    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {

        // key has been released verification
        if (prev_keys[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(kb_report->key, prev_keys[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = prev_keys[i];
            key_event.modifier = 0;
            key_event.state = KEY_STATE_RELEASED;
            key_event_callback(&key_event, arg);
        }

        // key has been pressed verification
        if (kb_report->key[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(prev_keys, kb_report->key[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = kb_report->key[i];
            key_event.modifier = kb_report->modifier.val;
            key_event.state = KEY_STATE_PRESSED;
            key_event_callback(&key_event, arg);
        }
    }

    memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}

static void hid_host_generic_report_callback(const uint8_t *const data, const int length)
{
    hid_print_new_device_report_header(HID_PROTOCOL_NONE);
    for (int i = 0; i < length; i++) {
        printf("%02X", data[i]);
    }
    putchar('\r');
}

void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                hid_host_keyboard_report_callback(data, data_length, arg);
            }
        } else {
            hid_host_generic_report_callback(data, data_length);
        }

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
    default:
        ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
                 hid_proto_name_str[dev_params.proto]);
        break;
    }
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
                 hid_proto_name_str[dev_params.proto]);

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = arg
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

static void usb_host_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive(arg);

    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            printf("usb_host_device_free_all\n");
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

static void usb_host_hid_task(void *arg)
{
    app_event_queue_t evt_queue;

    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = arg
    };
    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));
    ESP_LOGI(TAG, "Waiting for HID Device to be connected");

    while (1) {
        // Wait queue
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    break;
                } else {
                    ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                }
            }

            if (APP_EVENT_HID_HOST == evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }

    ESP_LOGI(TAG, "HID Driver uninstall");
    ESP_ERROR_CHECK(hid_host_uninstall());
    xQueueReset(app_event_queue);
    vQueueDelete(app_event_queue);
}

esp_err_t usb_hid_init(void *input_queue)
{
    BaseType_t task_created;

    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));
    if (app_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_FAIL;
    }

    // Create usb_host_task to initialize USB Host library and handle usb host events
    task_created = xTaskCreatePinnedToCore(usb_host_task, "usb_task", 4096,
                                           xTaskGetCurrentTaskHandle(), 2, NULL, 0);
    if (task_created != pdPASS) {
        vQueueDelete(app_event_queue);
        ESP_LOGE(TAG, "Failed to create USB host task");
        return ESP_FAIL;
    }
    // Wait for notification from usb_host_task to proceed
    ulTaskNotifyTake(false, 1000);

    // Create usb_host_hid_task
    task_created = xTaskCreatePinnedToCore(usb_host_hid_task, "hid_task", 4096,
                                           input_queue, 2, NULL, 1);
    if (task_created != pdPASS) {
        vQueueDelete(app_event_queue);
        ESP_LOGE(TAG, "Failed to create USB hid task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
