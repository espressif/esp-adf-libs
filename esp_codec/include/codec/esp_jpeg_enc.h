// Copyright 2022 Espressif Systems (Shanghai) CO., LTD
// All rights reserved.

#pragma once

#include "stdbool.h"
#include "stdint.h"
#include "esp_jpeg_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* JPEG configure information*/
typedef struct jpeg_info {
    int width;                      /*!< Image wdith */
    int height;                     /*!< Image height */
    jpeg_raw_type_t src_type;       /*!< Input image type */
    jpeg_subsampling_t subsampling; /*!< JPEG chroma subsampling factors.*/
    uint8_t quality;                /*!< Quality: 1-100, higher is better. Typical values are around 40 - 100. */
    bool task_enable;               /*!< True: `jpeg_enc_open`  would create task to finish part of encoding work. false: no task help the encoder encode */
    uint8_t hfm_task_priority;      /*!< Task priority.If task_enable is true, this must be set */
    uint8_t hfm_task_core;          /*!< Task core.If task_enable is true, this must be set */
} jpeg_enc_info_t;

/**
 * @brief      Create an JPEG handle to encode
 *
 * @param      info  The configuration information
 *
 * @return     >0: The JPEG encoder handle
 *             NULL: refer to `jpeg_error_t`
 */
void *jpeg_enc_open(jpeg_enc_info_t *info);

/**
 * @brief      Encode one image
 *
 * @param      handle      The JPEG encoder handle. It gained from `jpeg_enc_open`
 * @param      in_buf      The input buffer, It needs a completed image.
 * @param      inbuf_size  The size of `in_buf`. The value must be size of a completed image.
 * @param      out_buf     The output buffer, it saves a completed JPEG image. The size must be gather than 700 bytes.
 * @param      outbuf_size The size of output buffer
 * @param      out_size    The size of JPEG image
 *
 * @return     `JPEG_ERR_OK`: It has finished to encode one image.
 *             other values refer to `jpeg_error_t`
 */
jpeg_error_t jpeg_enc_process(const void *handle, const uint8_t *in_buf, int inbuf_size, uint8_t *out_buf, int outbuf_size, int *out_size);

/**
 * @brief      Get block size. Block size is minimum process unit.
 *
 * @param      handle      The JPEG encoder handle. It gained from `jpeg_enc_open`
 *
 * @return     positive value     block size
 */
int jpeg_enc_get_block_size(const void *handle);

/**
 * @brief      Encode block size image. Get block size from  `jpeg_enc_get_block_size`
 *
 * @param      handle      The JPEG encoder handle. It gained from `jpeg_enc_open`
 * @param      in_buf      The input buffer, It needs a completed image.
 * @param      inbuf_size  The size of `in_buf`. Get block size from  `jpeg_enc_get_block_size`
 * @param      out_buf     The output buffer, it saves a completed JPEG image. The size must be gather than 700 bytes.
 * @param      outbuf_size The size of output buffer
 * @param      out_size    The size of JPEG image
 *
 * @return     block size: It has finished to encode current block size image.
 *             `JPEG_ERR_OK`: It has finished to encode one image.
 *             `JPEG_ERR_FAIL`:   Encoder failed
 */
int jpeg_enc_process_with_block(const void *handle, const uint8_t *in_buf, int inbuf_size, uint8_t *out_buf, int outbuf_size, int *out_size);

/**
 * @brief      Deinit JPEG handle
 *
 * @param      handle      The JPEG encoder handle. It gained from `jpeg_enc_open`
 *
 * @return     `JPEG_ERR_OK`     It has finished to deinit.
 */
jpeg_error_t jpeg_enc_close(void *handle);

/**
 * Example usage:
 * demo1: It is to encode one image using `jpeg_enc_process`
 * @code{c}
 *
 * int esp_jpeg_enc_demo1 () {
 *  /// configure encoder
 *  jpeg_enc_info_t info;
 *  info.width = 320;
 *  info.height = 240;
 *  info.src_type = JPEG_ENC_SRC_TYPE_YCBCR;
 *  info.subsampling = JPEG_SUB_SAMPLE_YUV444;
 *  info.quality = 50;
 *  info.hfm_task_core = 1;
 *  info.hfm_task_priority = 13;
 *
 *  /// callocate input buffer to fill original image  stream.
 *  int in_len = info.width *info.height * 3;
 *  char *inbuf = malloc(in_len);
 *  if(inbuf == NULL) {
 *      goto exit;
 *  }
 *  /// callocate output buffer to fill encodered image stream.
 *  int out_len = info.width * info.height * 2;
 *  char *outbuf = malloc(out_len);
 *  if (inbuf == NULL) {
 *      goto exit;
 *  }
 *  int out_size = 0;
 *  void* el = jpeg_enc_open(&info);
 *  if (el == NULL) {
 *      goto exit;
 *  }
 *  char in_name[100] = "test.yuv";
 *  char out_name[100] = "test.jpg";
 *  FILE *in = fopen(in_name, "rb");
 *  if (el == NULL){
 *      goto exit;
 *  }
 *  FILE *out = fopen(out_name, "wb");
 *  if (el == NULL) {
 *      fclose(in);
 *      goto exit;
 *  }
 *  int ret = fread(inbuf, 1, in_len, in);
 *  if(ret <= 0) {
 *      fclose(in);
 *      fclose(out);
 *      goto exit;
 *  }
 *  jpeg_enc_process(el, inbuf, in_len, outbuf, out_len, &out_size);
 *  ret = fwrite(outbuf, 1, out_size, out);
 *  if (ret <= 0){
 *      fclose(in);
 *      fclose(out);
 *      goto exit;
 *  }
 *  fclose(in);
 *  fclose(out);
 *  exit:
 *  jpeg_enc_close(el);
 *  free(inbuf);
 *  free(outbuf);
 * }

 *  demo2: It is to encode one image using `jpeg_enc_process_with_block`
 * int esp_jpeg_enc_demo2 () {
 *  jpeg_enc_info_t info;
 *  info.width = 320;
 *  info.height = 240;
 *  info.src_type = JPEG_ENC_SRC_TYPE_YCBCR;
 *  info.subsampling = JPEG_SUB_SAMPLE_YUV444;
 *  info.quality = 50;
 *  info.hfm_task_core = 1;
 *  info.hfm_task_priority = 13;
 *  int in_len = jpeg_enc_get_block_size(el);
 *  char *inbuf = malloc(in_len);
 *  if(inbuf == NULL) {
 *      goto exit;
 *  }
 *  int num_times = info.width * info.height * 3 / in_len;
 *  int out_len = info.width * info.height * 2;
 *  char *outbuf = malloc(out_len);
 *  if (inbuf == NULL){
 *      goto exit;
 *  }
 *  int out_size = 0;
 *  void* el = jpeg_enc_open(&info);
 *  if (el == NULL){
 *      goto exit;
 *  }
 *
 *  char in_name[100] = "test.yuv";
 *  char out_name[100] = "test.jpg";
 *  FILE *in = fopen(in_name, "rb") if (el == NULL)
 *  {
 *      goto exit;
 *  }
 *  FILE *out = fopen(out_name, "wb") if (el == NULL)
 *  {
 *      fclose(in);
 *      goto exit;
 *  }
 *  for (size_t j = 0; j < num_times; j++) {
 *     int ret = fread(inbuf, 1, in_len, in);
 *     if(ret <= 0) {
 *         ret = fwrite(outbuf, 1, out_size, out);
 *         fclose(in);
 *         fclose(out);
 *         goto exit;
 *     }
 *     jpeg_enc_process_with_block(el, inbuf, in_len, outbuf, out_len, &out_size);
 *  }
 *  fclose(in);
 *  fclose(out);
 *  exit:
 *  jpeg_enc_close(el);
 *  free(inbuf);
 *  free(outbuf);
 * }
 * @endcode
 */

#ifdef __cplusplus
}
#endif
