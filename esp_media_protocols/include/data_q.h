/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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
#ifndef DATA_Q_H
#define DATA_Q_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;        /*!< Buffer for data queue */
    uint32_t buffer_size; /*!< Buffer size */
    uint32_t wp;          /*!< Buffer write pointer */
} data_q_t;

/**
 * @brief Initialize data queue
 */
int data_q_init(data_q_t *q, int size);

/**
 * @brief Get data from buffer queue
 */
uint8_t *data_q_get(data_q_t *q, int size);

/**
 * @brief Get queue data size
 */
int data_q_size(data_q_t *q);

/**
 * @brief Pop certain size of data from queue for reuse
 */
int data_q_pop(data_q_t *q, int size);

/**
 * @brief Pop all data from queue
 */
int data_q_pop_all(data_q_t *q);

/**
 * @brief Pop all data from queue
 */
void data_q_deinit(data_q_t *q);

#ifdef __cplusplus
}
#endif

#endif
