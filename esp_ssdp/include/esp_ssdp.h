/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _SSDP_H_
#define _SSDP_H_

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SSDP Configuration struct
 */
typedef struct {
    char *udn;              /*!< Unique Device Name */
    char *location;         /*!< The ssdp location for this udn, format as example: http://192.168.8.196/path
                                 or http://${ip}/path which ${ip} will be replaced by device ip address */
    int port;               /*!< SSDP listening on this port, default port will be used if not set */
    int notify_interval_ms; /*!< SSDP notify interval */
} ssdp_config_t;

/**
 * @brief SSDP service description
 */
typedef struct {
    char *usn;       //Unique Service Name
    char *location;
} ssdp_service_t;

typedef struct {

} ssdp_query_result_t;

/**
 * @brief Start SSDP responder service
 *
 * @param config SSDP configuration
 * @param services SSDP Search Target that will be responding
 *
 * @return
 *  - ESP_OK
 *  - ESP_FAIL
 */
esp_err_t ssdp_start(ssdp_config_t *config, const ssdp_service_t services[]);

/**
 * @brief Stop and free all SSDP resources
 */
void ssdp_stop();

/**
 * @brief      SSDP Query msearch
 *
 * @param[in]  addr     The address
 * @param[in]  port     The port
 * @param      target   The target
 * @param[in]  timeout  The timeout
 *
 * @return     { description_of_the_return_value }
 */
ssdp_query_result_t *ssdp_query_msearch(const char addr, int port, const char *target[], TickType_t timeout);

/**
 * @brief      Free query result
 *
 * @param      result  The result
 */
void sspd_free_query_result(ssdp_query_result_t *result);

#ifdef __cplusplus
}
#endif

#endif
