/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_ADAPTER_H
#define MEDIA_LIB_ADAPTER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Add all external library that media used using default function wrapper
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  wrapper functions not OK
 */
esp_err_t media_lib_add_default_adapter(void);

/**
 * @brief  Add default OS wrapper fuctions
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  OS wrapper functions not OK
 */
esp_err_t media_lib_add_default_os_adapter(void);

/**
 * @brief  Add default crypt related wrapper functions
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Crypt wrapper functions not OK
 */
esp_err_t media_lib_add_default_crypt_adapter(void);

/**
 * @brief  Add default socket related wrapper functions
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Socket wrapper functions not OK
 */
esp_err_t media_lib_add_default_socket_adapter(void);

/**
 * @brief  Add default tls related wrapper functions
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Tls wrapper functions not OK
 */
esp_err_t media_lib_add_default_tls_adapter(void);

/**
 * @brief  Add default network interface related wrapper functions
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Network interface wrapper functions not OK
 */
esp_err_t media_lib_add_default_netif_adapter(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_ADAPTER_H */
