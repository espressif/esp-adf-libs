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

#ifndef _UPNP_H
#define _UPNP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_http_server.h"
#include "esp_upnp_service.h"

typedef struct upnp_* upnp_handle_t;

/**
 * UPnP Device Description – general info
 * Official: http://www.upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
 * Ref: https://embeddedinn.wordpress.com/tutorials/upnp-device-architecture/
 */
typedef struct {
    const char              *friendly_name;     /*!< Short user-friendly title */
    const char              *serial;            /*!< Device manufacturer's serial number */
    const char              *uuid;              /*!< Device UUID */
    const char              *root_path;         /*!< The custom XML root path */
    const char              *device_type;       /*!< Will replace for deviceType in UPnP schema urn:schemas-upnp-org:device:deviceType:version */
    const char              *version;           /*!< Will replace for version in UPnP schema urn:schemas-upnp-org:device:deviceType:version */
    const char              *upc;               /*!< Universal Product Code */
    const char              *manufacturer;      /*!< Manufacturer name */
    const char              *manufacturer_url;  /*!< URL to manufacturer site */
    const char              *model_description; /*!< long user-friendly title */
    const char              *model_name;        /*!< model name */
    const char              *model_number;      /*!< model number */
    const char              *model_url;         /*!< URL to model site */
    const upnp_file_info_t  logo;               /*!< Logo for the device*/
    httpd_handle_t          httpd;              /*!< UPnP needs a HTTP server to work */
    int                     port;               /*!< Server port (for advertise)*/
    void                    *user_ctx;          /*!< User context, it be will passed to attribute/notify and action callback */
    bool                    device_list;        /*!< Support Device List */
} upnp_config_t;


/**
 * @brief      Intialize UPnP object
 *
 * @param[in]  config  The configuration
 *
 * @return     UPnP handle
 */
upnp_handle_t esp_upnp_init(const upnp_config_t *config);

/**
 * @brief      Send the notification to the client
 *
 * @param[in]  upnp           The upnp handle
 * @param[in]  service_name   The service name
 *
 * @return
 *     - ESP_OK
 *     - ESP_xx if any errors
 */
esp_err_t esp_upnp_send_notify(upnp_handle_t upnp, const char *service_name);

/**
 * @brief      Send the AVTransport notification to the client
 *
 * @param[in]  upnp           The upnp handle
 * @param[in]  action_name    The action name
 *
 * @return
 *     - ESP_OK
 *     - ESP_xx if any errors
 */
esp_err_t esp_upnp_send_avt_notify(upnp_handle_t upnp, const char *action_name);

/**
 * @brief      Send custom notification to the client
 *
 * @param[in]  upnp           The upnp handle
 * @param[in]  service_name   The service name
 * @param[in]  event_xml      custom event xml (ref: EventLastChange.xml)
 * @param[in]  xml_length     The xml length
 *
 * @return
 *     -    ESP_OK
 *     -    ESP_xx if any errors
 */
esp_err_t esp_upnp_send_custom_notify(upnp_handle_t upnp, const char *service_name, const char *event_xml, int32_t xml_length);

/**
 * @brief      Clean up and destroy the UPnP handle
 *
 * @param[in]  upnp  The UPnP handle
 *
 * @return
 *     - ESP_OK
 *     - ESP_xx if any errors
 */
esp_err_t esp_upnp_destroy(upnp_handle_t upnp);

/**
 * @brief      Serve http static files (using upnp_file_info_t format)
 *
 * @param      req   The request
 *
 * @return
 *     - ESP_OK
 *     - ESP_xx if any errors
 */
esp_err_t esp_upnp_serve_static_files(httpd_req_t *req);

/**
 * @brief      Register actions/notifies for the service
 *
 * @param[in]  upnp             The UPnP handle
 * @param[in]  service_item     The service item
 * @param[in]  service_actions  The service actions
 * @param[in]  service_notify   The service notify
 *
 * @return
 *     - ESP_OK
 *     - ESP_xx if any errors
 */
esp_err_t esp_upnp_register_service(upnp_handle_t               upnp,
                                const upnp_service_item_t   *service_item,
                                const service_action_t      service_actions[],
                                const service_notify_t      service_notify[]);
#ifdef __cplusplus
}
#endif

#endif
