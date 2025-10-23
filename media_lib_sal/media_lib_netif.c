/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "media_lib_netif.h"
#include "media_lib_netif_reg.h"
#include "media_lib_common.h"

#ifdef CONFIG_MEDIA_LIB_NETIF_ENABLE

static media_lib_netif_t media_netif_lib;

esp_err_t media_lib_netif_register(media_lib_netif_t *netif_lib)
{
    MEDIA_LIB_DEFAULT_INSTALLER(netif_lib, &media_netif_lib, media_lib_netif_t);
}

int media_lib_netif_get_ipv4_info(media_lib_net_type_t type, media_lib_ipv4_info_t *ip_info)
{
    if (media_netif_lib.get_ipv4_info) {
        return media_netif_lib.get_ipv4_info(type, ip_info);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

char* media_lib_ipv4_ntoa(const media_lib_ipv4_addr_t *addr)
{
    if (media_netif_lib.ipv4_ntoa) {
        return media_netif_lib.ipv4_ntoa(addr);
    }
    return NULL;
}
#endif
