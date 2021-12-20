/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2021 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifndef MEDIA_LIB_CRYPT_H
#define MEDIA_LIB_CRYPT_H

#include "media_lib_crypt_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     md5 init wrapper
 *
 * @param[out]  ctx  md5 struct pointer
 */
void media_lib_md5_init(media_lib_md5_handle_t *ctx);

/**
 * @brief     md5 deint wrapper
 *
 * @param  ctx  md5 instance
 */
void media_lib_md5_free(media_lib_md5_handle_t ctx);

/**
 * @brief     md5 start wrapper
 *
 * @param  ctx  md5 instance
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: md5 init fail
 */
int media_lib_md5_start(media_lib_md5_handle_t ctx);

/**
 * @brief      md5 add input data
 * 
 * @param      ctx     md5 instance
 * @param      input   input data
 * @param      len     input length
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: md5 update fail
 */
int media_lib_md5_update(media_lib_md5_handle_t ctx, const unsigned char *input, size_t len);

/**
 * @brief      get md5 output
 * 
 * @param      ctx     md5 instance
 * @param      output  md5 value of input
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: md5 finish fail
 */
int media_lib_md5_finish(media_lib_md5_handle_t ctx, unsigned char output[16]);

/**
 * @brief     sha256 init wrapper
 *
 * @param[out]  ctx  sha256 struct pointer
 */
void media_lib_sha256_init(media_lib_sha256_handle_t *ctx);

/**
 * @brief     sha256 deint wrapper
 *
 * @param  ctx  sha256 instance
 */
void media_lib_sha256_free(media_lib_sha256_handle_t ctx);

/**
 * @brief     sha256 start wrapper
 *
 * @param  ctx  sha256 instance
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: sha256 start fail
 */
int media_lib_sha256_start(media_lib_sha256_handle_t ctx);

/**
 * @brief      sha256 add input data
 * 
 * @param      ctx     sha256 instance
 * @param      input   input data
 * @param      len     input length
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: sha256 update fail
 */
int media_lib_sha256_update(media_lib_sha256_handle_t ctx, const unsigned char *input, size_t len);

/**
 * @brief      get sha256 output
 * 
 * @param      ctx     sha256 instance
 * @param      output  sha256 value of input
 * 
 * @return      
 *              - 0: on success
 *              - ESP_ERR_NOT_SUPPORTED: wrapper function not registered
 *              - Others: sha256 finish fail
 */
int media_lib_sha256_finish(media_lib_sha256_handle_t ctx, unsigned char output[32]);

#ifdef __cplusplus
}
#endif

#endif