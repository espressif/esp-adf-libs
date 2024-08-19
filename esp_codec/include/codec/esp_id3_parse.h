// Copyright 2024 Espressif Systems (Shanghai) CO., LTD.
// All rights reserved.

#pragma once

/*
Identification 3(ID3) is a metadata format used to store additional information about audio files,
such as song name,artists,albums,etc. ID3 tags are often embedded in MP3 files so that music players 
or other software can read and display this information. There are several ID3 tag versions,including
Id3v1 and ID3v2, which differ in terms of storage capacity, tag support, and functionality.
ID3V2 is basically divided into two parts, one is the ID3 header, the other is the data. The data format
uses frames to represent different types of information, which are structured within a tag in a binary format.
They are typically placed at the beginning or end of an audio files.
Curently, this parser supports v2.3 and v2.4 parsing. General information are grouped by `esp_id3_frame_t`, 
the frame.data is encoded by frame.enc_type. And the frame.data size is frame.data_size.If user want to get genres,
please convert frame.data of genres from frame.enc_type to integer. And then get genres by `esp_id3_get_genres_str`. 
The picture frame is different from other frames. It has four properties, `mime_type` is picture format, 
`pic_des` is description of picture, `picture_data` is stored picture data,`picture_data_size` is size of the `picture_data`.
Usage:
step 1: Get ID3 information by `esp_id3_parse`.
step 2: Show ID3 information.
step 3: Free ID3 information by `esp_id3_free`.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
/** @breif error number */
typedef enum {
    ESP_ID3_ERR_OK          =  0, /*!< Sucessed*/
    ESP_ID3_ERR_FAIL        = -1, /*!< Fail */
    ESP_ID3_ERR_NO_TAG      = -2, /*!< No ID3 tag*/
    ESP_ID3_ERR_NO_DATA     = -3, /*!< Input data is not enough*/
    ESP_ID3_ERR_NO_MEM      = -4, /*!< Memory allocation failed*/
    ESP_ID3_ERR_VERSION     = -5, /*!< ID3 version is in error range */
    ESP_ID3_ERR_NOT_SUPPORT = -6, /*!< ID3 version is un-support*/
} esp_id3_err_t;

/** @breif picture format*/
typedef enum {
    ID3_MIME_TAG_GIF   = 0, /*!< GIF picture */
    ID3_MIME_TAG_MJPEG = 1, /*!< MJPEG picture */
    ID3_MIME_TAG_PNG   = 2, /*!< PNG picture */
    ID3_MIME_TAG_TIFF  = 3, /*!< TIFF picture */
    ID3_MIME_TAG_BMP   = 4, /*!< BMP picture */
    ID3_MIME_TAG_NONE  = 5, /*!< No picture */
} id3v2_mime_tags_t;

/** @breif charset*/
typedef enum {
    esp_id3_enc_iso_8859_1 = 0, /*!< Charset:ISO-8859-1*/
    esp_id3_enc_utf16_bom  = 1, /*!< UTF-16 BOM */
    esp_id3_enc_utf16_be   = 2, /*!< UTF-16 BE */
    esp_id3_enc_utf8       = 3, /*!< UTF-8 */
} esp_id3_encoding_t;

/** @breif frame information*/
typedef struct {
    esp_id3_encoding_t enc_type; /*!< Frame data charset */
    uint8_t*           data;     /*!< Frame data*/
    uint32_t           data_size;/*!< Frame data size*/
} esp_id3_frame_t;

/** @breif ID3 information*/
typedef struct {
    uint32_t          id3_size;          /*!< ID3 size */
    uint8_t           id3_version;       /*!< ID3 version */
    uint8_t           id3_flags;         /*!< ID3 flag */
    uint8_t*          extend_header;     /*!< ID3 extend header */
    uint32_t          extend_header_size;/*!< ID3 extend header size */
    esp_id3_frame_t   year;              /*!< Year */
    esp_id3_frame_t   date;              /*!< Data */
    esp_id3_frame_t   time;              /*!< Time */
    esp_id3_frame_t   title;             /*!< Title */
    esp_id3_frame_t   composer;          /*!< Composer */
    esp_id3_frame_t   genres;            /*!< Genres */
    esp_id3_frame_t   comment;           /*!< Comment */
    id3v2_mime_tags_t mime_type;         /*!< Picture format */
    uint8_t*          pic_des;           /*!< Picture description */
    uint8_t*          picture_data;      /*!< Picture data */
    uint32_t          picture_data_size; /*!< Picture data size */
    esp_id3_frame_t   performer;         /*!< Performer */
    esp_id3_frame_t   track;             /*!< Track */
    esp_id3_frame_t   album;             /*!< Album */
} esp_id3_info_t;

/** @breif ID3 configure*/
typedef struct
{
    int     (*read)(uint8_t* buffer, uint32_t length, void* ctx); /*!< The read callback */
    void*   ctx;                                                  /*!< The read function handle */
    int32_t max_tolerate;                                         /*!< The maximum tolerated error bytes before ID3.*/
    bool    no_parse;                                             /*!< True: not need parse ID3. false: parse ID3 */
} esp_id3_parse_cfg_t;

/**
 * @brief          Parse ID3 version 2
 * 
 * @note  If `*id3_info != NULL`, please free the `id3_info` by call `esp_id3_free` whatever the return value 
 *
 * @param[in]       cfg      ID3 configuration
 * @param[out]      id3_info ID3 information
 *
 * @return     
 *       - ESP_ID3_ERR_OK  Sucessed
 *       - Others          Failed
 */
esp_id3_err_t esp_id3_parse(esp_id3_parse_cfg_t* cfg, esp_id3_info_t** id3_info);

/**
 * @brief          Get genre string by index. 
 *                 The index is from after decoding`id3_info->genres.data` by `id3_info->genres.enc_type`.
 *                 If genred data is 'CR', the index is 256.
 *                 If genred data is 'RX', the index is 257.
 *                 If index greater than 257, it will return NULL.
 *                 If index greater than 191 and less than 255, it will return NULL.
 *
 * @param[in]      index The index of genre from ID3 information
 *
 * @return
 *       - genre string
 *
*/
const char* esp_id3_get_genres_str(int16_t index);

/**
 * @brief          Free ID3
 *
 * @param[in]      id3_info ID3 information
 *
 */
void esp_id3_free(esp_id3_info_t** id3_info);

#ifdef __cplusplus
}
#endif