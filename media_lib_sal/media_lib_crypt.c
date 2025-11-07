/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "media_lib_crypt.h"
#include "media_lib_crypt_reg.h"
#include "media_lib_common.h"

#ifdef CONFIG_MEDIA_LIB_CRYPT_ENABLE
static media_lib_crypt_t media_crypt_lib;

esp_err_t media_lib_crypt_register(media_lib_crypt_t *crypt_lib)
{
    MEDIA_LIB_DEFAULT_INSTALLER(crypt_lib, &media_crypt_lib, media_lib_crypt_t);
}

void media_lib_md5_init(media_lib_md5_handle_t *ctx)
{
    if (media_crypt_lib.md5_init) {
        media_crypt_lib.md5_init(ctx);
    }
}

void media_lib_md5_free(media_lib_md5_handle_t ctx)
{
    if (media_crypt_lib.md5_free) {
        media_crypt_lib.md5_free(ctx);
    }
}

int media_lib_md5_start(media_lib_md5_handle_t ctx)
{
    if (media_crypt_lib.md5_start) {
        return media_crypt_lib.md5_start(ctx);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_md5_update(media_lib_md5_handle_t ctx, const unsigned char *input, size_t len)
{
    if (media_crypt_lib.md5_update) {
        return media_crypt_lib.md5_update(ctx, input, len);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_md5_finish(media_lib_md5_handle_t ctx, unsigned char output[16])
{
    if (media_crypt_lib.md5_finish) {
        return media_crypt_lib.md5_finish(ctx, output);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

void media_lib_sha256_init(media_lib_sha256_handle_t *ctx)
{
    if (media_crypt_lib.sha256_init) {
        media_crypt_lib.sha256_init(ctx);
    }
}

void media_lib_sha256_free(media_lib_sha256_handle_t ctx)
{
    if (media_crypt_lib.sha256_free) {
        media_crypt_lib.sha256_free(ctx);
    }
}

int media_lib_sha256_start(media_lib_sha256_handle_t ctx)
{
    if (media_crypt_lib.sha256_start) {
        return media_crypt_lib.sha256_start(ctx);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_sha256_update(media_lib_sha256_handle_t ctx, const unsigned char *input, size_t len)
{
    if (media_crypt_lib.sha256_update) {
        return media_crypt_lib.sha256_update(ctx, input, len);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_sha256_finish(media_lib_sha256_handle_t ctx, unsigned char output[32])
{
    if (media_crypt_lib.sha256_finish) {
        return media_crypt_lib.sha256_finish(ctx, output);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

void media_lib_aes_init(media_lib_aes_handle_t *ctx)
{
    if (media_crypt_lib.aes_init) {
        media_crypt_lib.aes_init(ctx);
    }
}

void media_lib_aes_free(media_lib_aes_handle_t ctx)
{
    if (media_crypt_lib.aes_free) {
        media_crypt_lib.aes_free(ctx);
    }
}

int media_lib_aes_set_key(media_lib_aes_handle_t ctx, uint8_t *key, uint8_t key_bits)
{
    if (media_crypt_lib.aes_set_key) {
        media_crypt_lib.aes_set_key(ctx, key, key_bits);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

int media_lib_aes_crypt_cbc(media_lib_aes_handle_t ctx, bool decrypt_mode, uint8_t iv[16], uint8_t *input, size_t size, uint8_t *output)
{
    if (media_crypt_lib.aes_crypt_cbc) {
        media_crypt_lib.aes_crypt_cbc(ctx, decrypt_mode, iv, input, size, output);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
