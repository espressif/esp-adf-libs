/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "esp_midi_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Structure definition for a sample cache node. It is circular linked list
 *         This structure represents a node in the sample cache, containing a pointer to the next node,
 *         `esp_midi_data_t` data, and a flag indicating whether the node is in use.
 */
typedef struct _sample_cache_t {
    struct _sample_cache_t  *next;    /*!< Pointer to the next node in the list */
    esp_midi_data_t          data;    /*!< Data stored in this node */
    bool                     is_use;  /*!< Flag indicating if the node is currently in use. Flag indicating whether the node is in use (true) or not (false) */
} esp_midi_sample_cache_t;

/**
 * @brief  Creates a new sample cache node
 *         This function allocates memory and initializes a new sample cache node.
 *         It takes the number of nodes, the buffer length for each node's data, and a pointer to a pointer to receive the newly created node.
 *         The function returns an error code indicating the success or failure of the operation
 *
 * @param  num           The number of nodes to create
 * @param  buf_len       The buffer length for each node's data
 * @param  sample_cache  Pointer to a pointer to receive the newly created node
 *
 * @return
 *       - ESP_MIDI_ERR_OK        Succeeded
 *       - ESP_MIDI_ERR_MEM_LACK  Insufficient memory
 */
esp_midi_err_t esp_midi_sample_cache_create_node(int16_t num, uint32_t buf_len, esp_midi_sample_cache_t **sample_cache);

/**
 * @brief  Retrieves an empty data buffer from a sample cache node
 *         This function searches the sample cache for an unused node and returns a pointer to its data field
 *         (assumed to already contain a sufficiently large data buffer). If an unused node is found, it returns the pointer to the node's data.
 *         If no unused node is found, it may return NULL
 *
 * @param  sample_cache  Pointer to the sample cache
 * @param  buf_len       The requested data buffer size
 * @param  num           Pointer to receive the number of node in `sample_cache`
 * @return
 *       - >0    Returns a pointer to the data field of the found node
 *       - NULL  Failed
 */
esp_midi_sample_cache_t *esp_midi_sample_cache_get_empty_data(esp_midi_sample_cache_t **sample_cache, uint32_t buf_len, uint16_t *num);

/**
 * @brief  Sets the MIDI sample cache data as unused.
 *         This function marks the data associated with the specified MIDI sample cache
 *         as not currently in use. This can be useful for memory management purposes,
 *         indicating that the sample data can be safely overwritten or freed if needed
 *
 * @note  This function does not actually free the memory occupied by the sample data;
 *        it merely sets a flag indicating that the data is not currently being used
 *
 * @param  sample_cache  Pointer to the MIDI sample cache whose data should be marked as unused
 */
void esp_midi_sample_cache_set_data_unuse(esp_midi_sample_cache_t *sample_cache);

/**
 * @brief  Sets the specified MIDI sample cache entry's data to unused state.
 *         This function marks the data of a particular MIDI sample cache entry as unused.
 *         It is useful when you want to invalidate or clear the data associated with a
 *         specific sample in the cache, without removing the entry itself from the cache.
 *
 * @param  sample_cache  Pointer to the MIDI sample cache structure.
 * @param  num           The index of the sample cache entry to be marked as unused.
 * @param  data          A placeholder parameter for consistency with other functions,
 *                       but not used in this specific function. It allows for a uniform
 *                       function signature across related operations.
 *
 */
void esp_midi_sample_cache_search_set_data_unuse(esp_midi_sample_cache_t *sample_cache, uint16_t num, esp_midi_data_t data);

/**
 * @brief  Deletes a node from the sample cache
 *         This function deletes a node from the  sample cache and frees the resources it occupies
 *         It takes a pointer to a pointer to the sample cache and the index of the node to delete.
 *         The function returns an error code indicating the success or failure of the operation
 *
 * @param  sample_cache  Pointer to a pointer to the MIDI sample cache
 * @param  num           Pointer to receive the number of node in `sample_cache`
 *
 * @return
 *       - ESP_MIDI_ERR_OK  Succeeded
 */
esp_midi_err_t esp_midi_sample_cache_delete_node(esp_midi_sample_cache_t **sample_cache, uint16_t num);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
