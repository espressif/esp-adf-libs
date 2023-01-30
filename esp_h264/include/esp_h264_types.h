// Copyright 2023-2026 Espressif Systems (Shanghai) CO., LTD
// All rights reserved.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error number
 */
typedef enum {
    ESP_H264_ERR_OK   = 0,  /*<! Sucessed */
    ESP_H264_ERR_FAIL = -1, /*<! Failed */
    ESP_H264_ERR_ARG  = -2, /*<! Arguments error */
    ESP_H264_ERR_MEM  = -3, /*<! Insufficient memory */
} esp_h264_err_t;

/**
 * @brief This is RAW data format.
 */
typedef enum {
    ESP_H264_RAW_FMT_YUV422, /*<! YUYV. Encoder supported */
    ESP_H264_RAW_FMT_I420,   /*<! IYUV. Encoder supported */
} esp_h264_raw_format_t;

/**
 * @brief Enumerate video frame type
 */
typedef enum {
    ESP_H264_FRAME_TYPE_INVALID = -1, /*<! Encoder not ready or parameters are invalidate */
    ESP_H264_FRAME_TYPE_IDR     = 0,  /*<! IDR frame */
    ESP_H264_FRAME_TYPE_I       = 1,  /*<! I frame type */
    ESP_H264_FRAME_TYPE_P       = 2,  /*<! P frame type */
    ESP_H264_FRAME_TYPE_SKIP    = 3,  /*<! Skip the frame based encoder kernel */
    ESP_H264_FRAME_TYPE_IPMIXED = 4   /*<! A frame where I and P slices are mixing, not supported yet */
} esp_h264_frame_type_t;

/**
 * @brief  H264 Data packet
 */
typedef struct {
    uint8_t *buffer; /*<! Data buffer */
    int      len;    /*<! Buffer length */
} esp_h264_pkt_t;

/**
 * @brief  Unencoded data stream information
 */
typedef struct {
    uint32_t       pts;      /*<! Presentation time stamp. Detailed instroduction refer pts in `esp_h264_enc_frame_t` */
    esp_h264_pkt_t raw_data; /*<! Unencoded data stream  */
} esp_h264_raw_frame_t;

/**
 * @brief  Encoded data stream information
 */
typedef struct {
    uint32_t               pts;          /*<! Presentation time stamp. It is time scale. PTS plus time base is actual time.
                                              Commonly time base in TS stream is {1, 90000}. So PTS uint is 1/90000 second.
                                              If time base is milliseconed, PTS uint is 1 / 1000 second */
    uint32_t               dts;          /*<! Decoding time stamp */
    esp_h264_frame_type_t  frame_type_t; /*<! Frame type */
    esp_h264_pkt_t        *layer_data;   /*<! Encoded data stream. It will be more than one `esp_h264_pkt_t` */
    uint32_t               layer_num;    /*<! Number of `esp_h264_pkt_t` in `layer_data`*/
} esp_h264_enc_frame_t;

#ifdef __cplusplus
}
#endif
