/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "esp_err.h"

#include "esp_rtc_version.h"
#include "esp_rtsp_version.h"
#include "esp_rtmp_version.h"
#include "esp_rtmp_types.h"
#include "esp_rtc.h"
#include "esp_rtsp.h"
#include "esp_rtmp_push.h"
#include "esp_rtmp_server.h"
#include "esp_rtmp_src.h"
#include "esp_mrm_client.h"
#include "esp_ssdp.h"
#include "esp_upnp.h"
#include "esp_upnp_notify.h"
#include "media_lib_err.h"

TEST_CASE("proto_versions_strings", "[esp_media_protocols]")
{
    TEST_ASSERT_GREATER_THAN(0, strlen(ESP_RTC_VERSION));
    TEST_ASSERT_GREATER_THAN(0, strlen(ESP_RTSP_VERSION));
    TEST_ASSERT_GREATER_THAN(0, strlen(ESP_RTMP_VERION));
    const char *rv = esp_rtmp_get_version();
    TEST_ASSERT_NOT_NULL(rv);
    TEST_ASSERT_GREATER_THAN(0, strlen(rv));
    TEST_ASSERT_EQUAL_STRING(ESP_RTMP_VERION, rv);
}

TEST_CASE("proto_rtmp_standby_toggle", "[esp_media_protocols]")
{
    esp_rtmp_set_standby(true);
    esp_rtmp_set_standby(false);
}

TEST_CASE("proto_rtc_null_and_invalid_config", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_rtc_service_init(NULL));

    TEST_ASSERT_EQUAL(ESP_OK, esp_rtc_call(NULL, "1000"));
    TEST_ASSERT_EQUAL(ESP_OK, esp_rtc_answer(NULL));
    TEST_ASSERT_EQUAL(ESP_OK, esp_rtc_bye(NULL));
    TEST_ASSERT_EQUAL(ESP_OK, esp_rtc_service_deinit(NULL));

    esp_rtc_sip_message_info_t sip = {0};
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtc_read_incoming_messages(NULL, &sip));
    esp_rtc_hangup_msg_t hang = {0};
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtc_get_hangup_msg(NULL, &hang));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtc_set_invite_info(NULL, &sip));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtc_send_dtmf(NULL, 0, 0, 0));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtc_set_private_header(NULL, "x"));
}

TEST_CASE("proto_rtsp_null_handles", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_rtsp_server_start(NULL));
    TEST_ASSERT_NULL(esp_rtsp_client_start(NULL));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_rtsp_server_stop(NULL));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_rtsp_client_stop(NULL));
}

TEST_CASE("proto_rtmp_push_invalid", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_rtmp_push_open(NULL));
    rtmp_push_cfg_t cfg = {0};
    TEST_ASSERT_NULL(esp_rtmp_push_open(&cfg));

    esp_rtmp_audio_info_t a = {0};
    esp_rtmp_video_info_t v = {0};
    esp_rtmp_audio_data_t ad = {0};
    esp_rtmp_video_data_t vd = {0};
    uint8_t cmd = 0;

    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_set_audio_info(NULL, &a));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_set_video_info(NULL, &v));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_connect(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_audio(NULL, &ad));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_video(NULL, &vd));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_send_command(NULL, &cmd, 1));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG,
                     esp_rtmp_push_set_command_cb(NULL, NULL, NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_push_close(NULL));
}

TEST_CASE("proto_rtmp_server_invalid", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_rtmp_server_open(NULL));
    rtmp_server_cfg_t cfg = {0};
    TEST_ASSERT_NULL(esp_rtmp_server_open(&cfg));

    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_server_setup(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_server_query(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_server_monitor_puller(NULL, NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_server_monitor_connect_in(NULL, NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_server_close(NULL));
}

TEST_CASE("proto_rtmp_src_invalid", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_rtmp_src_open(NULL));
    rtmp_src_cfg_t cfg = {0};
    TEST_ASSERT_NULL(esp_rtmp_src_open(&cfg));

    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_connect(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_receive_media(NULL, false));
    uint8_t buf[4];
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_read(NULL, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_send_command(NULL, buf, 1));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_set_command_cb(NULL, NULL, NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_rtmp_src_close(NULL));
}

TEST_CASE("proto_mrm_client_null", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_mrm_client_create(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_mrm_client_destroy(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_mrm_client_master_start(NULL, "http://noop"));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_mrm_client_master_stop(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_mrm_client_slave_start(NULL));
    TEST_ASSERT_EQUAL(ESP_MEDIA_ERR_INVALID_ARG, esp_mrm_client_slave_stop(NULL));
}

TEST_CASE("proto_ssdp_quiescent_api", "[esp_media_protocols]")
{
    esp_ssdp_stop();
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_ssdp_send_byebye());
}

TEST_CASE("proto_upnp_null_handles", "[esp_media_protocols]")
{
    esp_upnp_destroy(NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_send_notify(NULL, "svc"));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_send_avt_notify(NULL, "act"));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_send_custom_notify(NULL, "svc", "<e/>", 4));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_register_service(NULL, NULL, NULL, NULL));
}

TEST_CASE("proto_upnp_service_null_handles", "[esp_media_protocols]")
{
    TEST_ASSERT_NULL(esp_upnp_service_init(NULL));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_service_destroy(NULL));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_service_register_actions(NULL, NULL, NULL, NULL, NULL));
}

TEST_CASE("proto_upnp_notify_init_destroy", "[esp_media_protocols]")
{
    upnp_notify_handle_t n = esp_upnp_notify_init();
    if (n) {
        TEST_ASSERT_NULL(esp_upnp_notify_subscribe(NULL, "s", "u", 10));
        TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_notify_unsubscribe(NULL, NULL));
        TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_notify_unsubscribe_by_name(NULL, "s"));
        TEST_ASSERT_NOT_EQUAL(ESP_OK, esp_upnp_notify_send(NULL, (char *)"d", 1, "s"));
        TEST_ASSERT_EQUAL(ESP_OK, esp_upnp_notify_destroy(n));
    }
}
