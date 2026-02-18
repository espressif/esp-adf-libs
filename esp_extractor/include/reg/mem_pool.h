/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct mem_pool_t *mem_pool_handle_t;

/**
 * @brief  Create memory pool
 * @note  Memory pool manager a block of memory
 *        It avoids calling system malloc/free frequently
 *        Meanwhile memory can be shared across multiple modules
 *
 * @param[in]  size  Total memory size for memory pool to manage
 *
 * @return
 *       - NULL    Memory is not enough
 *       - Others  Memory pool instance
 */
mem_pool_handle_t mem_pool_create(uint32_t size);

/**
 * @brief  Allocate memory from memory pool
 *
 * @param[in]  pool  Memory pool instance
 * @param[in]  size  Allocated size
 *
 * @return
 *       - NULL    Allocate memory fail
 *       - Others  Allocated ok
 */
void *mem_pool_alloc(mem_pool_handle_t pool, uint32_t size);

/**
 * @brief  Check whether request memory size over pool size or not
 *
 * @param[in]  pool  Memory pool instance
 * @param[in]  size  Request memory size
 *
 * @return
 *       - false  Request size is less than pool size
 *       - true   Invalid pool or size over pool size
 */
bool mem_pool_is_over_size(mem_pool_handle_t pool, uint32_t size);

/**
 * @brief  Get pool size
 *
 * @param[in]  pool  Memory pool instance
 *
 * @return
 *Pool size
 */
uint32_t mem_pool_get_size(mem_pool_handle_t pool);

/**
 * @brief  Try allocated memory
 *
 * @param[in]  pool  Memory pool instance
 * @param[in]  size  Allocated size
 *
 * @return
 *       - NULL    Memory not enough
 *       - Others  Allocated ok
 */
void *mem_pool_try_alloc(mem_pool_handle_t pool, uint32_t size);

/**
 * @brief  Set alignment for the pool
 *
 * @note  After set if call `mem_pool_malloc_aligned` with alignment not set will use this alignment
 *
 * @param[in]  pool   Memory pool instance
 * @param[in]  align  Alignment for buffer
 */
void mem_pool_set_align(mem_pool_handle_t pool, uint16_t align);

/**
 * @brief  Realloc memory from memory pool
 *
 * @note  This API will block if find left block size less than requested
 *        Until user release the used pool, and size is enough it will return
 *
 * @param[in]  pool    Memory pool instance
 * @param[in]  buffer  Old memory pointer
 * @param[in]  size    New size
 *
 * @return
 *       - NULL    Realloc memory fail
 *       - Others  RE-allocated memory address
 */
void *mem_pool_realloc(mem_pool_handle_t pool, void *buffer, uint32_t size);

/**
 * @brief  Try realloc memory from memory pool
 *
 * @param[in]  pool    Memory pool instance
 * @param[in]  buffer  Old memory pointer
 * @param[in]  size    New size
 *
 * @return
 *       - not   NULL  Realloc ok
 *       - NULL  Realloc memory fail
 */
void *mem_pool_try_realloc(mem_pool_handle_t pool, void *buffer, uint32_t size);

/**
 * @brief  Malloc aligned buffer from memory pool
 *
 * @note  This API will block if find left block size less than requested
 *        Until user release the used pool, and size is enough it will return
 *
 * @param[in]  pool        Memory pool instance
 * @param[in]  size        Size to allocate
 * @param[in]  align_size  Alignment for the needed buffer
 *
 * @return
 *       - NULL    Allocate memory fail
 *       - Others  Malloc memory address
 */
void *mem_pool_malloc_aligned(mem_pool_handle_t pool, uint32_t size, uint16_t align_size);

/**
 * @brief  Try malloc aligned buffer from memory pool
 *
 * @param[in]  pool        Memory pool instance
 * @param[in]  size        Size to allocate
 * @param[in]  align_size  Alignment for the needed buffer
 *
 * @return
 *       - NULL    Allocate memory fail
 *       - Others  Malloc memory address
 */
void *mem_pool_try_malloc_aligned(mem_pool_handle_t pool, uint32_t size, uint16_t align_size);

/**
 * @brief  Free memory from memory pool
 *
 * @param[in]  pool    Memory pool instance
 * @param[in]  buffer  Buffer to free
 */
void mem_pool_free(mem_pool_handle_t pool, void *buffer);

/**
 * @brief  Destroy memory pool
 *
 * @param[in]  pool  Memory pool instance
 */
void mem_pool_destroy(mem_pool_handle_t pool);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
