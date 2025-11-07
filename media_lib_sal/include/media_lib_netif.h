/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_NETIF_H
#define MEDIA_LIB_NETIF_H

#include "media_lib_netif_reg.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Wrapper for get ipv4 information
 * @return
 *       - Others  Returned by wrapper function directly
 */
int media_lib_netif_get_ipv4_info(media_lib_net_type_t type, media_lib_ipv4_info_t *ip_info);

/**
 * @brief  Wrapper for convert ipv4 into string
 * @return
 *       - Others  Returned by wrapper function directly
 */
char *media_lib_ipv4_ntoa(const media_lib_ipv4_addr_t *addr);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_NETIF_H */
