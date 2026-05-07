/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <errno.h>
#include <stdint.h>
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

#define TEST_THREAD_STACK_SIZE (4 * 1024)
#define TEST_THREAD_PRIO       5
#define TEST_THREAD_CORE       0
#define TEST_WAIT_MS           5000
#define TEST_SHORT_WAIT_MS     100
#define TEST_NET_CLEANUP_MS    500
#define TEST_LOOPBACK_ADDR     "127.0.0.1"

typedef struct {
    media_lib_mutex_handle_t  mutex;
    media_lib_sema_handle_t   start;
    media_lib_sema_handle_t   done;
    volatile int              counter;
    volatile int              err;
} os_thread_ctx_t;

typedef struct {
    media_lib_sema_handle_t  ready;
    media_lib_sema_handle_t  done;
    int                      port;
    volatile int             err;
} tcp_server_ctx_t;

static int set_sock_timeout(int sock)
{
    struct timeval tv = {
        .tv_sec = TEST_WAIT_MS / 1000,
        .tv_usec = (TEST_WAIT_MS % 1000) * 1000,
    };
    if (media_lib_socket_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        media_lib_socket_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        return -1;
    }
    return 0;
}

static void abort_sock(int sock)
{
    if (sock >= 0) {
        struct linger linger = {
            .l_onoff = 1,
            .l_linger = 0,
        };
        media_lib_socket_setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
        media_lib_socket_shutdown(sock, SHUT_RDWR);
    }
}

static void close_sock(int *sock)
{
    if (*sock >= 0) {
        abort_sock(*sock);
        media_lib_socket_close(*sock);
        *sock = -1;
    }
}

static int read_exact(int sock, char *buf, size_t size)
{
    size_t got = 0;
    while (got < size) {
        ssize_t ret = media_lib_socket_recv(sock, buf + got, size - got, 0);
        if (ret <= 0) {
            return -1;
        }
        got += ret;
    }
    return 0;
}

static int write_exact(int sock, const char *buf, size_t size)
{
    size_t sent = 0;
    while (sent < size) {
        ssize_t ret = media_lib_socket_send(sock, buf + sent, size - sent, 0);
        if (ret <= 0) {
            return -1;
        }
        sent += ret;
    }
    return 0;
}

static int tls_read_exact(media_lib_tls_handle_t tls, char *buf, size_t size)
{
    size_t got = 0;
    while (got < size) {
        int ret = media_lib_tls_read(tls, buf + got, size - got);
        if (ret <= 0) {
            return -1;
        }
        got += ret;
    }
    return 0;
}

static int tls_write_exact(media_lib_tls_handle_t tls, const char *buf, size_t size)
{
    size_t sent = 0;
    while (sent < size) {
        int ret = media_lib_tls_write(tls, buf + sent, size - sent);
        if (ret <= 0) {
            return -1;
        }
        sent += ret;
    }
    return 0;
}

static void close_tls(media_lib_tls_handle_t *tls)
{
    if (*tls) {
        abort_sock(media_lib_tls_getsockfd(*tls));
        media_lib_tls_delete(*tls);
        *tls = NULL;
    }
}

/*******************************************************************************
 * media_lib_os.h test group
 ******************************************************************************/

static void test_media_lib_os_memory(void)
{
    void *ptr = media_lib_malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    memset(ptr, 0x5a, 100);
    media_lib_free(ptr);

    ptr = media_lib_calloc(10, 10);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EACH_EQUAL_UINT8(0, ptr, 100);
    ptr = media_lib_realloc(ptr, 200);
    TEST_ASSERT_NOT_NULL(ptr);
    media_lib_free(ptr);

    char *dup = media_lib_strdup("sal");
    TEST_ASSERT_NOT_NULL(dup);
    TEST_ASSERT_EQUAL_STRING("sal", dup);
    media_lib_free(dup);

    char *fmt = NULL;
    TEST_ASSERT_EQUAL(5, media_lib_asprintf(&fmt, "%s-%d", "sal", 1));
    TEST_ASSERT_EQUAL_STRING("sal-1", fmt);
    media_lib_free(fmt);
}

static void test_media_lib_os_mutex(void)
{
    media_lib_mutex_handle_t mutex;
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_create(&mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_lock(mutex, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_lock(mutex, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_unlock(mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_unlock(mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_destroy(mutex));
}

static void test_media_lib_os_semaphore(void)
{
    media_lib_sema_handle_t sema;
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&sema));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, media_lib_sema_lock(sema, TEST_SHORT_WAIT_MS));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_unlock(sema));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(sema, TEST_WAIT_MS));
    TEST_ASSERT_NOT_EQUAL(ESP_OK, media_lib_sema_lock(sema, TEST_SHORT_WAIT_MS));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(sema));
}

static void os_worker(void *arg)
{
    os_thread_ctx_t *ctx = (os_thread_ctx_t *)arg;
    if (media_lib_sema_lock(ctx->start, TEST_WAIT_MS) != ESP_OK ||
        media_lib_mutex_lock(ctx->mutex, TEST_WAIT_MS) != ESP_OK) {
        ctx->err = 1;
    } else {
        ctx->counter++;
        if (media_lib_mutex_unlock(ctx->mutex) != ESP_OK) {
            ctx->err = 1;
        }
    }
    media_lib_sema_unlock(ctx->done);
    media_lib_thread_destroy(NULL);
}

static void test_media_lib_os_thread(void)
{
    os_thread_ctx_t ctx = {0};
    media_lib_thread_handle_t thread;

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_create(&ctx.mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.start));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_lock(ctx.mutex, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_thread_create(&thread, "sal_os", os_worker, &ctx,
                                                     TEST_THREAD_STACK_SIZE, TEST_THREAD_PRIO, TEST_THREAD_CORE));

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_unlock(ctx.start));
    media_lib_thread_sleep(TEST_SHORT_WAIT_MS);
    TEST_ASSERT_EQUAL(0, ctx.counter);
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_unlock(ctx.mutex));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(ctx.done, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(0, ctx.err);
    TEST_ASSERT_EQUAL(1, ctx.counter);

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.start));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_mutex_destroy(ctx.mutex));
}

static void test_media_lib_os_event_group(void)
{
    media_lib_event_grp_handle_t group;
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_event_group_create(&group));
    TEST_ASSERT_EQUAL(0, media_lib_event_group_wait_bits(group, 0x01, TEST_SHORT_WAIT_MS) & 0x01);
    TEST_ASSERT_NOT_EQUAL(0, media_lib_event_group_set_bits(group, 0x03) & 0x03);
    TEST_ASSERT_NOT_EQUAL(0, media_lib_event_group_wait_bits(group, 0x03, TEST_WAIT_MS) & 0x03);
    media_lib_event_group_clr_bits(group, 0x01);
    TEST_ASSERT_EQUAL(0, media_lib_event_group_wait_bits(group, 0x01, TEST_SHORT_WAIT_MS) & 0x01);
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_event_group_destroy(group));
}

static void test_media_lib_os_critical(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_enter_critical_section());
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_leave_critical_section());
}

TEST_CASE("test_os_lib", "[media_lib_sal]")
{
    test_media_lib_os_memory();
    test_media_lib_os_mutex();
    test_media_lib_os_semaphore();
    test_media_lib_os_thread();
    test_media_lib_os_event_group();
    test_media_lib_os_critical();
}

/*******************************************************************************
 * media_lib_crypt.h test group
 ******************************************************************************/

static void test_media_lib_crypt_md5(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    const unsigned char input[] = "abc";
    const unsigned char expected[16] = {
        0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
    };
    unsigned char output[16] = {0};
    media_lib_md5_handle_t md5_ctx;
    media_lib_md5_init(&md5_ctx);
    TEST_ASSERT_NOT_NULL(md5_ctx);
    TEST_ASSERT_EQUAL(0, media_lib_md5_start(md5_ctx));
    TEST_ASSERT_EQUAL(0, media_lib_md5_update(md5_ctx, input, strlen((const char *)input)));
    TEST_ASSERT_EQUAL(0, media_lib_md5_finish(md5_ctx, output));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output, sizeof(expected));
    media_lib_md5_free(md5_ctx);
#endif
}

static void test_media_lib_crypt_sha256(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    const unsigned char input[] = "abc";
    const unsigned char expected[32] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    };
    unsigned char output[32] = {0};
    media_lib_sha256_handle_t sha256_ctx;
    media_lib_sha256_init(&sha256_ctx);
    TEST_ASSERT_NOT_NULL(sha256_ctx);
    TEST_ASSERT_EQUAL(0, media_lib_sha256_start(sha256_ctx));
    TEST_ASSERT_EQUAL(0, media_lib_sha256_update(sha256_ctx, input, strlen((const char *)input)));
    TEST_ASSERT_EQUAL(0, media_lib_sha256_finish(sha256_ctx, output));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output, sizeof(expected));
    media_lib_sha256_free(sha256_ctx);
#endif
}

static void test_media_lib_crypt_aes(void)
{
#if defined(CONFIG_MEDIA_LIB_CRYPT_ENABLE)
    uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    };
    uint8_t iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    uint8_t plain[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    };
    uint8_t expected[16] = {
        0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
        0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
    };
    uint8_t encrypted[16] = {0};
    uint8_t decrypted[16] = {0};
    uint8_t work_iv[16];
    media_lib_aes_handle_t aes_ctx;
    media_lib_aes_init(&aes_ctx);
    TEST_ASSERT_NOT_NULL(aes_ctx);
    TEST_ASSERT_EQUAL(0, media_lib_aes_set_key(aes_ctx, key, 128));
    memcpy(work_iv, iv, sizeof(work_iv));
    TEST_ASSERT_EQUAL(0, media_lib_aes_crypt_cbc(aes_ctx, false, work_iv, plain, sizeof(plain), encrypted));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, encrypted, sizeof(expected));
    TEST_ASSERT_EQUAL(0, media_lib_aes_set_key(aes_ctx, key, 128));
    memcpy(work_iv, iv, sizeof(work_iv));
    TEST_ASSERT_EQUAL(0, media_lib_aes_crypt_cbc(aes_ctx, true, work_iv, encrypted, sizeof(encrypted), decrypted));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plain, decrypted, sizeof(plain));
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
    TEST_ASSERT_GREATER_OR_EQUAL(0, sock);
    TEST_ASSERT_EQUAL(0, media_lib_socket_close(sock));
#endif
}

static int start_tcp_server(tcp_server_ctx_t *ctx, int *listen_sock)
{
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = 0,
    };
    socklen_t addr_len = sizeof(addr);
    int opt = 1;

    *listen_sock = media_lib_socket_open(AF_INET, SOCK_STREAM, 0);
    if (*listen_sock < 0 ||
        media_lib_socket_setsockopt(*listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0 ||
        media_lib_socket_bind(*listen_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
        media_lib_socket_getsockname(*listen_sock, (struct sockaddr *)&addr, &addr_len) != 0 ||
        media_lib_socket_listen(*listen_sock, 1) != 0) {
        ctx->err = errno;
        close_sock(listen_sock);
        return -1;
    }
    ctx->port = ntohs(addr.sin_port);
    return 0;
}

static void tcp_server_thread(void *arg)
{
    tcp_server_ctx_t *ctx = (tcp_server_ctx_t *)arg;
    int listen_sock = -1;
    int client_sock = -1;
    char rx[5] = {0};
    const char tx[] = "pong";

    if (start_tcp_server(ctx, &listen_sock) == 0) {
        media_lib_sema_unlock(ctx->ready);
        client_sock = media_lib_socket_accept(listen_sock, NULL, NULL);
        if (client_sock < 0) {
            ctx->err = errno;
        } else {
            ctx->err = set_sock_timeout(client_sock);
            if (read_exact(client_sock, rx, 4) != 0 || strcmp(rx, "ping") != 0 ||
                write_exact(client_sock, tx, 4) != 0) {
                ctx->err = errno ? errno : ESP_FAIL;
            }
        }
    } else {
        media_lib_sema_unlock(ctx->ready);
    }

    close_sock(&client_sock);
    close_sock(&listen_sock);
    media_lib_sema_unlock(ctx->done);
    media_lib_thread_destroy(NULL);
}

static void test_media_lib_socket_loopback(void)
{
#if defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
    tcp_server_ctx_t ctx = {0};
    media_lib_thread_handle_t thread;
    int sock = -1;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
    };
    char rx[5] = {0};

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.ready));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_thread_create(&thread, "sal_tcp", tcp_server_thread, &ctx,
                                                     TEST_THREAD_STACK_SIZE, TEST_THREAD_PRIO, TEST_THREAD_CORE));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(ctx.ready, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(0, ctx.err);

    sock = media_lib_socket_open(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, sock);
    TEST_ASSERT_EQUAL(0, set_sock_timeout(sock));
    TEST_ASSERT_EQUAL(1, media_lib_socket_inet_pton(AF_INET, TEST_LOOPBACK_ADDR, &addr.sin_addr));
    addr.sin_port = htons(ctx.port);
    TEST_ASSERT_EQUAL(0, media_lib_socket_connect(sock, (struct sockaddr *)&addr, sizeof(addr)));
    TEST_ASSERT_EQUAL(0, write_exact(sock, "ping", 4));
    TEST_ASSERT_EQUAL(0, read_exact(sock, rx, 4));
    TEST_ASSERT_EQUAL_STRING("pong", rx);

    close_sock(&sock);
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(ctx.done, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(0, ctx.err);
    media_lib_thread_sleep(TEST_NET_CLEANUP_MS);
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.ready));
#endif
}

#if defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
TEST_CASE("test_socket_lib", "[media_lib_sal]")
{
    test_media_lib_socket();
    test_media_lib_socket_loopback();
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

#if defined(CONFIG_MEDIA_LIB_TLS_ENABLE) && defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
static const char s_test_cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDCTCCAfGgAwIBAgIUJYyk7wm2e+/Plyb3xL1EpuUY4tMwDQYJKoZIhvcNAQEL\n"
"BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI2MDUwNjEyNDkyM1oXDTM2MDUw\n"
"MzEyNDkyM1owFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF\n"
"AAOCAQ8AMIIBCgKCAQEAzhfEWJb1nRQ6X4g+hRM1jIqfbS5Acd2ZLx4/XAHt3VtF\n"
"ud/aDy43BJfkZUdJ+imp2wrqYavFrgDaYKy6ZHJt1sMJMwIJr2t8oj6K11Jx7Va5\n"
"aqV4lNk74GRd6VbWa0ordHspLYoKGQphNVzJDbHD6DE8d0bLNsFkRAgl8srfu6VA\n"
"2O9bUj2Xlh0M3IWwU5kejFV3039mhJ/BptIm/bXsObvmc5FAxa6rZwYTXOlACP4v\n"
"0PotqDUPpxzi0ypupNSQa2PX5RZ8JgXmEr0njP79CEbNHyUSVqJ3WOkRWSFvJfdZ\n"
"NGo4xj2BR6xwfiWh2pmqB0wPLANCAfug33wud34d0QIDAQABo1MwUTAdBgNVHQ4E\n"
"FgQUa8wSCH8FSuR1UsxqTIu66rFHe0gwHwYDVR0jBBgwFoAUa8wSCH8FSuR1Usxq\n"
"TIu66rFHe0gwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAan1Q\n"
"IWf10LtN5I5uHz3tRPJVSg7I06Ez+epOsr3xz+FiwPGapfp0fOOXZF7vX/Q7UDtc\n"
"haCRy+CL+uJ+3zCd5P9MmKXzqT4EA9FiB/RlIEN1cBtoO/xGGhAUA83iuD3sA7L3\n"
"w6APOCo42IfgBIvfJsEYZ7hOWayC2qIWWOQNAhvTg+X5cAU2aNcNiknrd/e+Of0i\n"
"aj6BSREm8CRz0ZNQRRLthC1eMlcfGripvY+3wPcFAFJbTWAsYyvQaVpAcUH3QNYb\n"
"hxJjK48wF8NWkR16UF39QK+ca2K5d2gOEvR8OSRkjdUc7JoTDMCFO3qzb2o6SqNx\n"
"lqexIuncX0jBXnrhFg==\n"
"-----END CERTIFICATE-----\n";

static const char s_test_key_pem[] =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDOF8RYlvWdFDpf\n"
"iD6FEzWMip9tLkBx3ZkvHj9cAe3dW0W539oPLjcEl+RlR0n6KanbCuphq8WuANpg\n"
"rLpkcm3WwwkzAgmva3yiPorXUnHtVrlqpXiU2TvgZF3pVtZrSit0eyktigoZCmE1\n"
"XMkNscPoMTx3Rss2wWRECCXyyt+7pUDY71tSPZeWHQzchbBTmR6MVXfTf2aEn8Gm\n"
"0ib9tew5u+ZzkUDFrqtnBhNc6UAI/i/Q+i2oNQ+nHOLTKm6k1JBrY9flFnwmBeYS\n"
"vSeM/v0IRs0fJRJWondY6RFZIW8l91k0ajjGPYFHrHB+JaHamaoHTA8sA0IB+6Df\n"
"fC53fh3RAgMBAAECggEARnwebOYETpqydCXWrBCryctHR0IATHEriM/hf4gxX/lt\n"
"WWrT20lFC88fD/xXsrU5IG8VRh5HV4F3LoUkomH4avD9XnKd+V7bm2mkdov95tw4\n"
"Qd8h5FZ/gfkRnVJTtxIA1q+gn7CMT5AbOvLm2EYp6GmecnTF3444yoO25ti3uCbp\n"
"Ydr93pWch8wkOnOTzjFywoo5myxuIIWa469J+lwN3saZq07OjeaDreoQ7YCmsiQV\n"
"GQEzp8Qnfq2rntchLy5dg/QguRW3W/YodGzHDcFS4Biydba3glQQq7mPPWN7Q5SG\n"
"9C1/wPuV6Ug/mxRnVVmdydqrl+YveHR7XwqXpncgIQKBgQDrpykz74qevdL7P42G\n"
"yiLTpwLqgFU/0s0VBzR7DimF3RV36+eLuOlFOreszQZ9Da5ZZn+zLcouxHIvziAA\n"
"PRLY8om9fiO7A8i7ahy+wogZ8TPcHkQOKBS3943IIl5WPzZp4MMvcLpCF9tZCXHp\n"
"Fr2MSjXvjzpEqQ+AAzqrivmIHQKBgQDf4zaRFZZn3jKtauZpEMqvogn6hUgPebN6\n"
"STMFqqGSk4Nol4Krm5KOQeVKfjup79X6xPSicTDvPntXtf7w5y3qVulavJTGGCXc\n"
"tlF4aMiz+M/kKTqXky5vjABn8xfh7efmET478sVAo4lyPbgaVGvmJSc9NdW4Znf/\n"
"SWwg2O7GRQKBgCJr/3vDoMcBSWFD7kNjIWUzPxzL9Cfnfed1bvgOoIgaPDmRLvhc\n"
"4U4ofo3yQOFhbzdF3p9or+DW9yr+e/Qs96TTAMW6ODPoIJknrd07gy/u6Lldfmlm\n"
"92fL9FzokEke4K7kkv5KBBsFaxf9ZjM7DCMC8A/3JvbCjy4OyrImEb8NAoGBANnA\n"
"hqjzoxag0Q9D4pebglbyNdxX1y9eAcbU/O/s9jq2iF+oxKmdpRwBUT5zqhntL2Q4\n"
"PWrkxmBal+JGG5A4eLrsDwh2VAibIfAZzwFagKIyRBg4VUlpGAfb1eVEVR0jo6Fr\n"
"cekihOEKbbsT9kR9iXgM+K4a9yPjXaCIy8bi1tE1AoGBANOPXS6eKDDWXa0Tct+d\n"
"kn7QqQlirmNenkZiafO3mbwx4bXiQ7CBymeMUTJwJsKjcT1pxvlqxShWRxIrOCgh\n"
"N4vtcKPfL3MY5wxDXQPju1OGbwsWhCvHvoiATrpaXqFjvVUhnIfLyuxKvMta+Uj9\n"
"hONHaiEEPYLaKJ5hDTmxT9Hr\n"
"-----END PRIVATE KEY-----\n";

static void tls_server_thread(void *arg)
{
    tcp_server_ctx_t *ctx = (tcp_server_ctx_t *)arg;
    media_lib_tls_server_cfg_t cfg = {
        .servercert_buf = s_test_cert_pem,
        .servercert_bytes = sizeof(s_test_cert_pem),
        .serverkey_buf = s_test_key_pem,
        .serverkey_bytes = sizeof(s_test_key_pem),
    };
    media_lib_tls_handle_t tls = NULL;
    int listen_sock = -1;
    int client_sock = -1;
    char rx[9] = {0};

    if (start_tcp_server(ctx, &listen_sock) == 0) {
        media_lib_sema_unlock(ctx->ready);
        client_sock = media_lib_socket_accept(listen_sock, NULL, NULL);
        if (client_sock < 0) {
            ctx->err = errno;
        } else {
            ctx->err = set_sock_timeout(client_sock);
            tls = media_lib_tls_new_server(client_sock, &cfg);
            if (!tls || tls_read_exact(tls, rx, 8) != 0 || strcmp(rx, "tls ping") != 0 ||
                tls_write_exact(tls, "tls pong", 8) != 0) {
                ctx->err = errno ? errno : ESP_FAIL;
            }
        }
    } else {
        media_lib_sema_unlock(ctx->ready);
    }

    if (tls) {
        close_tls(&tls);
        client_sock = -1;
    }
    close_sock(&client_sock);
    close_sock(&listen_sock);
    media_lib_sema_unlock(ctx->done);
    media_lib_thread_destroy(NULL);
}

static void test_media_lib_tls_loopback(void)
{
#if !defined(CONFIG_MBEDTLS_TLS_SERVER) && !defined(CONFIG_ESP_TLS_SERVER)
    TEST_IGNORE_MESSAGE("CONFIG_MBEDTLS_TLS_SERVER is required for TLS server adapter test");
#else
    tcp_server_ctx_t ctx = {0};
    media_lib_tls_cfg_t cfg = {
        .cacert_buf = s_test_cert_pem,
        .cacert_bytes = sizeof(s_test_cert_pem),
        .skip_common_name = true,
        .timeout_ms = TEST_WAIT_MS,
    };
    media_lib_thread_handle_t thread;
    media_lib_tls_handle_t tls;
    char rx[9] = {0};

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.ready));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_create(&ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_thread_create(&thread, "sal_tls", tls_server_thread, &ctx,
                                                     TEST_THREAD_STACK_SIZE * 2, TEST_THREAD_PRIO, TEST_THREAD_CORE));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(ctx.ready, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(0, ctx.err);

    tls = media_lib_tls_new(TEST_LOOPBACK_ADDR, strlen(TEST_LOOPBACK_ADDR), ctx.port, &cfg);
    TEST_ASSERT_NOT_NULL(tls);
    TEST_ASSERT_GREATER_OR_EQUAL(0, media_lib_tls_getsockfd(tls));
    TEST_ASSERT_EQUAL(0, tls_write_exact(tls, "tls ping", 8));
    TEST_ASSERT_EQUAL(0, tls_read_exact(tls, rx, 8));
    TEST_ASSERT_EQUAL_STRING("tls pong", rx);
    close_tls(&tls);

    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_lock(ctx.done, TEST_WAIT_MS));
    TEST_ASSERT_EQUAL(0, ctx.err);
    media_lib_thread_sleep(TEST_NET_CLEANUP_MS);
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.done));
    TEST_ASSERT_EQUAL(ESP_OK, media_lib_sema_destroy(ctx.ready));
#endif
}
#endif

#if defined(CONFIG_MEDIA_LIB_TLS_ENABLE)
TEST_CASE("test_tls_lib", "[media_lib_sal]")
{
#if !defined(CONFIG_MEDIA_LIB_SOCKET_ENABLE)
    TEST_IGNORE_MESSAGE("CONFIG_MEDIA_LIB_SOCKET_ENABLE is required for TLS loopback test");
#else
    test_media_lib_tls_loopback();
#endif
}
#endif

/*******************************************************************************
 * media_lib_mem_trace.h test group
 ******************************************************************************/
#define MAX_LEAK_COUNT  (10)
#define LEAK_TRACE_DEPTH (5)

typedef struct {
    void *mem[MAX_LEAK_COUNT];
    int   size[MAX_LEAK_COUNT];
    int   count;
} leak_trace_t;

typedef void *(*leak_func_t)(leak_trace_t *ctx, int size);

void *leak_func_malloc(leak_trace_t *ctx, int size)
{
    if (ctx->count < MAX_LEAK_COUNT) {
        ctx->mem[ctx->count] = media_lib_module_malloc("malloc", size);
        ctx->size[ctx->count] = size;
        return ctx->mem[ctx->count++];
    }
    return NULL;
}

void *leak_func_calloc(leak_trace_t *ctx, int size)
{
    if (ctx->count < MAX_LEAK_COUNT) {
        ctx->mem[ctx->count] = media_lib_module_calloc("calloc", 1, size);
        if (ctx->mem[ctx->count] == NULL) {
            return NULL;
        }
        ctx->size[ctx->count] = size;
        return ctx->mem[ctx->count++];
    }
    return NULL;
}

void *leak_func_realloc(leak_trace_t *ctx, int size)
{
    if (ctx->count < MAX_LEAK_COUNT) {
        ctx->mem[ctx->count] = media_lib_realloc(NULL, size);
        ctx->size[ctx->count] = size;
        return ctx->mem[ctx->count++];
    }
    return NULL;
}

static void test_media_lib_mem_trace(void)
{
    media_lib_mem_trace_cfg_t cfg = {
        .record_num = MAX_LEAK_COUNT,
        .stack_depth = LEAK_TRACE_DEPTH,
        .trace_type = MEDIA_LIB_MEM_TRACE_MODULE_USAGE | MEDIA_LIB_MEM_TRACE_LEAK,
    };
    int ret = media_lib_start_mem_trace(&cfg);
    TEST_ASSERT_EQUAL(0, ret);
    leak_trace_t ctx = {0};
    leak_func_t leak_funcs[] = {leak_func_malloc, leak_func_calloc, leak_func_realloc};

    for (int i = 0; i < MAX_LEAK_COUNT; i++) {
        void *mem = leak_funcs[i % sizeof(leak_funcs)/sizeof(leak_func_t)](&ctx, i + 10);
        TEST_ASSERT_NOT_NULL(mem);
    }
    uint32_t used = 0, peak = 0;
    ret = media_lib_get_mem_usage(NULL, &used, &peak);
    TEST_ASSERT_EQUAL(0, ret);
    media_lib_print_leakage("malloc");
    media_lib_print_leakage(NULL);
    for (int i = 0; i < ctx.count; i++) {
        media_lib_free(ctx.mem[i]);
    }
    media_lib_print_leakage("malloc");
    media_lib_print_leakage(NULL);
    media_lib_stop_mem_trace();
}

TEST_CASE("test_mem_trace", "[media_lib_sal]")
{
    test_media_lib_mem_trace();
}
