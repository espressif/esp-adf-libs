/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Extractor control mechanism for fine-tuning behavior.
 *
 *         Some controls are **common** across all extractors, while others are **extractor-specific**
 *         ### Common Controls (All Extractors)
 *         - `ESP_EXTRACTOR_CTRL_TYPE_SET_WAIT_OUTPUT`
 *         - `ESP_EXTRACTOR_CTRL_TYPE_SET_RESUME_INFO`
 *         - `ESP_EXTRACTOR_CTRL_TYPE_GET_RESUME_INFO`
 *
 *         ### Extractor-Specific Controls
 *         Other controls (e.g., indexing, seeking, ID3 parsing) depend on the extractor type
 *         Refer to the corresponding extractor's header for details
 */
typedef enum {
    ESP_EXTRACTOR_CTRL_TYPE_NONE = 0,  /*!< Invalid control type */

    ESP_EXTRACTOR_CTRL_TYPE_SET                  = 0x40,  /*!< Control type for setting */
    ESP_EXTRACTOR_CTRL_TYPE_SET_WAIT_OUTPUT      = 0x41,  /*!< `bool`:
                                                                - true   Waiting for output
                                                                - false  Not wait
                                                                Extractor common part will handle it
                                                                Each extractor can call `extractor_malloc_output_pool` */
    ESP_EXTRACTOR_CTRL_TYPE_SET_NO_ACCURATE_SEEK = 0x42,  /*!< `bool`:
                                                                - true   Use non accurate seek (considering I frame send ahead)
                                                                - false  Use accurate seek */
    ESP_EXTRACTOR_CTRL_TYPE_SET_DEEP_INDEXING    = 0x43,  /*!< `bool`:
                                                              - true   Use deep indexing, parse whole file to build indexing table
                                                              - false  No deep indexing, use estimated indexing use bitrate */
    ESP_EXTRACTOR_CTRL_TYPE_SET_DYNAMIC_INDEXING = 0x44,  /*!< `bool`:
                                                                - true   Build indexing table dynamically to save memory
                                                                - false  Full loading of indexing table */
    ESP_EXTRACTOR_CTRL_TYPE_SET_NO_INDEXING      = 0x45,  /*!< `bool`:
                                                                - true   No indexing, will not support seek
                                                                - false  Allow indexing */
    ESP_EXTRACTOR_CTRL_TYPE_SET_RESUME_INFO      = 0x46,  /*!< `esp_extractor_stream_resume_info_t`:
                                                                The struct store all information for resume playback after stop
                                                                And used for fast start up */
    ESP_EXTRACTOR_CTRL_TYPE_SET_ID3_PARSER       = 0x47,  /*!< `esp_extractor_id3_parse_cfg_t`:
                                                                Set ID3 parser so that when meet ID3 call parser to do parse
                                                                Or-else ID3 will be skipped */
    ESP_EXTRACTOR_CTRL_TYPE_SET_VERBOSE_LOG      = 0x48,  /*!< `bool`:
                                                              - true   Enable verbose log
                                                              - false  Disable verbose log */
    ESP_EXTRACTOR_CTRL_TYPE_SET_FRAME_ACROSS_PES = 0x49,  /*!< `bool`:
                                                                - true   Allow parse frame over multiple PES (specially for TS)
                                                                - false  Disable parse frame across PES (better performance) */
    ESP_EXTRACTOR_CTRL_TYPE_SET_STREAM_INFO      = 0x4a,  /*!< `esp_extractor_stream_info_t`:
                                                                Specially for raw extractor */
    ESP_EXTRACTOR_CTRL_TYPE_SET_MAX_FRAME_SIZE   = 0x4b,  /*!< `uint32_t`:
                                                                Set maximum frame size (specially for raw extractor) */

    ESP_EXTRACTOR_CTRL_TYPE_GET             = 0x80,  /*!< Control type for getting */
    ESP_EXTRACTOR_CTRL_TYPE_GET_RESUME_INFO = 0x81,  /*!< `esp_extractor_stream_resume_info_t`:
                                                           Getting resume info so that can support resume playback
                                                           When not used can call `esp_extractor_free_resume_info` to free extra data in it */
    ESP_EXTRACTOR_CTRL_TYPE_GET_ID3_BASIC   = 0x82,  /*!< `esp_extractor_id3_basic_t`:
                                                           Getting ID3 basic information then user can parse manually */
} esp_extractor_ctrl_type_t;

/**
 * @brief  Resume stream information for interrupt play
 */
typedef struct {
    esp_extractor_stream_type_t  stream_type;  /*!< Stream type */
    bool                         enable;       /*!< Stream enable or not */
    uint16_t                     stream_id;    /*!< Selected stream id */
    uint32_t                     bitrate;      /*!< Bitrate */
    uint32_t                     duration;     /*!< Stream duration (unit milliseconds) */
    uint32_t                     frame_index;  /*!< Current play frame index */
    union {
        esp_extractor_video_stream_info_t  video_info;  /*!< Video stream information */
        esp_extractor_audio_stream_info_t  audio_info;  /*!< Audio stream information */
    } stream_info;
    void     *backup_data;  /*!< Extra backup data */
    uint32_t  backup_size;  /*!< Extra backup data size */
} esp_extractor_stream_resume_info_t;

/**
 * @brief  Struct for resume information of streams
 */
typedef struct {
    esp_extractor_type_t                extractor_type;  /*!< Extractor type */
    uint32_t                            position;        /*!< Current extract byte position */
    uint32_t                            time;            /*!< Current extract time position (unit milliseconds)*/
    uint8_t                             stream_num;      /*!< Stream number */
    esp_extractor_stream_resume_info_t *resume_streams;  /*!< Kept resume stream information */
} esp_extractor_resume_info_t;

/**
 * @brief  ID3 reader callback
 *
 * @param[in]  data      Data to be read
 * @param[in]  size      Read size
 * @param[in]  skip      Whether skip this data
 * @param[in]  read_ctx  Read context
 *
 * @return
 *       - <       0     Failed to read
 *       - Others  Actual bytes being read
 */
typedef int (*id3_read_cb)(uint8_t *data, int size, bool skip, void *read_ctx);

/**
 * @brief  ID3 parse callback
 *
 * @param[in]  reader     Callback for read data
 * @param[in]  read_ctx   Read context
 * @param[in]  parse_ctx  Parse context
 *
 * @return
 *       - 0       On success
 *       - Others  Failed to parse
 */
typedef int (*id3_parse_cb)(id3_read_cb reader, void *read_ctx, void *parse_ctx);

/**
 * @brief  ID3 parse configuration
 *
 * @note  Parser need handle parse function in parse callback
 *        Read API and read context is provided in parse callback
 */
typedef struct {
    id3_parse_cb  parse_cb;   /*!< ID3 parse callback */
    void         *parse_ctx;  /*!< ID3 parse context */
} esp_extractor_id3_parse_cfg_t;

/**
 * @brief  ID3 basic information
 */
typedef struct {
    uint32_t  id3_pos;   /*!< Position of ID3 in file */
    uint32_t  id3_size;  /*!< ID3 data size */
} esp_extractor_id3_basic_t;

/**
 * @brief  Do control setting or getting for extractor
 *
 * @param[in]      extractor  Extractor handle
 * @param[in]      ctrl_type  Control type
 * @param[in,out]  ctrl       Pointer to control information
 * @param[in]      ctrl_size  Control information size
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Input is invalid
 */
esp_extractor_err_t esp_extractor_ctrl(esp_extractor_handle_t extractor, esp_extractor_ctrl_type_t ctrl_type,
                                       void *ctrl, int ctrl_size);

/**
 * @brief  Free backup resume information
 *
 * @param[in]  resume_info  Resume information
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Input is invalid
 */
esp_extractor_err_t esp_extractor_free_resume_info(esp_extractor_resume_info_t *resume_info);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
