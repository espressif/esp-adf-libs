/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/** Edge index: top, right, bottom, left (matches internal partition and shift_match_count[]) */
#define ESP_AMBIENT_LED_EDGE_COUNT   4
#define ESP_AMBIENT_LED_EDGE_TOP     0
#define ESP_AMBIENT_LED_EDGE_RIGHT   1
#define ESP_AMBIENT_LED_EDGE_BOTTOM  2
#define ESP_AMBIENT_LED_EDGE_LEFT    3

/** Four corners plus four edge midpoints */
#define ESP_AMBIENT_LED_CTRL_POINT_COUNT  8

#define ESP_AMBIENT_LED_DEFAULT_ROI_TH          40
#define ESP_AMBIENT_LED_DEFAULT_DELAY_FRAMES    2
#define ESP_AMBIENT_LED_DEFAULT_RECOVER_FRAMES  2
#define ESP_AMBIENT_LED_DEFAULT_RG_RATIO_MAX    0.2f  /*!< Default max |R-G|/G for near-gray (black border) */
#define ESP_AMBIENT_LED_DEFAULT_BG_RATIO_MAX    0.2f  /*!< Default max |B-G|/G for near-gray (black border) */
#define ESP_AMBIENT_LED_DEFAULT_MAX_SHIFT       0.2   /*!< Default per-edge max inward shift ratio */
#define ESP_AMBIENT_LED_DEFAULT_SHIFT_STEP      0.05  /*!< Default per-edge shift step ratio */
#define ESP_AMBIENT_LED_DEFAULT_ROI_SCALE       0.05  /*!< Default per-edge ROI shrink scale */
#define ESP_AMBIENT_LED_DEFAULT_IMAGE_WIDTH     1280  /*!< Default image width */
#define ESP_AMBIENT_LED_DEFAULT_IMAGE_HEIGHT    720   /*!< Default image height */
#define ESP_AMBIENT_LED_DEFAULT_ROI_TOP_BOTTOM  16    /*!< Default color-block count on top/bottom */
#define ESP_AMBIENT_LED_DEFAULT_ROI_RIGHT_LEFT  9     /*!< Default color-block count on left/right */

#define ESP_AMBIENT_LED_FOURCC(a, b, c, d)  \
    (((int)(a)) | (((int)(b)) << 8) | (((int)(c)) << 16) | (((int)(d)) << 24))

/**
 * @brief  Return codes for status-returning esp_ambient_led_* functions
 */
typedef enum {
    ESP_AMBIENT_LED_OK              = 0,   /*!< Success */
    ESP_AMBIENT_LED_ERR_INVALID_ARG = -1,  /*!< NULL pointer or out-of-range argument */
    ESP_AMBIENT_LED_ERR_NO_MEM      = -2,  /*!< Memory allocation failed */
    ESP_AMBIENT_LED_ERR_DEGENERATE  = -3,  /*!< Degenerate geometry (collinear points or near-zero area) */
} esp_ambient_led_err_t;

/**
 * @brief  2D point in image coordinates
 *
 *         Stored as double so eight-point arcs and inward shift stay sub-pixel.
 *         Integer pixel indices are used only for axis-aligned ROI rectangles.
 */
typedef struct {
    double  x;  /*!< X coordinate in pixels (sub-pixel) */
    double  y;  /*!< Y coordinate in pixels (sub-pixel) */
} esp_ambient_led_pt2_t;

/**
 * @brief  Per-edge scalars (top, right, bottom, left)
 *
 *         Not a pixel rectangle.
 *         Used for shift ratio, shift step, brightness threshold, and ROI scale.
 */
typedef struct {
    double  top;     /*!< Top edge value */
    double  bottom;  /*!< Bottom edge value */
    double  left;    /*!< Left edge value */
    double  right;   /*!< Right edge value */
} esp_ambient_led_edges_t;

/** @deprecated  Use esp_ambient_led_edges_t */
typedef esp_ambient_led_edges_t esp_ambient_led_shift_t;

/**
 * @brief  Region of Interest (ROI) block counts along top/bottom and left/right chains
 */
typedef struct {
    uint8_t  top_bottom;  /*!< Block count on top and bottom edges. Valid range is 2 to 255 */
    uint8_t  right_left;  /*!< Color-block count on left and right edges (same role as top_bottom) */
} esp_ambient_led_roi_num_t;

/**
 * @brief  Opaque vertical partition handle
 */
typedef void *esp_ambient_led_handle_t;

/**
 * @brief  Start corner for eight-point walk (which corner maps to canonical ctrl_points[0])
 *
 *         After reorder: 0-1-2 top, 2-3-4 right, 4-5-6 bottom, 6-7-0 left (corners at 0/2/4/6)
 */
typedef enum {
    ESP_AMBIENT_LED_CORNER_TL = 0,  /*!< Top-left, canonical ctrl_points[0] */
    ESP_AMBIENT_LED_CORNER_TR = 1,  /*!< Top-right, canonical ctrl_points[2] */
    ESP_AMBIENT_LED_CORNER_BR = 2,  /*!< Bottom-right, canonical ctrl_points[4] */
    ESP_AMBIENT_LED_CORNER_BL = 3,  /*!< Bottom-left, canonical ctrl_points[6] */
} __attribute__((packed)) esp_ambient_led_corner_t;

/**
 * @brief  Winding direction (image Y axis down, viewed from front of TV)
 *
 *         CLOCKWISE: walk canonical ctrl_points / edges in increasing index order — on screen
 *         from top-left that is 上→右→下→左.
 *         COUNTER_CLOCKWISE: decreasing index order — 上→左→下→右 from top-left.
 */
typedef enum {
    ESP_AMBIENT_LED_WIND_CCW = 0,  /*!< Screen CCW: canonical index decreases per step */
    ESP_AMBIENT_LED_WIND_CW  = 1,  /*!< Screen CW: canonical index increases per step */
} __attribute__((packed)) esp_ambient_led_winding_t;

/* Default values (must follow enum definitions above) */
#define ESP_AMBIENT_LED_CORNER_DEFAULT      ESP_AMBIENT_LED_CORNER_TL  /*!< Default start corner for pts walk */
#define ESP_AMBIENT_LED_WIND_DEFAULT        ESP_AMBIENT_LED_WIND_CW    /*!< Default winding direction */
/* Aliases kept for source compatibility */
#define ESP_AMBIENT_LED_ROI_CORNER_DEFAULT  ESP_AMBIENT_LED_CORNER_DEFAULT
#define ESP_AMBIENT_LED_ROI_WIND_DEFAULT    ESP_AMBIENT_LED_WIND_DEFAULT

/**
 * @brief  Supported input image pixel formats
 */
typedef enum {
    ESP_AMBIENT_LED_FORMAT_RGB565_LE = ESP_AMBIENT_LED_FOURCC('R', '5', '6', 'L'),  /*!< RGB565 little-endian */
    ESP_AMBIENT_LED_FORMAT_RGB565_BE = ESP_AMBIENT_LED_FOURCC('R', '5', '6', 'B'),  /*!< RGB565 big-endian */
    ESP_AMBIENT_LED_FORMAT_RGB888    = ESP_AMBIENT_LED_FOURCC('R', 'G', 'B', '3'),  /*!< RGB888 packed R,G,B */
    ESP_AMBIENT_LED_FORMAT_YUYV      = ESP_AMBIENT_LED_FOURCC('Y', 'U', 'Y', 'V'),  /*!< Packed YUV422: YUYV per line; converted to RGB internally; width must be even */
} esp_ambient_led_format_t;

/**
 * @brief  Eight control points plus walk start corner and winding
 */
typedef struct {
    esp_ambient_led_pt2_t      ctrl_points[ESP_AMBIENT_LED_CTRL_POINT_COUNT];  /*!< Corners and edge midpoints */
    esp_ambient_led_corner_t   start_corner;    /*!< Default ESP_AMBIENT_LED_CORNER_TL */
    esp_ambient_led_winding_t  winding;         /*!< Default ESP_AMBIENT_LED_WIND_CW */
} esp_ambient_led_pts_cfg_t;

/**
 * @brief  Region of Interest (ROI) layout and color-block output walk order
 *
 *         Internal geometry stays TOP to RIGHT to BOTTOM to LEFT; only cblk_samples[] order is remapped.
 *         ESP_AMBIENT_LED_WIND_CW from top-left visits edges 上→右→下→左.
 */
typedef struct {
    esp_ambient_led_roi_num_t  roi_number;    /*!< Block counts per chain */
    esp_ambient_led_edges_t    scale;         /*!< Region of Interest (ROI) shrink scale per edge */
    esp_ambient_led_corner_t   start_corner;  /*!< Output index 0 starts on this edge */
    esp_ambient_led_winding_t  winding;       /*!< Screen CW/CCW edge walk */
} esp_ambient_led_roi_num_cfg_t;

/**
 * @brief  Dynamic inward Region of Interest (ROI) shift
 *
 *         When recover_frames is 0, shift stays at start_index
 */
typedef struct {
    esp_ambient_led_edges_t  max_shift;                                      /*!< Maximum shift ratio per edge, must be in (0, 0.5] */
    esp_ambient_led_edges_t  shift_step;                                     /*!< Shift step ratio per edge */
    esp_ambient_led_edges_t  roi_th;                                         /*!< Per-edge brightness threshold */
    uint8_t                  delay_frames;                                   /*!< Frames to debounce shift level change */
    uint8_t                  recover_frames;                                 /*!< Frames between full shift scans, 0 disables */
    float                    rg_ratio_min;                                   /*!< Min R/G for near-gray trigger (0 = no lower bound) */
    float                    rg_ratio_max;                                   /*!< Max R/G for near-gray trigger */
    float                    bg_ratio_min;                                   /*!< Min B/G for near-gray trigger (0 = no lower bound) */
    float                    bg_ratio_max;                                   /*!< Max B/G for near-gray trigger */
    uint8_t                  shift_match_count[ESP_AMBIENT_LED_EDGE_COUNT];  /*!< Min matching blocks, 0 means all on edge */
    uint8_t                  start_index[ESP_AMBIENT_LED_EDGE_COUNT];        /*!< Initial shift level per edge; recover also
                                                                         resets to this value.  Clamped to
                                                                         [0, max_shift_number] at init.  Default 0. */
    uint8_t                  joint_shift;                                    /*!< When non-zero, after each edge's own
                                                                                 inward shift, the neighbor's inset inner
                                                                                 boundary also moves this edge's ROIs
                                                                                 toward the center */
} esp_ambient_led_shift_cfg_t;

/**
 * @brief  How filtered ROI pixels are reduced to one RGB sample
 */
typedef enum {
    ESP_AMBIENT_LED_SAMPLE_MODE_MEAN     = 0,  /*!< Mean of pixels that pass the filter */
    ESP_AMBIENT_LED_SAMPLE_MODE_DOMINANT = 1,  /*!< Dominant color (4-bit RGB histogram mode) after filter */
} __attribute__((packed)) esp_ambient_led_sample_mode_t;

/**
 * @brief  Pixel filter applied when computing cblk_samples for user output
 *
 *         Per-channel (R/G/B) min/max: reject only when R, G, and B are all outside
 *         their active bounds at the same time; keep if any channel is still inside.
 *         Ratio box is inverted: pixels inside the R/G × B/G band are excluded;
 *         pixels outside are kept for reduction (mean or dominant).
 *         exclude when:
 *         rg_ratio_min <= R/max(G,1) <= rg_ratio_max
 *         AND bg_ratio_min <= B/max(G,1) <= bg_ratio_max
 *
 *         A value of 0 for any max field means that ceiling is disabled (no upper limit).
 *         A value of 0 for any min field means no lower bound.
 *         If all four ratio fields are 0, no ratio filtering is applied.
 *         mode selects mean vs dominant-color reduction on the surviving pixels.
 *         min_ok_percent is the minimum share of ROI pixels that must pass the
 *         filter (1..100). 0 keeps the roi_color rule: any surviving pixel is enough.
 *         Below that share the ROI falls back to the unfiltered mean.
 *         Black border detection (shift trigger) always uses unfiltered mean.
 */
typedef struct {
    uint8_t                        r_min;           /*!< Minimum R value: R >= r_min (0 = disabled) */
    uint8_t                        r_max;           /*!< Maximum R value: R <= r_max (0 = disabled) */
    uint8_t                        g_min;           /*!< Minimum G value: G >= g_min (0 = disabled) */
    uint8_t                        g_max;           /*!< Maximum G value: G <= g_max (0 = disabled) */
    uint8_t                        b_min;           /*!< Minimum B value: B >= b_min (0 = disabled) */
    uint8_t                        b_max;           /*!< Maximum B value: B <= b_max (0 = disabled) */
    float                          rg_ratio_min;    /*!< Reject band lower: R/max(G,1) (0 = no lower bound) */
    float                          rg_ratio_max;    /*!< Reject band upper: R/max(G,1) (0 = no upper bound) */
    float                          bg_ratio_min;    /*!< Reject band lower: B/max(G,1) (0 = no lower bound) */
    float                          bg_ratio_max;    /*!< Reject band upper: B/max(G,1) (0 = no upper bound) */
    esp_ambient_led_sample_mode_t  mode;            /*!< MEAN or DOMINANT */
    uint8_t                        min_ok_percent;  /*!< Min pass ratio 1..100; 0 = any pass count > 0 */
} esp_ambient_led_sample_filter_cfg_t;

/**
 * @brief  Input image configuration
 *
 * @note  For `ESP_AMBIENT_LED_FORMAT_YUYV`, width must be even.
 */
typedef struct {
    int                       width;   /*!< Image width in pixels */
    int                       height;  /*!< Image height in pixels */
    esp_ambient_led_format_t  format;  /*!< Pixel format */
} esp_ambient_led_image_cfg_t;

/**
 * @brief  Full vertical partition configuration
 */
typedef struct {
    esp_ambient_led_pts_cfg_t            pts;            /*!< Eight-point geometry */
    esp_ambient_led_image_cfg_t          image;          /*!< Source image size and format */
    esp_ambient_led_roi_num_cfg_t        roi_number;     /*!< Region of Interest (ROI) counts and output walk */
    esp_ambient_led_shift_cfg_t          shift;          /*!< Dynamic shift parameters */
    esp_ambient_led_sample_filter_cfg_t  sample_filter;  /*!< Pixel filter for user cblk_samples output */
} esp_ambient_led_cfg_t;

/**
 * @brief  Per-color-block Red Green Blue (RGB) sample
 */
typedef struct {
    uint8_t  rgb[3];  /*!< RGB sample (mean or dominant), rgb[0]=R rgb[1]=G rgb[2]=B */
} esp_ambient_led_sample_t;

/**
 * @brief  Build a default configuration for an image of size w by h
 *
 * @note  Does not allocate and has no shared state. Control points span the full
 *        frame. Format is RGB888. ROI counts are 16 (top/bottom) and 9 (left/right).
 *        Shift and the sample filter use the ESP_AMBIENT_LED_DEFAULT_* values.
 *        A non-positive w or h falls back to 1280x720.
 *        Replace ctrl_points with real calibration before production use.
 *        The returned struct is about a few hundred bytes; call this at init time.
 *
 * @param[in]  w  Image width in pixels
 * @param[in]  h  Image height in pixels
 *
 * @return  Default configuration. The caller owns the returned value
 */
static inline esp_ambient_led_cfg_t esp_ambient_led_cfg_get_defaults(int w, int h)
{
    esp_ambient_led_cfg_t cfg;
    double fw;
    double fh;

    if (w <= 0) {
        w = ESP_AMBIENT_LED_DEFAULT_IMAGE_WIDTH;
    }
    if (h <= 0) {
        h = ESP_AMBIENT_LED_DEFAULT_IMAGE_HEIGHT;
    }
    memset(&cfg, 0, sizeof(cfg));

    cfg.image.width = w;
    cfg.image.height = h;
    cfg.image.format = ESP_AMBIENT_LED_FORMAT_RGB888;

    fw = (double)w;
    fh = (double)h;
    cfg.pts.ctrl_points[0].x = 0.0;
    cfg.pts.ctrl_points[0].y = 0.0;
    cfg.pts.ctrl_points[1].x = fw * 0.5;
    cfg.pts.ctrl_points[1].y = 0.0;
    cfg.pts.ctrl_points[2].x = fw - 1.0;
    cfg.pts.ctrl_points[2].y = 0.0;
    cfg.pts.ctrl_points[3].x = fw - 1.0;
    cfg.pts.ctrl_points[3].y = fh * 0.5;
    cfg.pts.ctrl_points[4].x = fw - 1.0;
    cfg.pts.ctrl_points[4].y = fh - 1.0;
    cfg.pts.ctrl_points[5].x = fw * 0.5;
    cfg.pts.ctrl_points[5].y = fh - 1.0;
    cfg.pts.ctrl_points[6].x = 0.0;
    cfg.pts.ctrl_points[6].y = fh - 1.0;
    cfg.pts.ctrl_points[7].x = 0.0;
    cfg.pts.ctrl_points[7].y = fh * 0.5;
    cfg.pts.start_corner = ESP_AMBIENT_LED_CORNER_DEFAULT;
    cfg.pts.winding = ESP_AMBIENT_LED_WIND_DEFAULT;

    cfg.roi_number.roi_number.top_bottom = ESP_AMBIENT_LED_DEFAULT_ROI_TOP_BOTTOM;
    cfg.roi_number.roi_number.right_left = ESP_AMBIENT_LED_DEFAULT_ROI_RIGHT_LEFT;
    cfg.roi_number.scale.top = ESP_AMBIENT_LED_DEFAULT_ROI_SCALE;
    cfg.roi_number.scale.bottom = ESP_AMBIENT_LED_DEFAULT_ROI_SCALE;
    cfg.roi_number.scale.left = ESP_AMBIENT_LED_DEFAULT_ROI_SCALE;
    cfg.roi_number.scale.right = ESP_AMBIENT_LED_DEFAULT_ROI_SCALE;
    cfg.roi_number.start_corner = ESP_AMBIENT_LED_CORNER_DEFAULT;
    cfg.roi_number.winding = ESP_AMBIENT_LED_WIND_DEFAULT;

    cfg.shift.max_shift.top = ESP_AMBIENT_LED_DEFAULT_MAX_SHIFT;
    cfg.shift.max_shift.bottom = ESP_AMBIENT_LED_DEFAULT_MAX_SHIFT;
    cfg.shift.max_shift.left = ESP_AMBIENT_LED_DEFAULT_MAX_SHIFT;
    cfg.shift.max_shift.right = ESP_AMBIENT_LED_DEFAULT_MAX_SHIFT;
    cfg.shift.shift_step.top = ESP_AMBIENT_LED_DEFAULT_SHIFT_STEP;
    cfg.shift.shift_step.bottom = ESP_AMBIENT_LED_DEFAULT_SHIFT_STEP;
    cfg.shift.shift_step.left = ESP_AMBIENT_LED_DEFAULT_SHIFT_STEP;
    cfg.shift.shift_step.right = ESP_AMBIENT_LED_DEFAULT_SHIFT_STEP;
    cfg.shift.roi_th.top = ESP_AMBIENT_LED_DEFAULT_ROI_TH;
    cfg.shift.roi_th.bottom = ESP_AMBIENT_LED_DEFAULT_ROI_TH;
    cfg.shift.roi_th.left = ESP_AMBIENT_LED_DEFAULT_ROI_TH;
    cfg.shift.roi_th.right = ESP_AMBIENT_LED_DEFAULT_ROI_TH;
    cfg.shift.delay_frames = ESP_AMBIENT_LED_DEFAULT_DELAY_FRAMES;
    cfg.shift.recover_frames = ESP_AMBIENT_LED_DEFAULT_RECOVER_FRAMES;
    cfg.shift.rg_ratio_max = ESP_AMBIENT_LED_DEFAULT_RG_RATIO_MAX;
    cfg.shift.bg_ratio_max = ESP_AMBIENT_LED_DEFAULT_BG_RATIO_MAX;

    cfg.sample_filter.mode = ESP_AMBIENT_LED_SAMPLE_MODE_MEAN;
    return cfg;
}

/**
 * @brief  Initialize partition: arcs, grids, Region of Interest (ROI) caches, shift tables
 *
 * @param[in]   cfg     Partition configuration; all fields are validated before any allocation
 * @param[out]  handle  Created handle, must be released with esp_ambient_led_deinit()
 *
 * @return
 *       - ESP_AMBIENT_LED_OK               Succeeded
 *       - ESP_AMBIENT_LED_ERR_INVALID_ARG  cfg or handle is NULL, ctrl_points contains NaN/Inf, image config is
 *                                          invalid, or a configuration field is out of range
 *       - ESP_AMBIENT_LED_ERR_NO_MEM       Memory allocation failed
 *       - ESP_AMBIENT_LED_ERR_DEGENERATE   Eight input points form a degenerate arc geometry (collinear
 *                                          midpoints or near-zero arc radius)
 */
esp_ambient_led_err_t esp_ambient_led_init(const esp_ambient_led_cfg_t *cfg,
                                           esp_ambient_led_handle_t *handle);

/**
 * @brief  Free handle from esp_ambient_led_init()
 *
 * @param  handle  Handle to free, no-op if NULL
 */
void esp_ambient_led_deinit(esp_ambient_led_handle_t handle);

/**
 * @brief  Sample all color blocks from one frame using current shift-level Region of Interest (ROI)s
 *
 * @param[in]   handle        Initialized partition handle. Shift state inside the handle is updated
 * @param[in]   src           Image buffer; format and size must match init configuration
 * @param[out]  cblk_samples  Output array; caller must allocate 2*top_bottom + 2*right_left elements
 *
 * @return
 *       - ESP_AMBIENT_LED_OK               Succeeded
 *       - ESP_AMBIENT_LED_ERR_INVALID_ARG  handle, src, or cblk_samples is NULL, or handle is not initialized
 */
esp_ambient_led_err_t esp_ambient_led_process(esp_ambient_led_handle_t handle, const uint8_t *src,
                                              esp_ambient_led_sample_t *cblk_samples);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
