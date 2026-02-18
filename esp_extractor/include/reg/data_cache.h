/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Data cache is a module that cache input data
 *         It avoids frequently read and seek operation when cache is fit
 *         And provide easy API for extractor to use to get data
 *         Cache logic is kept as simple:
 *           - If cache is fit, cache buffer is kept
 *           - Else cache is invalid and flushed
 */

/**
 * @brief  Cache open file callback
 */
typedef void *(*_cache_open_func)(char *url, void *input_ctx);

/**
 * @brief  Cache read data callback
 */
typedef int (*_cache_read_func)(void *buffer, uint32_t size, void *ctx);

/**
 * @brief  Cache read abort callback
 */
typedef int (*_cache_read_abort_func)(void *ctx);

/**
 * @brief  Cache seek to byte position callback
 */
typedef int (*_cache_seek_func)(uint32_t position, void *ctx);

/**
 * @brief  Cache get file size callback
 */
typedef uint32_t (*_cache_get_filesize_func)(void *ctx);

/**
 * @brief  Close file callback
 */
typedef int (*_cache_close_func)(void *ctx);

/**
 * @brief  Struct for data cache
 */
typedef struct {
    // Input parameters keeper
    _cache_open_func          open;           /*!< Open callback */
    _cache_read_func          read;           /*!< Read callback */
    _cache_read_abort_func    read_abort;     /*!< Read abort */
    _cache_seek_func          seek;           /*!< Seek callback */
    _cache_get_filesize_func  get_file_size;  /*!< Get file size callback */
    _cache_close_func         close;          /*!< Close file callback */
    void                     *input_ctx;      /*!< Input context */
    // Data cache
    uint8_t                  *cache_buffer;       /*!< Cache buffer */
    uint32_t                  cache_buffer_size;  /*!< Cache buffer size */
    uint32_t                  block_size;         /*!< Cache buffer block size */
    uint32_t                  cache_start_pos;    /*!< Start file position of cache buffer */
    uint32_t                  cached_size;        /*!< Cached size */
    uint32_t                  read_pos;           /*!< Cached read position */
    uint32_t                  file_size;          /*!< File size */
    void                     *ctx;                /*!< File context */
    char                     *url;                /*!< File url */
    bool                      is_eof;             /*!< Reach file end or not */
    bool                      aborted;            /*!< File is aborted */
} data_cache_t;

/**
 * @brief  Configuration for data cache
 */
typedef struct {
    _cache_open_func          open;        /*!< Open callback */
    _cache_read_func          read;        /*!< Read callback */
    _cache_read_abort_func    read_abort;  /*!< Read abort callback */
    _cache_seek_func          seek;        /*!< Seek callback */
    _cache_get_filesize_func  file_size;   /*!< Get file size callback */
    _cache_close_func         close;       /*!< Close callback */
    char                     *url;         /*!< File url */
    void                     *input_ctx;   /*!< Input context */
    uint32_t                  cache_size;  /*!< Data cache size */
    uint32_t                  block_size;  /*!< Block size */
} data_cache_cfg_t;

/**
 * @brief  Create data cache
 *
 * @param[in]  cfg  Configuration of data cache
 *
 * @return
 *       - NULL    Fail to create
 *       - Others  On success
 */
data_cache_t *data_cache_create(data_cache_cfg_t *cfg);

/**
 * @brief  Open input files
 *
 * @param[in]  cfg  Configuration of data cache
 *
 * @return
 *       - 0       On Success
 *       - Others  Failed to open
 */
int data_cache_open(data_cache_t *cache);

/**
 * @brief  Read data from data cache
 *         If cache is missing, it will try to read from input read callback
 *
 * @param[in]  cache   Data cache instance
 * @param[in]  buffer  Buffer to save read data
 *                     When buffer set to NULL will cache to cache buffer without call seek
 * @param[in]  size    Size to read
 *
 * @return
 *       - >       0     Actual read size
 *       - Others  Fail to read data
 */
int data_cache_read(data_cache_t *cache, void *buffer, uint32_t size);

/**
 * @brief  Read data directly
 *
 * @note  This API is specially designed for read huge data from file
 *        It is unnecessary to cache it into cache buffer
 *        If read size is smaller than cache size it will still try to cache it
 *
 * @param[in]  cache   Data cache instance
 * @param[in]  buffer  Buffer to save read data
 * @param[in]  size    Size to read
 *
 * @return
 *       - >       0     Actual size read
 *       - Others  Failed to read data
 */
int data_cache_read_directly(data_cache_t *cache, void *buffer, uint32_t size);

/**
 * @brief  Read little endian integer value from data cache
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  data   Integer value to store
 *
 * @return
 *       - 0       On success
 *       - Others  Fail to read
 */
int data_cache_read_int_le(data_cache_t *cache, int *data);

/**
 * @brief  Read big endian integer value from data cache
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  data   Integer value to read
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to read
 */
int data_cache_read_int_be(data_cache_t *cache, int *data);

/**
 * @brief  Read string from data cache
 *
 * @note  String is terminated by \0 and is included
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  str    String value to read
 * @param[in]  len    Limit of string length to read
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to read
 */
int data_cache_read_string(data_cache_t *cache, char *str, int len);

/**
 * @brief  Read big endian short value from data cache
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  data   Short value to read
 *
 * @return
 *       - 0       On success
 *       - Others  Fail to read
 */
int data_cache_read_short_be(data_cache_t *cache, short int *data);

/**
 * @brief  Seek to byte position
 * @note  If seek position is in cache, no actual seek happen
 *        Only update cache read position
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  pos    Position to seek
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to seek
 */
int data_cache_seek(data_cache_t *cache, uint32_t pos);

/**
 * @brief  Skip certain bytes
 *
 * @note  If input support seek and skip huge size it will do seek
 *        If input not support seek it will read data until size matched
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  size   Size to skip
 *
 * @return
 *       - 0       On success
 *       - Others  Fail to seek
 */
int data_cache_skip(data_cache_t *cache, uint32_t size);

/**
 * @brief  Get current position
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - Value  Current position
 */
uint32_t data_cache_get_position(data_cache_t *cache);

/**
 * @brief  Get cache size
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - Value  Cached data size
 */
uint32_t data_cache_size(data_cache_t *cache);

/**
 * @brief  Get internal cache buffer
 *
 * @note  Specially for reuse among parse modules
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - Buffer  Internal cache buffer
 */
void *data_cache_get_cache_buffer(data_cache_t *cache);

/**
 * @brief  Get file size
 *
 * @note  File size is acquired during open and cached
 *        This API is not time consuming
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - File  size
 */
uint32_t data_cache_get_file_size(data_cache_t *cache);

/**
 * @brief  Flush data cache
 *
 * @note  Flush will keep the data unread in cache
 *        It is used to map cache start position to read position
 *        User can use `data_cache_get_cache_buffer` to get the data directly
 *
 * @param[in]  cache  Data cache instance
 */
void data_cache_flush(data_cache_t *cache);

/**
 * @brief  Reset data cache
 *
 * @note  All cache data will be cleared and cache position reset to file start
 *
 * @param[in]  cache  Data cache instance
 */
void data_cache_reset(data_cache_t *cache);

/**
 * @brief  Reuse data cache to read same or another URL
 *
 * @param[in]  cache  Data cache instance
 * @param[in]  url    New url, set to NULL to reload current URL
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to reload
 */
int data_cache_reload(data_cache_t *cache, char *url);

/**
 * @brief  Abort on going read
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - 0       On success
 *       - Others  Fail to close
 */
int data_cache_read_abort(data_cache_t *cache);

/**
 * @brief  Cancel abort
 *
 * @note  Abort can only cancel by external API
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to close
 */
int data_cache_cancel_abort(data_cache_t *cache);

/**
 * @brief  Close opened file handle and invalidate data cache
 *
 * @param[in]  cache  Data cache instance
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to close
 */
int data_cache_close(data_cache_t *cache);

/**
 * @brief  Destroy data cache
 */
void data_cache_destroy(data_cache_t *cache);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
