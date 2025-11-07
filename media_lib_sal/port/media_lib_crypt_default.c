/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_log.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"
#include "media_lib_crypt_reg.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "mbedtls/md5.h"
#include "esp_idf_version.h"
#include "aes/esp_aes.h"
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#include "mbedtls/compat-2.x.h"
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

#define RETURN_ON_NULL_HANDLE(h)                                               \
    if (h == NULL)   {                                                         \
        return ESP_ERR_INVALID_ARG;                                            \
    }

static void _md5_init(media_lib_md5_handle_t *ctx)
{
    mbedtls_md5_context *md5 = (mbedtls_md5_context *)media_lib_malloc(sizeof(mbedtls_md5_context));
    if (md5) {
        mbedtls_md5_init(md5);
        *ctx = md5;
    }
}

static void _md5_free(media_lib_md5_handle_t ctx)
{
    if (ctx) {
        mbedtls_md5_free((mbedtls_md5_context *)ctx);
        media_lib_free(ctx);
    }
}

static int _md5_start(media_lib_md5_handle_t ctx)
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_md5_starts_ret((mbedtls_md5_context *)ctx);
}

static int _md5_update(media_lib_md5_handle_t ctx, const unsigned char *input, size_t len)
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_md5_update_ret((mbedtls_md5_context *)ctx, input, len);
}

static int _md5_finish(media_lib_md5_handle_t ctx, unsigned char output[16])
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_md5_finish_ret((mbedtls_md5_context *)ctx, output);
}

static void _sha256_init(media_lib_sha256_handle_t *ctx)
{
    mbedtls_sha256_context *sha256 = (mbedtls_sha256_context *)media_lib_malloc(sizeof(mbedtls_sha256_context));
    if (sha256) {
        mbedtls_sha256_init(sha256);
        *ctx = sha256;
    }
}

static void _sha256_free(media_lib_sha256_handle_t ctx)
{
    if (ctx) {
        mbedtls_sha256_free((mbedtls_sha256_context *)ctx);
        media_lib_free(ctx);
    }
}

static int _sha256_start(media_lib_sha256_handle_t ctx)
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_sha256_starts_ret((mbedtls_sha256_context *)ctx, false);
}

static int _sha256_update(media_lib_sha256_handle_t ctx, const unsigned char *input, size_t len)
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_sha256_update_ret((mbedtls_sha256_context *)ctx, input, len);
}

static int _sha256_finish(media_lib_sha256_handle_t ctx, unsigned char output[16])
{
    RETURN_ON_NULL_HANDLE(ctx);
    return mbedtls_sha256_finish_ret((mbedtls_sha256_context *)ctx, output);
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
