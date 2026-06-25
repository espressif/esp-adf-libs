/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "esp_extractor.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void *esp_extractor_id3_parser_hd_t;

/**
 * @brief  ID3 text encoding type
 */
typedef enum {
    ESP_EXTRACTOR_ID3_TEXT_ENCODING_ISO_8859_1 = 0,     /*!< ISO-8859-1 */
    ESP_EXTRACTOR_ID3_TEXT_ENCODING_UTF_16     = 1,     /*!< UTF-16 with BOM */
    ESP_EXTRACTOR_ID3_TEXT_ENCODING_UTF_16BE   = 2,     /*!< UTF-16BE without BOM */
    ESP_EXTRACTOR_ID3_TEXT_ENCODING_UTF_8      = 3,     /*!< UTF-8 */
    ESP_EXTRACTOR_ID3_TEXT_ENCODING_NONE       = 0xFF,  /*!< Not set */
} esp_extractor_id3_text_encoding_t;

/**
 * @brief  ID3 key value pair
 */
typedef struct {
    char    *key;       /*!< Frame key */
    char    *value;     /*!< Frame value */
    uint8_t  encoding;  /*!< Text encoding type */
} esp_extractor_id3_kv_t;

/**
 * @brief  Common ID3 information
 *
 * @note  All memory is owned by parser handle and valid until
 *        `esp_extractor_id3_parser_close` is called.
 */
typedef struct {
    char                   *title;       /*!< Title */
    char                   *author;      /*!< Author/artist */
    char                   *album;       /*!< Album */
    char                   *date;        /*!< Date/year */
    char                   *genre;       /*!< Genre */
    char                   *cover_mime;  /*!< Cover MIME type */
    uint8_t                *cover;       /*!< Cover data */
    uint32_t                cover_size;  /*!< Cover data size */
    uint8_t                 encoding;    /*!< Text encoding type for common fields */
    esp_extractor_id3_kv_t *extra;       /*!< Extra string fields */
    uint16_t                extra_num;   /*!< Number of extra fields */
} esp_extractor_id3_info_t;

/**
 * @brief  Plug ID3 parser into extractor
 *
 * @param[in]   extractor  Extractor handle
 * @param[out]  handle     ID3 parser handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid argument
 *       - ESP_EXTRACTOR_ERR_NO_MEM   Not enough memory
 *       - Others                     Failed to set parser to extractor
 */
esp_extractor_err_t esp_extractor_id3_parser_open(esp_extractor_handle_t extractor,
                                                  esp_extractor_id3_parser_hd_t *handle);

/**
 * @brief  Get parsed ID3 information
 *
 * @param[in]   handle  ID3 parser handle
 * @param[out]  info    ID3 information pointer
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG    Invalid argument
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  No ID3 information parsed yet
 */
esp_extractor_err_t esp_extractor_id3_parser_get_info(esp_extractor_id3_parser_hd_t handle,
                                                      const esp_extractor_id3_info_t **info);

/**
 * @brief  Close ID3 parser and free all parsed information
 *
 * @param[in]  handle  ID3 parser handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid argument
 */
esp_extractor_err_t esp_extractor_id3_parser_close(esp_extractor_id3_parser_hd_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
