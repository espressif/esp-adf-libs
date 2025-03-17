#include "esp_imgfx_types.h"

uint32_t pixel_fmt[15] = {
    ESP_IMGFX_PIXEL_FMT_Y,            // ESP_IMGFX_FOURCC('G', 'R', 'E', 'Y'),  /*!< 8 bpp Greyscale */
    ESP_IMGFX_PIXEL_FMT_I420,         // ESP_IMGFX_FOURCC('Y', 'U', '1', '2'),  /*!< 12 bpp YUV 4:2:0 planar, 1 UV pair per 2x2 pixel block */
    ESP_IMGFX_PIXEL_FMT_O_UYY_E_VYY,  // ESP_IMGFX_FOURCC('O', 'U', 'E', 'V'),  /*!< 12 bpp Espressif Y-U-V 4:2:0 format */
    ESP_IMGFX_PIXEL_FMT_RGB565_LE,    // ESP_IMGFX_FOURCC('R', 'G', 'B', 'L'),  /*!< 16 bpp RGB-565 little-endian */
    ESP_IMGFX_PIXEL_FMT_BGR565_LE,    // ESP_IMGFX_FOURCC('B', 'G', 'R', 'L'),  /*!< 16 bpp BGR-565 little-endian */
    ESP_IMGFX_PIXEL_FMT_RGB565_BE,    // ESP_IMGFX_FOURCC('R', 'G', 'B', 'B'),  /*!< 16 bpp RGB-565 big-endian */
    ESP_IMGFX_PIXEL_FMT_BGR565_BE,    // ESP_IMGFX_FOURCC('B', 'G', 'R', 'B'),  /*!< 16 bpp BGR-565 big-endian */
    ESP_IMGFX_PIXEL_FMT_YUYV,         // ESP_IMGFX_FOURCC('Y', 'U', 'Y', 'V'),  /*!< 16 bpp YUYV 4:2:2 packed */
    ESP_IMGFX_PIXEL_FMT_UYVY,         // ESP_IMGFX_FOURCC('U', 'Y', 'V', 'Y'),  /*!< 16 bpp UYVY 4:2:2 packed */
    ESP_IMGFX_PIXEL_FMT_RGB888,       // ESP_IMGFX_FOURCC('R', 'G', 'B', '3'),  /*!< 24 bpp RGB888 */
    ESP_IMGFX_PIXEL_FMT_BGR888,       // ESP_IMGFX_FOURCC('B', 'G', 'R', '3'),  /*!< 24 bpp BGR888 */
    ESP_IMGFX_PIXEL_FMT_YUV_PLANNER,  // ESP_IMGFX_FOURCC('4', '4', '4', 'P'),  /*!< 24 bpp YUV444 planar */
    ESP_IMGFX_PIXEL_FMT_YUV_PACKET,   // ESP_IMGFX_FOURCC('V', '3', '0', '8'),  /*!< 24 bpp YUV444 packed */
    ESP_IMGFX_PIXEL_FMT_ARGB888,      // ESP_IMGFX_FOURCC('A', 'B', '2', '4'),  /*!< 32 bpp ARGB8888 */
    ESP_IMGFX_PIXEL_FMT_ABGR888,      // ESP_IMGFX_FOURCC('A', 'R', '2', '4'),  /*!< 32 bpp ABGR8888 */
};
