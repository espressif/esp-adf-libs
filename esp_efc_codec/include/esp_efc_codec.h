/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  EFC codec error codes
 */
typedef enum {
    ESP_EFC_OK                = 0,   /*!< Success */
    ESP_EFC_ERR_INVALID_ARG   = -1,  /*!< NULL pointer, bad geometry, or an out-of-range index */
    ESP_EFC_ERR_INVALID_STATE = -2,  /*!< Frame decoded out of order; decode is sequential */
    ESP_EFC_ERR_INVALID_DATA  = -3,  /*!< Unknown opcode or an illegal color-key index */
    ESP_EFC_ERR_INVALID_SIZE  = -4,  /*!< Frame payload missing or shorter than its directory length */
    ESP_EFC_ERR_NO_MEM        = -5,  /*!< Allocation failed */
} esp_efc_err_t;

/**
 * @brief  EFC sequential decoder handle
 *
 *         EFC (Emote Frame Codec), a lossless tile-based codec for short RGB565 clips
 */
typedef struct esp_efc_dec *esp_efc_dec_handle_t;

/**
 * @brief  EFC decoder information
 */
typedef struct {
    int       width;                /*!< Frame width in pixels */
    int       height;               /*!< Frame height in pixels */
    int       frame_count;          /*!< Number of frames in the clip */
    int       tile_size;            /*!< Tile size in pixels */
    int       token_width;          /*!< Bytes per token: 1 (palette) or 2 (raw RGB565) */
    int       palette_color_count;  /*!< Palette colors, or 0 for raw RGB565 tokens */
    int       color_key_index;      /*!< Key palette index after frame 0 is decoded, otherwise -1 */
    uint16_t  color_key;            /*!< RGB565 transparency key; valid when color_key_index >= 0 */
    int       changed_x_start;      /*!< First changed pixel column in the last decoded frame */
    int       changed_x_end;        /*!< One past the last changed column; equal to start when unchanged */
    int       changed_y_start;      /*!< First changed pixel row in the last decoded frame */
    int       changed_y_end;        /*!< One past the last changed row; equal to start when unchanged */
} esp_efc_dec_info_t;


/**
 * @brief  Open a decoder over an EFC blob
 *
 *         The blob is referenced, not copied. It must stay valid for the life of
 *         the decoder. When ESP_EFC_DECODE_4X4_ONLY is defined the decoder accepts
 *         only fixed 4x4-grid blobs (pair with encoding at tile 4).
 *
 * @param[in]   blob      Pointer to the EFC blob
 * @param[in]   blob_len  Length of the blob in bytes
 * @param[out]  handle    Receives the decoder handle
 *
 * @return
 *       - ESP_EFC_OK                Success
 *       - ESP_EFC_ERR_INVALID_ARG   NULL blob or handle pointer
 *       - ESP_EFC_ERR_INVALID_DATA  Invalid blob or unsupported layout
 *       - ESP_EFC_ERR_INVALID_SIZE  Blob is too short
 *       - ESP_EFC_ERR_NO_MEM        Allocation failed
 */
esp_efc_err_t esp_efc_dec_open(const uint8_t *blob, size_t blob_len, esp_efc_dec_handle_t *handle);

/**
 * @brief  Get decoder information
 *
 * @param[in]   handle  Decoder handle
 * @param[out]  info    Receives all clip and decoder information
 *
 * @return
 *       - ESP_EFC_OK               Success
 *       - ESP_EFC_ERR_INVALID_ARG  NULL handle or info pointer
 */
esp_efc_err_t esp_efc_dec_get_info(esp_efc_dec_handle_t handle, esp_efc_dec_info_t *info);

/**
 * @brief  Decode one frame into a persistent RGB565 output buffer
 *
 *         Frames must be decoded in order (0, 1, 2, ...) because a later frame
 *         predicts from the previous one. Decoding the same index twice in a row
 *         is allowed. The output buffer must persist across calls: rows the frame
 *         does not touch are assumed to still hold the previous frame.
 *
 * @param[in]   handle      Decoder handle
 * @param[in]   index       Frame index to decode
 * @param[out]  rgb565_out  Output buffer of width * height uint16 pixels
 *
 * @return
 *       - ESP_EFC_OK                 Success
 *       - ESP_EFC_ERR_INVALID_ARG    NULL handle or buffer, or index out of range
 *       - ESP_EFC_ERR_INVALID_STATE  Frame is not the next frame in order
 *       - ESP_EFC_ERR_INVALID_DATA   Unknown opcode or an illegal color-key index
 *       - ESP_EFC_ERR_INVALID_SIZE   Frame payload missing or short
 */
esp_efc_err_t esp_efc_dec_frame(esp_efc_dec_handle_t handle, int index, uint16_t *rgb565_out);

/**
 * @brief  Decode one frame into RGB565A8 (color plane, then alpha plane)
 *
 *         Layout matches the EAF player image: uint16 RGB565 pixels for
 *         width * height, stride width * 2, then one alpha byte per pixel
 *         (0x00 for the color key, 0xFF for opaque). The buffer is
 *         width * height * 2 + width * height bytes and must persist across
 *         frames, same as esp_efc_dec_frame(). A clip with no color key is fully
 *         opaque. The color plane is the same pixels esp_efc_dec_frame() writes.
 *
 * @param[in]   handle    Decoder handle
 * @param[in]   index     Frame index to decode
 * @param[out]  rgb565a8  Color plane followed by the alpha plane
 *
 * @return
 *       - ESP_EFC_OK                 Success
 *       - ESP_EFC_ERR_INVALID_ARG    NULL handle or buffer, or index out of range
 *       - ESP_EFC_ERR_INVALID_STATE  Frame is not the next frame in order
 *       - ESP_EFC_ERR_INVALID_DATA   Unknown opcode or an illegal color-key index
 *       - ESP_EFC_ERR_INVALID_SIZE   Frame payload missing or short
 */
esp_efc_err_t esp_efc_dec_frame_with_alpha(esp_efc_dec_handle_t handle, int index, uint8_t *rgb565a8);

/**
 * @brief  Close a decoder and free its resources
 *
 * @param[in]  handle  Decoder handle (NULL is ignored)
 */
void esp_efc_dec_close(esp_efc_dec_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
