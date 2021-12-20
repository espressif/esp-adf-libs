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

#ifndef _UPNP_NOTIF_H_
#define _UPNP_NOTIF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upnp_notify_* upnp_notify_handle_t;

/**
 * @brief      Initialize UPnP notify, the value returned by this function can be used to
 *             subscribe/unsubscribe and to send an event to a client
 *
 * @return     The upnp notify handle
 */
upnp_notify_handle_t esp_upnp_notify_init();

/**
 * @brief      Subscribe/Re-new the subscriptions for UPnP events
 *
 * @param[in]  notify        The notify handle
 * @param[in]  service_name  The service name
 * @param[in]  uri_or_sid    The uri or sid
 * @param[in]  timeout_sec   The timeout in seconds (If after this time passes without any renew
 *                           this subscription will be unsubscribed automatically )
 *
 * @return     The sid
 */
char *esp_upnp_notify_subscribe(upnp_notify_handle_t notify,
                            const char           *service_name,
                            const char           *uri_or_sid,
                            int                  timeout_sec);

/**
 * @brief      Unsubscribe the notify by sid
 *
 * @param[in]  notify  The notify handle
 * @param[in]  sid     The sid
 *
 * @return
 *     -    ESP_OK
 *     -    ESP_xx if any errors
 */
esp_err_t esp_upnp_notify_unsubscribe(upnp_notify_handle_t notify, char *sid);

/**
 * @brief      Unsubscribe the notify by service name
 *
 * @param[in]  notify           The notify handle
 * @param[in]  service_name     The service name
 *
 * @return
 *     -    ESP_OK
 *     -    ESP_xx if any errors
 */
esp_err_t esp_upnp_notify_unsubscribe_by_name(upnp_notify_handle_t notify, const char *service_name);

/**
 * @brief      Send the notification to the client
 *
 * @param[in]  notify         The notify handle
 * @param      data           The data
 * @param[in]  data_len       The data length
 * @param[in]  service_name   The service name
 *
 * @return
 *     -    ESP_OK
 *     -    ESP_xx if any errors
 */
esp_err_t esp_upnp_notify_send(upnp_notify_handle_t notify,
                           char                 *data,
                           int                  data_len,
                           const char           *service_name);

/**
 * @brief      Clean up & destroy the notify
 *
 * @param[in]  notify  The notify
 *
 * @return
 *     -    ESP_OK
 *     -    ESP_xx if any errors
 */
esp_err_t esp_upnp_notify_destroy(upnp_notify_handle_t notify);

#ifdef __cplusplus
}
#endif

#endif
