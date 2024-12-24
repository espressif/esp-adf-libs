/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_midi_sample_cache.h"

static const char *TAG = "MIDI.SAMPLE_CACHE";

esp_midi_err_t esp_midi_sample_cache_delete_node(esp_midi_sample_cache_t **sample_cache, uint16_t num)
{
    esp_midi_sample_cache_t *node = *sample_cache;
    for (uint16_t i = 0; i < num; i++) {
        if (node) {
            esp_midi_sample_cache_t *node_next = node->next;
            if (node->data.buffer) {
                free(node->data.buffer);
            }
            free(node);
            node = node_next;
        }
    }
    return ESP_MIDI_ERR_OK;
}

esp_midi_err_t esp_midi_sample_cache_create_node(int16_t num, uint32_t buf_len, esp_midi_sample_cache_t **sample_cache)
{
    esp_midi_sample_cache_t *node_header;
    esp_midi_sample_cache_t *tmp = NULL;
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    esp_midi_sample_cache_t *node = node_header = tmp = calloc(1, sizeof(esp_midi_sample_cache_t));
    ESP_MIDI_RETURN_ON_FALSE(node, ESP_MIDI_ERR_MEM_LACK, TAG, "The node(size %d) allocation failed, line %d", sizeof(esp_midi_sample_cache_t), __LINE__);
    node->data.buffer = calloc(1, buf_len);
    if (node->data.buffer == NULL) {
        free(node);
        ESP_MIDI_LOGE(TAG, "The node(size %d) allocation failed", (int)buf_len);
        return ESP_MIDI_ERR_MEM_LACK;
    }
    node->data.len = buf_len;
    node->is_use = false;
    for (uint16_t i = 1; i < num; i++) {
        tmp = calloc(1, sizeof(esp_midi_sample_cache_t));
        ESP_MIDI_GOTO_ON_FALSE(tmp, ESP_MIDI_ERR_MEM_LACK, _node_exit, TAG, "The node(size %d) allocation failed, line %d", sizeof(esp_midi_sample_cache_t), __LINE__);
        tmp->data.buffer = calloc(1, buf_len);
        if (tmp->data.buffer == NULL) {
            free(tmp);
            ESP_MIDI_LOGE(TAG, "The node(size %d) allocation failed, line %d", (int)buf_len, __LINE__);
            ret = ESP_MIDI_ERR_MEM_LACK;
            goto _node_exit;
        }
        tmp->data.len = buf_len;
        tmp->is_use = false;
        node->next = tmp;
        node = tmp;
    }
    tmp->next = node_header;
    *sample_cache = node_header;
    return ESP_MIDI_ERR_OK;
_node_exit:
    esp_midi_sample_cache_delete_node(&node_header, num);
    return ESP_MIDI_ERR_MEM_LACK;
}

esp_midi_sample_cache_t *esp_midi_sample_cache_get_empty_data(esp_midi_sample_cache_t **sample_cache, uint32_t buf_len, uint16_t *num)
{
    esp_midi_sample_cache_t *node = *sample_cache;
    esp_midi_err_t ret = ESP_MIDI_ERR_OK;
    if (*sample_cache == NULL) {
        ret = esp_midi_sample_cache_create_node(1, buf_len, sample_cache);
        if (ret != ESP_MIDI_ERR_OK) {
            return NULL;
        }
        (*sample_cache)->is_use = true;
        return *sample_cache;
    }
    esp_midi_sample_cache_t *sample_cache_empty = NULL;
    for (uint16_t i = 0; i < *num; i++) {
        if (node->is_use == false) {
            sample_cache_empty = node;
            if (node->data.len >= buf_len) {
                node->is_use = true;
                return node;
            }
        }
        node = node->next;
    }
    if (sample_cache_empty) {
        uint8_t *buffer = calloc(1, buf_len);
        ESP_MIDI_RETURN_ON_FALSE(buffer, NULL, TAG, "The buffer(size %ld) allocation failed, line %d", buf_len, __LINE__);
        if (sample_cache_empty->data.buffer) {
            free(sample_cache_empty->data.buffer);
        }
        sample_cache_empty->data.buffer = buffer;
        sample_cache_empty->data.len = buf_len;
        sample_cache_empty->is_use = true;
        return sample_cache_empty;
    }
    sample_cache_empty = calloc(1, sizeof(esp_midi_sample_cache_t));
    ESP_MIDI_RETURN_ON_FALSE(sample_cache_empty, NULL, TAG, "The node(size %d) allocation failed, line %d", sizeof(esp_midi_sample_cache_t), __LINE__);
    sample_cache_empty->data.buffer = calloc(1, buf_len);
    if (sample_cache_empty->data.buffer == NULL) {
        free(sample_cache_empty);
        ESP_MIDI_LOGE(TAG, "The node(size %d) allocation failed, line %d", (int)buf_len, __LINE__);
        printf("num:%d\n", *num);
        return NULL;
    }
    sample_cache_empty->data.len = buf_len;
    sample_cache_empty->is_use = true;
    sample_cache_empty->next = node->next;
    node->next = sample_cache_empty;
    (*num)++;
    *sample_cache = sample_cache_empty;
    return sample_cache_empty;
}

void esp_midi_sample_cache_set_data_unuse(esp_midi_sample_cache_t *sample_cache)
{
    sample_cache->is_use = false;
}

void esp_midi_sample_cache_search_set_data_unuse(esp_midi_sample_cache_t *sample_cache, uint16_t num, esp_midi_data_t data)
{
    esp_midi_sample_cache_t *node = sample_cache;
    for (uint16_t i = 0; i < num; i++) {
        if (node) {
            if (node->data.buffer == data.buffer) {
                node->is_use = false;
            }
            node = node->next;
        }
    }
}
