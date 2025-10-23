/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_NETIF_REG_H
#define MEDIA_LIB_NETIF_REG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    uint32_t  addr;  /*!< IPv4 address */
} media_lib_ipv4_addr_t;

typedef struct {
    media_lib_ipv4_addr_t  ip;       /**< Interface IPV4 address */
    media_lib_ipv4_addr_t  netmask;  /**< Interface IPV4 netmask */
    media_lib_ipv4_addr_t  gw;       /**< Interface IPV4 gateway address */
} media_lib_ipv4_info_t;

typedef enum {
    MEDIA_LIB_NET_TYPE_STA = 0,  /**< Wi-Fi STA (station) interface */
    MEDIA_LIB_NET_TYPE_AP,       /**< Wi-Fi soft-AP interface */
    MEDIA_LIB_NET_TYPE_ETH,      /**< Ethernet interface */
} media_lib_net_type_t;

typedef int (*__media_lib_netif_get_ipv4_info)(media_lib_net_type_t type, media_lib_ipv4_info_t *ip_info);
typedef char *(*__media_lib_ipv4_ntoa)(const media_lib_ipv4_addr_t *addr);
typedef struct {
    __media_lib_netif_get_ipv4_info  get_ipv4_info;  /*!< Get ipv4 information */
    __media_lib_ipv4_ntoa            ipv4_ntoa;
} media_lib_netif_t;

/**
 * @brief  Register network interface related wrapper functions for media library
 *
 * @param  netif_lib  Network interface wrapper function lists
 *
 * @return
 *       - ESP_OK               On success
 *       - ESP_ERR_INVALID_ARG  Some members of network interface lib not set
 */
esp_err_t media_lib_netif_register(media_lib_netif_t *netif_lib);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_NETIF_REG_H */
