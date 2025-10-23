/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "unity.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "media_lib_crypt.h"
#include "media_lib_socket.h"
#include "media_lib_tls.h"
#include "media_lib_netif.h"
#include "media_lib_mem_trace.h"
#include "media_lib_err.h"

/*******************************************************************************
 * media_lib_os.h test group
 ******************************************************************************/

static void test_media_lib_os_memory(void)
{
    void *ptr = media_lib_malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    media_lib_free(ptr);

    ptr = media_lib_calloc(10, 10);
    TEST_ASSERT_NOT_NULL(ptr);
    media_lib_free(ptr);
}

static void test_media_lib_os_mutex(void)
{
    media_lib_mutex_handle_t mutex;
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_create(&mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_destroy(mutex));
}

static void test_media_lib_os_semaphore(void)
{
    media_lib_sema_handle_t sema;
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&sema));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(sema));
}

TEST_CASE("test_os_lib", "[media_lib_sal]")
{
    test_media_lib_os_memory();
    test_media_lib_os_mutex();
    test_media_lib_os_semaphore();
}

/*******************************************************************************
 * media_lib_crypt.h test group
 ******************************************************************************/

static void test_media_lib_crypt_md5(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    media_lib_md5_handle_t md5_ctx;
    media_lib_md5_init(&md5_ctx);
    TEST_ASSERT_NOT_NULL(md5_ctx);
    media_lib_md5_free(md5_ctx);
#endif
}

static void test_media_lib_crypt_sha256(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    media_lib_sha256_handle_t sha256_ctx;
    media_lib_sha256_init(&sha256_ctx);
    TEST_ASSERT_NOT_NULL(sha256_ctx);
    media_lib_sha256_free(sha256_ctx);
#endif
}

static void test_media_lib_crypt_aes(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    media_lib_aes_handle_t aes_ctx;
    media_lib_aes_init(&aes_ctx);
    TEST_ASSERT_NOT_NULL(aes_ctx);
    media_lib_aes_free(aes_ctx);
#endif
}

#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
TEST_CASE("test_crypt_lib", "[media_lib_sal]")
{
    test_media_lib_crypt_md5();
    test_media_lib_crypt_sha256();
    test_media_lib_crypt_aes();
}
#endif

/*******************************************************************************
 * media_lib_socket.h test group
 ******************************************************************************/

static void test_media_lib_socket(void)
{
#if defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
    int sock = media_lib_socket_open(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        TEST_ASSERT_EQUAL(0, media_lib_socket_close(sock));
    }
#endif
}

#if defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
TEST_CASE("test_socket_lib", "[media_lib_sal]")
{
    test_media_lib_socket();
}
#endif

/*******************************************************************************
 * media_lib_netif.h test group
 ******************************************************************************/

static void test_media_lib_netif(void)
{
#if defined(CONFIG_MEDIA_LIB_NETIF_ENABLE)
    media_lib_ipv4_info_t ip_info;
    int ret = media_lib_netif_get_ipv4_info(0, &ip_info);
    TEST_ASSERT_TRUE(ret == 0 || ret != 0);
#endif
}

#if defined(CONFIG_MEDIA_LIB_NETIF_ENABLE)
TEST_CASE("test_netif_lib", "[media_lib_sal]")
{
    test_media_lib_netif();
}
#endif

/*******************************************************************************
 * media_lib_tls.h test group
 ******************************************************************************/

static void test_media_lib_tls(void)
{
#if defined(CONFIG_MEDIA_LIB_TLS_ENABLE)
    media_lib_tls_cfg_t cfg = {0};
    media_lib_tls_handle_t tls = media_lib_tls_new("localhost", 9, 443, &cfg);
    if (tls) {
        TEST_ASSERT_EQUAL(0, media_lib_tls_delete(tls));
    }
#endif
}

#if defined(CONFIG_MEDIA_LIB_TLS_ENABLE)
TEST_CASE("test_tls_lib", "[media_lib_sal]")
{
    test_media_lib_tls();
}
#endif

/*******************************************************************************
 * media_lib_mem_trace.h test group
 ******************************************************************************/

static void test_media_lib_mem_trace(void)
{
#ifdef CONFIG_MEDIA_LIB_MEM_AUTO_TRACE
    uint32_t used = 0, peak = 0;
    int ret = media_lib_get_mem_usage(NULL, &used, &peak);
    TEST_ASSERT_EQUAL(0, ret);
#endif
}

#ifdef CONFIG_MEDIA_LIB_MEM_AUTO_TRACE
TEST_CASE("test_mem_trace", "[media_lib_sal]")
{
    test_media_lib_mem_trace();
}
#endif
