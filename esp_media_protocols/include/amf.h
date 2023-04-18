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
#ifndef AMF_H
#define AMF_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "data_q.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Store 24 bits big endian value to memory
 */
#define big24(b, v)                   \
    {                                 \
        uint8_t *_b = b;              \
        uint32_t _v = v;              \
        _b[0] = (uint8_t) (_v >> 16); \
        _b[1] = (uint8_t) (_v >> 8);  \
        _b[2] = (uint8_t) _v;         \
    }

/**
 * @brief Read 24 bits big endian value from memory
 */
#define big24_v(b, v)                             \
    {                                             \
        uint8_t *_b = b;                          \
        v = (_b[0] << 16) + (_b[1] << 8) + _b[2]; \
    }

/**
 * @brief Read 16 bits big endian value from memory
 */
#define big16_v(b, v)             \
    {                             \
        uint8_t *_b = b;          \
        v = (_b[0] << 8) + _b[1]; \
    }

/**
 * @brief Read 32 bits big endian value from memory
 */
#define big32_v(b, v)                                             \
    {                                                             \
        uint8_t *_b = b;                                          \
        v = (_b[0] << 24) + (_b[1] << 16) + (_b[2] << 8) + _b[3]; \
    }

/**
 * @brief Store 16 bits big endian value to memory
 */
#define big16(b, v)                  \
    {                                \
        uint8_t *_b = b;             \
        uint16_t _v = v;             \
        _b[0] = (uint8_t) (_v >> 8); \
        _b[1] = (uint8_t) _v;        \
    }

/**
 * @brief Store 32 bits big endian value to memory
 */
#define big32(b, v)                   \
    {                                 \
        uint8_t *_b = b;              \
        uint32_t _v = v;              \
        _b[0] = (uint8_t) (_v >> 24); \
        _b[1] = (uint8_t) (_v >> 16); \
        _b[2] = (uint8_t) (_v >> 8);  \
        _b[3] = (uint8_t) _v;         \
    }

/**
 * @brief Store 24 bits little endian value to memory
 */
#define little24(b, v)                \
    {                                 \
        uint8_t *_b = b;              \
        uint32_t _v = v;              \
        _b[0] = (uint8_t) (_v);       \
        _b[1] = (uint8_t) (_v >> 8);  \
        _b[2] = (uint8_t) (_v >> 16); \
    }

/**
 * @brief Read 32 bits little endian value from memory
 */
#define little32(b, v)      \
    {                       \
        int *_b = (int*) b; \
        uint32_t _v = v;    \
        *((int*) _b) = _v;  \
    }

/**
 * @brief Read 24 bits little endian value from memory
 */
#define little24_v(b, v)                  \
    {                                     \
        uint8_t *_b = b;                  \
        v = (*(uint32_t*) _b) & 0xFFFFFF; \
    }

/**
 * @brief Read 32 bits little endian value from memory
 */
#define little32_v(b, v)       \
    {                          \
        uint8_t *_b = b;       \
        v = (*(uint32_t*) _b); \
    }

typedef int (*amf_obj_cb)(data_q_t *q, void *ctx);

/**
 * @brief AMF data type
 */
typedef enum {
    AMF_DATA_TYPE_NONE,
    AMF_DATA_TYPE_STR,
    AMF_DATA_TYPE_NUM,
    AMF_DATA_TYPE_ARR,
    AMF_DATA_TYPE_OBJ,
    AMF_DATA_TYPE_BOOL,
    AMF_DATA_TYPE_NULL,
} amf_data_type_t;

/**
 * @brief AMF data list after parsed
 */
typedef struct _amf_data_list_t {
    amf_data_type_t          type;
    void                    *body;
    struct _amf_data_list_t *next;
} amf_data_list_t;

/**
 * @brief AMF string structure
 */
typedef struct {
    char *s;
    int   len;
} amf_str_t;

/**
 * @brief AMF object structure
 */
typedef struct {
    amf_data_list_t *arr;
} amf_obj_t;

/**
 * @brief AMF array structure
 */
typedef struct {
    amf_data_list_t *arr;
    int              n;
} amf_arr_t;

/**
 * @brief Callback to iterate AMF object
 */
typedef void (*amf_obj_walker_cb)(amf_data_list_t *k, amf_data_list_t *v, void *ctx);

/**
 * @brief Callback to iterate AMF array
 */
typedef void (*amf_arr_walker_cb)(amf_data_list_t *k, amf_data_list_t *v, void *ctx);

/**
 * @brief        Add string to AMF storage
 *
 * @param         q: Pointer to data queue
 * @param         s:  String to add
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_string(data_q_t *q, char *s);

/**
 * @brief        Add key string to AMF storage
 *               Key string is a special type for AMF object and array
 * @param         q: Pointer to data queue
 * @param         s:  String to add
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_key_string(data_q_t *q, char *s);

/**
 * @brief        Add boolean to AMF storage
 * @param         q: Pointer to data queue
 * @param         b:  Boolean value to add
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_bool(data_q_t *q, bool v);

/**
 * @brief        Add object to AMF storage
 * @param         q: Pointer to data queue
 * @param         obj_cb:  Callback to fill AMF objece key and value
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_obj(data_q_t *q, amf_obj_cb obj_cb, void *ctx);

/**
 * @brief        Add array to AMF storage
 * @param         q: Pointer to data queue
 * @param         obj_cb:  Callback to fill AMF objece key and value
 * @param         n:  AMF array item number
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_array(data_q_t *q, amf_obj_cb obj_cb, int n, void *ctx);

/**
 * @brief        Add double number to AMF storage
 * @param         q: Pointer to data queue
 * @param         f:  Double value to fill
 * @return        - 0: On success
 *                - -1: Buffer not enough
 */
int amf_add_number(data_q_t *q, double f);

/**
 * @brief        Dump AMF data and print it out
 * @param         d:  Data pointer
 * @param         size:  Data size
 */
void dump_amf(uint8_t *d, int size);

/**
 * @brief         Parse data into AMF data structure
 *                For string it is refer to origin data,
 *                You must not release the origin data util AMF data structure not used
 * @param         d:  Data pointer
 * @param         len:  Data size
 * @return        - NULL: Fail to parse
 *                - Others: AMF data list
 */
amf_data_list_t *amf_parse_data(uint8_t *d, int len);

/**
 * @brief         Free parser for AMF
 * @param         list: AMF data list
 */
void amf_free_parse(amf_data_list_t *list);

/**
 * @brief         Get AMF data list by index
 * @param         list: AMF data list
 * @param         idx: Index start from 0
 * @return        - NULL: No matched index
 *                - Others: AMF data list
 */
amf_data_list_t *amf_get_by_idx(amf_data_list_t *list, int idx);

/**
 * @brief         Get value from object by key
 * @param         list: AMF data list
 * @param         expect: Expected key
 * @return        - NULL: No matched key
 *                - Others: AMF data list
 *                
 */
amf_data_list_t *amf_get_obj_by_key(amf_data_list_t *list, char *expect);

/**
 * @brief         Iterate on AMF object
 * @param         list: AMF data list
 * @param         cb: Callback for object keys and values
 * @param         ctx: Input Context
 * @return        - 0: On success
 *                - -1: Input list is not object
 */
int amf_walker_obj(amf_data_list_t *list, amf_obj_walker_cb cb, void *ctx);

/**
 * @brief         Iterate on AMF array
 * @param         list: AMF data list
 * @param         cb: Callback for array keys and values
 * @param         ctx: Input Context
 * @return        - 0: On success
 *                - -1: Input list ls not array
 */
int amf_walker_arr(amf_data_list_t *list, amf_arr_walker_cb cb, void *ctx);

#ifdef __cplusplus
}
#endif

#endif
