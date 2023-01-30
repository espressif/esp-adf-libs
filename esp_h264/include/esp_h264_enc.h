// Copyright 2023-2026 Espressif Systems (Shanghai) CO., LTD
// All rights reserved.

#pragma once

#include "esp_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *esp_h264_enc_t;

#define DEFAULT_H264_ENCODER_CONFIG()            \
    {                                            \
        .pic_type       = ESP_H264_RAW_FMT_I420, \
        .width          = 320,                   \
        .height         = 192,                   \
        .fps            = 30,                    \
        .gop_size       = 30,                    \
        .target_bitrate = 300000,                \
    }

/**
 * @brief Configure information.
 */
typedef struct {
    esp_h264_raw_format_t pic_type;       /*<! Un-encoding data format */
    int                   width;          /*<! Width. Un-support width is less than 16. */
    int                   height;         /*<! Height. Un-support height is less than 16. */
    int                   fps;            /*<! Maxinum input frame rate */
    int                   gop_size;       /*<! Period of Intra frame */
    int                   target_bitrate; /*<! Target bitrate desired */
} esp_h264_enc_cfg_t;

/**
 * @brief Create an H264 handle to encode
 *
 * @param[in]  cfg    The configuration information
 * @param[out] handle The H264 encoder handle
 *
 * @return
 *       - ESP_H264_ERR_OK: Succeeded
 *       - ESP_H264_ERR_ARG: Arguments error
 *       - ESP_H264_ERR_MEM: Insufficient memory
 *       - ESP_H264_ERR_FAIL: Failed
 */
esp_h264_err_t esp_h264_enc_open(esp_h264_enc_cfg_t *cfg, esp_h264_enc_t *handle);

/**
 * @brief Encode one image
 *
 * @param[in]  handle     The H264 encoder handle. This is come from `esp_h264_enc_open`
 * @param[in]  in_frame   Unencoded data stream information
 * @param[out] out_frame  Encoded data stream information
 *
 * @return
 *       - ESP_H264_ERR_OK: Succeeded
 *       - ESP_H264_ERR_ARG: Arguments error
 *       - ESP_H264_ERR_MEM: Insufficient memory
 *       - ESP_H264_ERR_FAIL: Failed
 */
esp_h264_err_t esp_h264_enc_process(esp_h264_enc_t handle, esp_h264_raw_frame_t *in_frame, esp_h264_enc_frame_t *out_frame);

/**
 * @brief Destroy H264 handle
 *
 * @param[in] handle  The H264 encoder handle
 *
 * @return
 *       - ESP_H264_ERR_OK: Succeeded
 */
esp_h264_err_t esp_h264_enc_close(esp_h264_enc_t handle);

#ifdef __cplusplus
}
#endif
