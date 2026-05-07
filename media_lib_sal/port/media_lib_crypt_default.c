/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_log.h"
#include "media_lib_crypt_reg.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "esp_idf_version.h"

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
#include "psa/crypto.h"
#else
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/version.h"
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#include "mbedtls/compat-2.x.h"
#endif
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0))
#include "aes/esp_aes.h"
#elif (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#if CONFIG_IDF_TARGET_ESP32
#include "esp32/aes.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/aes.h"
#endif
#else
#include "hwcrypto/aes.h"
#endif

#ifdef CONFIG_MEDIA_LIB_CRYPT_ENABLE

#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0))
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
#define sal_mbedtls_md5_starts(ctx)          mbedtls_md5_starts(ctx)
#define sal_mbedtls_md5_update(ctx, in, len) mbedtls_md5_update(ctx, in, len)
#define sal_mbedtls_md5_finish(ctx, out)     mbedtls_md5_finish(ctx, out)
#define sal_mbedtls_sha256_starts(ctx)       mbedtls_sha256_starts(ctx, 0)
#define sal_mbedtls_sha256_update(ctx, in, len) mbedtls_sha256_update(ctx, in, len)
#define sal_mbedtls_sha256_finish(ctx, out)  mbedtls_sha256_finish(ctx, out)
#else
#define sal_mbedtls_md5_starts(ctx)          mbedtls_md5_starts_ret(ctx)
#define sal_mbedtls_md5_update(ctx, in, len) mbedtls_md5_update_ret(ctx, in, len)
#define sal_mbedtls_md5_finish(ctx, out)     mbedtls_md5_finish_ret(ctx, out)
#define sal_mbedtls_sha256_starts(ctx)       mbedtls_sha256_starts_ret(ctx, false)
#define sal_mbedtls_sha256_update(ctx, in, len) mbedtls_sha256_update_ret(ctx, in, len)
#define sal_mbedtls_sha256_finish(ctx, out)  mbedtls_sha256_finish_ret(ctx, out)
#endif
#endif

#define RETURN_ON_NULL_HANDLE(h)                                               \
    if (h == NULL)   {                                                         \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
typedef struct {
    psa_hash_operation_t op;
    psa_algorithm_t      alg;
} sal_psa_hash_ctx_t;

static void _hash_init(void **ctx, psa_algorithm_t alg)
{
    sal_psa_hash_ctx_t *hash = (sal_psa_hash_ctx_t *)media_lib_calloc(1, sizeof(sal_psa_hash_ctx_t));
    if (hash) {
        hash->op = psa_hash_operation_init();
        hash->alg = alg;
        *ctx = hash;
    }
}

static void _hash_free(void *ctx)
{
    if (ctx) {
        sal_psa_hash_ctx_t *hash = (sal_psa_hash_ctx_t *)ctx;
        psa_hash_abort(&hash->op);
        media_lib_free(ctx);
    }
}

static int _hash_start(void *ctx)
{
    RETURN_ON_NULL_HANDLE(ctx);
    sal_psa_hash_ctx_t *hash = (sal_psa_hash_ctx_t *)ctx;
    psa_status_t ret = psa_crypto_init();
    if (ret != PSA_SUCCESS) {
        return ret;
    }
    psa_hash_abort(&hash->op);
    hash->op = psa_hash_operation_init();
    return psa_hash_setup(&hash->op, hash->alg);
}

static int _hash_update(void *ctx, const unsigned char *input, size_t len)
{
    RETURN_ON_NULL_HANDLE(ctx);
    sal_psa_hash_ctx_t *hash = (sal_psa_hash_ctx_t *)ctx;
    return psa_hash_update(&hash->op, input, len);
}

static int _hash_finish(void *ctx, unsigned char *output, size_t size)
{
    size_t hash_len = 0;
    RETURN_ON_NULL_HANDLE(ctx);
    sal_psa_hash_ctx_t *hash = (sal_psa_hash_ctx_t *)ctx;
    return psa_hash_finish(&hash->op, output, size, &hash_len);
}
#endif

static void _md5_init(media_lib_md5_handle_t *ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    _hash_init(ctx, PSA_ALG_MD5);
#else
    mbedtls_md5_context *md5 = (mbedtls_md5_context *)media_lib_malloc(sizeof(mbedtls_md5_context));
    if (md5) {
        mbedtls_md5_init(md5);
        *ctx = md5;
    }
#endif
}

static void _md5_free(media_lib_md5_handle_t ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    _hash_free(ctx);
#else
    if (ctx) {
        mbedtls_md5_free((mbedtls_md5_context *)ctx);
        media_lib_free(ctx);
    }
#endif
}

static int _md5_start(media_lib_md5_handle_t ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_start(ctx);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_md5_starts((mbedtls_md5_context *)ctx);
#endif
}

static int _md5_update(media_lib_md5_handle_t ctx, const unsigned char *input, size_t len)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_update(ctx, input, len);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_md5_update((mbedtls_md5_context *)ctx, input, len);
#endif
}

static int _md5_finish(media_lib_md5_handle_t ctx, unsigned char output[16])
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_finish(ctx, output, 16);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_md5_finish((mbedtls_md5_context *)ctx, output);
#endif
}

static void _sha256_init(media_lib_sha256_handle_t *ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    _hash_init(ctx, PSA_ALG_SHA_256);
#else
    mbedtls_sha256_context *sha256 = (mbedtls_sha256_context *)media_lib_malloc(sizeof(mbedtls_sha256_context));
    if (sha256) {
        mbedtls_sha256_init(sha256);
        *ctx = sha256;
    }
#endif
}

static void _sha256_free(media_lib_sha256_handle_t ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    _hash_free(ctx);
#else
    if (ctx) {
        mbedtls_sha256_free((mbedtls_sha256_context *)ctx);
        media_lib_free(ctx);
    }
#endif
}

static int _sha256_start(media_lib_sha256_handle_t ctx)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_start(ctx);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_sha256_starts((mbedtls_sha256_context *)ctx);
#endif
}

static int _sha256_update(media_lib_sha256_handle_t ctx, const unsigned char *input, size_t len)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_update(ctx, input, len);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_sha256_update((mbedtls_sha256_context *)ctx, input, len);
#endif
}

static int _sha256_finish(media_lib_sha256_handle_t ctx, unsigned char output[32])
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    return _hash_finish(ctx, output, 32);
#else
    RETURN_ON_NULL_HANDLE(ctx);
    return sal_mbedtls_sha256_finish((mbedtls_sha256_context *)ctx, output);
#endif
}

static void _aes_init(media_lib_aes_handle_t *ctx)
{
    esp_aes_context *aes = (esp_aes_context *)media_lib_malloc(sizeof(esp_aes_context));
    if (aes) {
        esp_aes_init(aes);
        *ctx = aes;
    }
}

static void _aes_free(media_lib_aes_handle_t ctx)
{
    if (ctx) {
        esp_aes_free((esp_aes_context *)ctx);
        media_lib_free(ctx);
    }
}

static int _aes_set_key(media_lib_aes_handle_t ctx, uint8_t *key, uint8_t key_bits)
{
    RETURN_ON_NULL_HANDLE(ctx);
    int ret = esp_aes_setkey((esp_aes_context *)ctx, key, key_bits);
    return ret;
}

static int _aes_crypt_cbc(media_lib_aes_handle_t ctx, bool decrypt_mode, uint8_t iv[16], uint8_t *input,
                          size_t size, uint8_t *output)
{
    RETURN_ON_NULL_HANDLE(ctx);
    int ret = esp_aes_crypt_cbc(
            (esp_aes_context *)ctx,
            decrypt_mode ? ESP_AES_DECRYPT : ESP_AES_ENCRYPT,
            size, iv, input, output);
    return ret;
}

esp_err_t media_lib_add_default_crypt_adapter(void)
{
    media_lib_crypt_t crypt_lib = {
        .md5_init = _md5_init,
        .md5_free = _md5_free,
        .md5_start = _md5_start,
        .md5_update = _md5_update,
        .md5_finish = _md5_finish,
        .sha256_init = _sha256_init,
        .sha256_free = _sha256_free,
        .sha256_start = _sha256_start,
        .sha256_update = _sha256_update,
        .sha256_finish = _sha256_finish,
        .aes_init = _aes_init,
        .aes_free = _aes_free,
        .aes_set_key = _aes_set_key,
        .aes_crypt_cbc = _aes_crypt_cbc,
    };
    return media_lib_crypt_register(&crypt_lib);
}
#endif
