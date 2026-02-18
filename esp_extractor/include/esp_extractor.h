/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Extract mask
 */
#define ESP_EXTRACT_MASK_AUDIO  (1 << 0)                                           /*!< Filter audio data only */
#define ESP_EXTRACT_MASK_VIDEO  (1 << 1)                                           /*!< Filter video data only */
#define ESP_EXTRACT_MASK_AV     (ESP_EXTRACT_MASK_AUDIO | ESP_EXTRACT_MASK_VIDEO)  /*!< Filter both audio and video */

/**
 * @brief  Extractor error code
 */
typedef enum {
    ESP_EXTRACTOR_ERR_NEED_MORE_BUF  = 1,   /*!< Parsing need more buffer, read more and retry */
    ESP_EXTRACTOR_ERR_ALREADY_EXIST  = 2,   /*!< Parsing when old frame not all consumed */
    ESP_EXTRACTOR_ERR_STREAM_CHANGED = 3,   /*!< Stream changed need update streaming info accordingly */
    ESP_EXTRACTOR_ERR_EOS            = 4,   /*!< Reach end of stream */
    ESP_EXTRACTOR_ERR_WAITING_OUTPUT = 5,   /*!< Output is not enough, user need sleep some time and retry */
    ESP_EXTRACTOR_ERR_SKIPPED        = 6,   /*!< Frame size over pool size, this frame is skipped */
    ESP_EXTRACTOR_ERR_OK             = 0,   /*!< No error */
    ESP_EXTRACTOR_ERR_FAIL           = -1,  /*!< General fail */
    ESP_EXTRACTOR_ERR_INV_ARG        = -2,  /*!< Invalid argument */
    ESP_EXTRACTOR_ERR_NOT_FOUND      = -3,  /*!< Not found (no such track, stream or data etc) */
    ESP_EXTRACTOR_ERR_NO_MEM         = -4,  /*!< Not enough memory */
    ESP_EXTRACTOR_ERR_READ           = -5,  /*!< Fail to read data from input */
    ESP_EXTRACTOR_ERR_NOT_SUPPORTED  = -6,  /*!< Not supported (not supported for control, setting etc) */
    ESP_EXTRACTOR_ERR_WRONG_HEADER   = -7,  /*!< Wrong format header */
    ESP_EXTRACTOR_ERR_ABORTED        = -8,  /*!< Aborted by IO */
} esp_extractor_err_t;

/**
 * @brief  Extractor container type
 */
typedef enum {
    ESP_EXTRACTOR_TYPE_NONE  = 0,                                  /*!< Invalid extractor type */
    ESP_EXTRACTOR_TYPE_WAV   = EXTRACTOR_4CC('W', 'A', 'V', ' '),  /*!< WAV extractor type */
    ESP_EXTRACTOR_TYPE_MP4   = EXTRACTOR_4CC('M', 'P', '4', ' '),  /*!< MP4 extractor type */
    ESP_EXTRACTOR_TYPE_TS    = EXTRACTOR_4CC('T', 'S', ' ', ' '),  /*!< TS extractor type */
    ESP_EXTRACTOR_TYPE_OGG   = EXTRACTOR_4CC('O', 'G', 'G', ' '),  /*!< OGG extractor type */
    ESP_EXTRACTOR_TYPE_AVI   = EXTRACTOR_4CC('A', 'V', 'I', ' '),  /*!< AVI extractor type */
    ESP_EXTRACTOR_TYPE_FLV   = EXTRACTOR_4CC('F', 'L', 'V', ' '),  /*!< FLV extractor type */
    ESP_EXTRACTOR_TYPE_CAF   = EXTRACTOR_4CC('C', 'A', 'F', ' '),  /*!< CAF extractor type */
    ESP_EXTRACTOR_TYPE_MP3   = EXTRACTOR_4CC('M', 'P', '3', ' '),  /*!< MP3 extractor type */
    ESP_EXTRACTOR_TYPE_AAC   = EXTRACTOR_4CC('A', 'A', 'C', ' '),  /*!< AAC extractor type */
    ESP_EXTRACTOR_TYPE_FLAC  = EXTRACTOR_4CC('F', 'L', 'A', 'C'),  /*!< FLAC extractor type */
    ESP_EXTRACTOR_TYPE_AMRNB = EXTRACTOR_4CC('A', 'M', 'R', 'N'),  /*!< AMR-NB extractor type */
    ESP_EXTRACTOR_TYPE_AMRWB = EXTRACTOR_4CC('A', 'M', 'R', 'W'),  /*!< AMR-WB extractor type */
    ESP_EXTRACTOR_TYPE_RAW   = EXTRACTOR_4CC('R', 'A', 'W', ' '),  /*!< Raw extractor type */
} esp_extractor_type_t;

/**
 * @brief  Extractor handle
 */
typedef void *esp_extractor_handle_t;

/**
 * @brief  Function groups to get input data
 */

/**
 * @brief  Read callback for input data
 *
 * @param[in]  buffer  Buffer to be read
 * @param[in]  size    Size to read
 * @param[in]  ctx     Input context
 *
 * @return
 *       - <       0     Error
 *       - Others  Actual read size
 */
typedef int (*_extractor_read_func)(void *buffer, uint32_t size, void *ctx);

/**
 * @brief  Seek callback for input data
 *
 * @param[in]  position  Position to seek
 * @param[in]  ctx       Input context
 *
 * @return
 *       - <  0  Error
 *       - 0  On Success
 */
typedef int (*_extractor_seek_func)(uint32_t position, void *ctx);

/**
 * @brief  Get total size callback for input
 *
 * @param[in]  ctx  Input context
 *
 * @return
 *       - Total  size of input
 */
typedef uint32_t (*_extractor_total_size_func)(void *ctx);

/**
 * @brief  Configuration of extractor
 *
 * @note  Extractor configuration is consisted by 3 parts
 *         - Extractor filter control
 *           `type`:
 *              optional and set when use know extractor type and use it as favorite type
 *              Internally still do probe for such type but improve efficiency if probe OK directly
 *            `extract_mask`:
 *              Used to control filter behavior, if file contain both audio and video
 *              But use only want to playback audio part, can set this mask to save memory an improve parse efficiency
 *         - Input configuration
 *            It consisted by some callback so that extractor can read input data and do fast seek
 *            If `in_size_cb` is set, can get file size to estimate duration or as read limit
 *         - Output configuration
 *            Extractor frame output will put into memory pool so that use can use it without copy
 *            Some decode may need input frame to be aligned (aligned to cached size)
 *            Both pool size and alignment is added to meet special cares
 */
typedef struct {
    esp_extractor_type_t        type;           /*!< Favorite type for extractor (optional) */
    uint8_t                     extract_mask;   /*!< Extract mask: video, audio or both */
    _extractor_read_func        in_read_cb;     /*!< Input read callback (required) */
    _extractor_seek_func        in_seek_cb;     /*!< Input seek callback (optional)
                                                     Will read data until destination position if not provided */
    _extractor_total_size_func  in_size_cb;     /*!< Input get file size callback (optional) */
    void                       *in_ctx;         /*!< Input context (should not NULL) */
    uint32_t                    out_pool_size;  /*!< Output pool size */
    uint16_t                    out_align;      /*!< Alignment for output */
} esp_extractor_config_t;

/**
 * @brief  Open extractor
 *
 * @note  Just allocate memory for cache and output buffer
 *
 * @param[in]   config     Extractor configuration
 * @param[out]  extractor  Output extractor handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 *       - ESP_EXTRACTOR_ERR_NO_MEM   Not enough memory
 */
esp_extractor_err_t esp_extractor_open(esp_extractor_config_t *config, esp_extractor_handle_t *extractor);

/**
 * @brief  Parse stream information
 *
 * @note  It may takes long time for network case, can be canceled by `esp_extractor_close`
 *
 * @param[in]  extractor  Extractor Handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 *       - Others                     Failed to parse
 */
esp_extractor_err_t esp_extractor_parse_stream(esp_extractor_handle_t extractor);

/**
 * @brief  Get total stream number of certain stream type
 *
 * @param[in]   extractor    Extractor Handle
 * @param[in]   stream_type  Stream type
 * @param[out]  stream_num   Stream number of the stream type to store
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG    Invalid input arguments
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Not found track of such stream type
 */
esp_extractor_err_t esp_extractor_get_stream_num(esp_extractor_handle_t extractor, esp_extractor_stream_type_t stream_type,
                                                 uint16_t *stream_num);

/**
 * @brief  Get stream information
 *
 * @note  Stream information keeps some internal information to decrease the memory usage
 *        Never try to modify it, internal information will be released after 'esp_extractor_close'.
 *        Don't rely on it once 'esp_extractor_close' called
 *
 * @param[in]   extractor    Extractor Handle
 * @param[in]   stream_type  Stream type
 * @param[in]   stream_idx   Stream index
 * @param[out]  info         Stream information to store
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 *       - Others                     Failed to get stream information
 */
esp_extractor_err_t esp_extractor_get_stream_info(esp_extractor_handle_t extractor, esp_extractor_stream_type_t stream_type,
                                                  uint16_t stream_idx, esp_extractor_stream_info_t *info);

/**
 * @brief  Enable stream
 *
 * @note  This API no need to be called in most of cases
 *        Extractor will auto enable first stream for each stream type after parse stream finished
 *        It can be used in following scenario:
 *          - Disable stream un-wanted by auto enable logic
 *          - Change audio track or video track, disable one and enable another afterwards
 *          - No want to filter certain stream during play
 *
 * @param[in]  extractor    Extractor Handle
 * @param[in]  stream_type  Stream type
 * @param[in]  stream_idx   Stream index
 * @param[in]  enable       Whether enable such stream or not (true: enable, false: disable)
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 *       - Others                     Failed to enable stream
 */
esp_extractor_err_t esp_extractor_enable_stream(esp_extractor_handle_t extractor, esp_extractor_stream_type_t stream_type,
                                                uint16_t stream_idx, bool enable);

/**
 * @brief  Do extract stream data operation, output one frame after each call
 *
 * @note  `frame_buffer` in `frame_info` is allocated in memory pool
 *         Users need call `esp_extractor_release_frame` to free when not used anymore
 *
 * @param[in]   extractor   Extractor Handle
 * @param[out]  frame_info  Output frame information
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK              On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG         Invalid input arguments
 *       - ESP_EXTRACTOR_ERR_EOS             Reach file end, no need to read again
 *       - ESP_EXTRACTOR_ERR_WAITING_OUTPUT  Output buffer is not enough, need wait for consumed and try again
 *       - Others                            Failed to extract data
 */
esp_extractor_err_t esp_extractor_read_frame(esp_extractor_handle_t extractor, esp_extractor_frame_info_t *frame_info);

/**
 * @brief  Release the pool memory of the frame
 *
 * @param[in]  extractor   Extractor Handle
 * @param[in]  frame_info  Output frame information
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 *       - Others                     Failed to release
 */
esp_extractor_err_t esp_extractor_release_frame(esp_extractor_handle_t extractor, esp_extractor_frame_info_t *frame_info);

/**
 * @brief  Extractor seek operation
 *
 * @param[in]  extractor  Extractor Handle
 * @param[in]  time_pos   Time position to seek (unit millisecond)
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK             On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG        Invalid input arguments
 *       - ESP_EXTRACTOR_ERR_NOT_SUPPORTED  Not supported to do seek operation
 *       - ESP_EXTRACTOR_ERR_EOS            Reach end of stream
 */
esp_extractor_err_t esp_extractor_seek(esp_extractor_handle_t extractor, uint32_t time_pos);

/**
 * @brief  Get extractor type
 *
 * @param[in]   extractor  Extractor Handle
 * @param[out]  type       Extractor type to store
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid input arguments
 */
esp_extractor_err_t esp_extractor_get_extractor_type(esp_extractor_handle_t extractor, esp_extractor_type_t *type);

/**
 * @brief  Close extractor
 *
 * @param[in]  extractor  Extractor Handle
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Extractor handle not valid
 */
esp_extractor_err_t esp_extractor_close(esp_extractor_handle_t extractor);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
