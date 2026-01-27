/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_extractor.h"
#include "esp_extractor_ctrl.h"
#include "data_cache.h"
#include "mem_pool.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define EXTRACTOR_FRAME_FLAG_NEED_PARSING  (1 << 4)
#define EXTRACTOR_NEED_PARSING(flag)       ((flag & EXTRACTOR_FRAME_FLAG_NEED_PARSING) > 0)

/**
 * @brief  Extractor parent struct
 *
 * @note  Common resource can be accessed from base directly
 *        To keep specified information, should store in `extractor_inst`
 *        And free related resource when close
 */
typedef struct {
    esp_extractor_type_t  type;            /*!< Extractor type */
    uint32_t              sub_type;        /*!< Sub extractor type */
    data_cache_t         *cache;           /*!< Data cache to read input data */
    mem_pool_handle_t     output_pool;     /*!< Output pool for output frame data */
    uint8_t               extract_mask;    /*!< Extractor mask */
    void                 *extractor_inst;  /*!< Extractor instance */
} extractor_t;

/**
 * @brief  Extractor control lists
 *
 * @note  Add user control before open will be buffered and setting once when open
 */
typedef struct _extractor_ctrl_list {
    esp_extractor_ctrl_type_t    ctrl_type;  /*!< Control type */
    void                        *ctrl;       /*!< Controlling data */
    int                          ctrl_size;  /*!< Control size */
    struct _extractor_ctrl_list *next;       /*!< Next control */
} extractor_ctrl_list_t;

/**
 * @brief  Probe function to get sub extractor type
 *
 * @note  Probe is used to check whether input is such extractor type or not
 *        Some container like AUDIO_ES may contain multiple sub-type like `MP3`, `AAC`
 *        When probe detect the sub-type, it can set so that in real parse can find matched parser to do frame parse action
 *
 * @param[in]   buffer    Buffer to be probed
 * @param[in]   size      Buffer size
 * @param[out]  sub_type  Sub type to be filled
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK             Probed ok
 *       - ESP_EXTRACTOR_ERR_NEED_MORE_BUF  Probed buffer size not enough need buffer more and retry
 *       - Others                           Failed to probe, no need probe again
 */
typedef esp_extractor_err_t (*_extractor_probe_func)(uint8_t *buffer, uint32_t size, uint32_t *sub_type);

/**
 * @brief  Extractor open function
 *
 * @note  Open function is used to parse stream in sync
 *        It should return immediately when read from input fail so that not block long time
 *        Some special setting will be set through control lists `ctrls`
 *        Each extractor need add its support control type in its header file for easy understanding
 *
 * @param[in]  extractor  Parent of extractor
 * @param[in]  ctrls      List of control settings
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK  On success
 *       - Others                Failed to open
 */
typedef esp_extractor_err_t (*_extractor_open)(extractor_t *extractor, extractor_ctrl_list_t *ctrls);

/**
 * @brief  Extractor control function
 *
 * @note  This function is optional for extractor
 *        If defined and meet unsupported settings, need return `ESP_EXTRACTOR_ERR_NOT_SUPPORTED`
 *
 * @param[in]      extractor  Parent of extractor
 * @param[in]      ctrl_type  Control type
 * @param[in,out]  ctrl       Control to setting
 * @param[in,out]  ctrl_size  Control size
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK             On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG        Invalid control setting
 *       - ESP_EXTRACTOR_ERR_NOT_SUPPORTED  Not supported control type
 *       - Others                           Failed to open
 */
typedef esp_extractor_err_t (*_extractor_extra_ctrl)(extractor_t *extractor, esp_extractor_ctrl_type_t ctrl_type,
                                                     void *ctrl, int ctrl_size);
/**
 * @brief  Get stream number function
 *
 * @param[in]   extractor    Parent of extractor
 * @param[in]   stream_type  Stream type to get number for
 * @param[out]  stream_num   Stream number to store
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG    Invalid argument
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  No such stream
 *       - Others                       Failed to get stream number
 */
typedef esp_extractor_err_t (*_extractor_get_stream_num)(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                         uint16_t *stream_num);

/**
 * @brief  Get stream number information function
 *
 * @param[in]   extractor     Parent of extractor
 * @param[in]   stream_type   Stream type to get information for
 * @param[in]   stream_index  Stream index for the stream type
 * @param[out]  stream_info   Stream information to store
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG    Invalid argument
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  No such stream
 *       - Others                       Failed to get stream information
 */
typedef esp_extractor_err_t (*_extractor_get_stream_info)(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                          uint16_t stream_index, esp_extractor_stream_info_t *stream_info);

/**
 * @brief  Enable stream function
 *
 * @note  This API is used to enable on or disable stream output of specified stream
 *        It allows to set during parse, behaviors like change audio
 *        `stream_idx` is in [0, stream_num) `stream_num` got from `_extractor_get_stream_num`
 *        Extractor common part will try to auto enable first stream of audio or video
 *        User need manually call this API to disable it
 *
 * @param[in]  extractor     Parent of extractor
 * @param[in]  stream_type   Stream type to be enabled or disabled
 * @param[in]  stream_index  Stream index for the stream type
 * @param[in]  enable        Enable or disable stream (true: enable, false: disable)
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid argument
 *       - Others                     Failed to enable stream
 */
typedef esp_extractor_err_t (*_extractor_enable_stream)(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                        uint16_t stream_index, bool enable);

/**
 * @brief  Read frame function
 *
 * @note  This API output stream frame (audio, video) in interleave mode
 *        It needs use `data_cache` to read input data parse it and output into `output_pool`
 *        In special case, output maybe not frame boundary, so need deep parse
 *        Extractor common part have deep parse logic, set following flag into `frame_flag` will enter deep parse logic
 *          - EXTRACTOR_FRAME_FLAG_NEED_PARSING
 *        Extractor common part will also control output buffer not enough cases
 *        Sub extractor need use `extractor_malloc_output_pool` to allocate output and do following check:
 *          - Buffer is not enough but not over pool size
 *            It needs seek to last frame end position during parsing or cache data internally
 *          - Buffer is not enough and frame over pool size
 *            It needs drop this frame or silent skipped it
 *
 * @param[in]   extractor   Parent of extractor
 * @param[out]  frame_info  Frame information to be filled
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK              On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG         Invalid argument
 *       - ESP_EXTRACTOR_ERR_WAITING_OUTPUT  Output buffer not enough need wait and retry
 *       - ESP_EXTRACTOR_ERR_SKIPPED         Frame over buffer size skipped this frame
 *       - ESP_EXTRACTOR_ERR_EOS             Reach input end
 *       - Others                            Failed to parse and read frame
 *
 */
typedef esp_extractor_err_t (*_extractor_read_frame)(extractor_t *extractor, esp_extractor_frame_info_t *frame_info);

/**
 * @brief  Seek function (optional)
 *
 * @param[in]  extractor  Parent of extractor
 * @param[in]  time       Time position to seek to (unit millisecond)
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid argument
 *       - Others                     Failed to seek
 */
typedef esp_extractor_err_t (*_extractor_seek)(extractor_t *extractor, uint32_t time);

/**
 * @brief  Extractor close function
 *
 * @param[in]  extractor  Parent of extractor
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK       On success
 *       - ESP_EXTRACTOR_ERR_INV_ARG  Invalid argument
 *       - Others                     Failed to close
 */
typedef esp_extractor_err_t (*_extractor_close)(extractor_t *extractor);

/**
 * @brief  Extractor register information
 */
typedef struct {
    _extractor_probe_func       probe;            /*!< Callback to probe for this container */
    _extractor_open             open;             /*!< Open function */
    _extractor_extra_ctrl       extra_ctrl;       /*!< Extra control for extractor (optional)*/
    _extractor_get_stream_num   get_stream_num;   /*!< Get stream number function */
    _extractor_get_stream_info  get_stream_info;  /*!< Get stream information function */
    _extractor_enable_stream    enable_stream;    /*!< Enable stream function */
    _extractor_read_frame       read_frame;       /*!< Read frame function */
    _extractor_seek             seek;             /*!< Seek function (optional) */
    _extractor_close            close;            /*!< Close function */
} extractor_reg_info_t;

/**
 * @brief  Register extractor
 *
 * @param[in]  type   Extractor type
 * @param[in]  table  Extractor register information it must to be `static const` or life cycle over extractor
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK      On success
 *       - ESP_EXTRACTOR_ERR_NO_MEM  No enough memory
 */
esp_extractor_err_t esp_extractor_register(esp_extractor_type_t type, const extractor_reg_info_t *reg_info);

/**
 * @brief  Unregister extractor by extractor type
 *
 * @param[in]  type  Extractor type
 *
 * @return
 *       - ESP_EXTRACTOR_ERR_OK         On success
 *       - ESP_EXTRACTOR_ERR_NOT_FOUND  Such extractor type not registered yet
 *       - ESP_EXTRACTOR_ERR_INV_ARG    Invalid input extractor type
 */
esp_extractor_err_t esp_extractor_unregister(esp_extractor_type_t type);

/**
 * @brief  Malloc buffer from memory pool
 *
 * @note  If use call `esp_extractor_ctrl` with `ESP_EXTRACTOR_CTRL_TYPE_SET_WAIT_OUTPUT`
 *        If there is no enough memory it will block
 *        When malloc size over pool size `over_size` will be set so that can skip this frame
 *
 * @param[in]   extractor  Extractor handle
 * @param[in]   size       Malloc size
 * @param[out]  over_size  Whether malloc size over pool size or not
 *
 * @return
 *       - NULL    Not enough memory in pool
 *       - Others  Malloc pool size
 */
uint8_t *extractor_malloc_output_pool(extractor_t *extractor, uint32_t size, bool *over_size);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
