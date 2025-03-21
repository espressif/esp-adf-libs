#include "color_convert.h"

void test_esp_imgfx_color_convert_open(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_color_convert_close(handle);
}

void test_esp_imgfx_color_convert_process(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    uint8_t *input_buf = NULL;
    uint8_t *output_buf = NULL;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&input_buf, 128, 640 * 480 * 2));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&output_buf, 128, 640 * 480 * 3));
    esp_imgfx_data_t in = {.data = input_buf, .data_len = 640 * 480 * 2};
    esp_imgfx_data_t out = {.data = output_buf, .data_len = 640 * 480 * 3};
    ret = esp_imgfx_color_convert_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    esp_imgfx_color_convert_close(handle);
    free(input_buf);
    free(output_buf);
}

void test_esp_imgfx_color_convert_open_invalid_cfg(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {0, 0},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_color_convert_open_unsupported_format(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = 255,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_NOT_SUPPORTED, ret);
}

void test_esp_imgfx_color_convert_get_cfg(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    esp_imgfx_color_convert_cfg_t new_cfg;
    ret = esp_imgfx_color_convert_get_cfg(handle, &new_cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    TEST_ASSERT_EQUAL(cfg.in_res.width, new_cfg.in_res.width);
    TEST_ASSERT_EQUAL(cfg.in_res.height, new_cfg.in_res.height);
    TEST_ASSERT_EQUAL(cfg.in_pixel_fmt, new_cfg.in_pixel_fmt);
    TEST_ASSERT_EQUAL(cfg.out_pixel_fmt, new_cfg.out_pixel_fmt);
    TEST_ASSERT_EQUAL(cfg.color_space_std, new_cfg.color_space_std);

    esp_imgfx_color_convert_close(handle);
}

void test_esp_imgfx_color_convert_get_cfg_invalid_handle(void)
{
    esp_imgfx_color_convert_cfg_t cfg;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_get_cfg(NULL, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_color_convert_set_cfg(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    cfg.out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
    ret = esp_imgfx_color_convert_set_cfg(handle, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    esp_imgfx_color_convert_close(handle);
}

void test_esp_imgfx_color_convert_set_cfg_invalid_parameter(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    cfg.in_pixel_fmt = 255;
    ret = esp_imgfx_color_convert_set_cfg(handle, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_NOT_SUPPORTED, ret);

    esp_imgfx_color_convert_close(handle);
}

void test_esp_imgfx_color_convert_process_insufficient_data(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    uint8_t *input_buf = NULL;
    uint8_t *output_buf = NULL;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&input_buf, 128, 640 * 480 * 2 - 1));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&output_buf, 128, 640 * 480 * 3));
    esp_imgfx_data_t in = {.data = input_buf, .data_len = 640 * 480 * 2 - 1};
    esp_imgfx_data_t out = {.data = output_buf, .data_len = 640 * 480 * 3};
    ret = esp_imgfx_color_convert_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_DATA_LACK, ret);

    esp_imgfx_color_convert_close(handle);
    free(input_buf);
    free(output_buf);
}

void test_esp_imgfx_color_convert_process_insufficient_buffer(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    uint8_t *input_buf = NULL;
    uint8_t *output_buf = NULL;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&input_buf, 128, 640 * 480 * 2));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&output_buf, 128, 640 * 480 * 3 - 1));
    esp_imgfx_data_t in = {.data = input_buf, .data_len = 640 * 480 * 2};
    esp_imgfx_data_t out = {.data = output_buf, .data_len = 640 * 480 * 3 - 1};

    ret = esp_imgfx_color_convert_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_BUFF_NOT_ENOUGH, ret);

    esp_imgfx_color_convert_close(handle);
    free(input_buf);
    free(output_buf);
}

void test_esp_imgfx_color_convert_process_invalid_parameter(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    uint8_t *input_buf = NULL;
    uint8_t *output_buf = NULL;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&input_buf, 128, 640 * 480 * 2));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&output_buf, 128, 640 * 480 * 3));
    esp_imgfx_data_t in = {.data = input_buf, .data_len = 640 * 480 * 2};
    esp_imgfx_data_t out = {.data = output_buf, .data_len = 640 * 480 * 3};
    ret = esp_imgfx_color_convert_process(NULL, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);

    esp_imgfx_color_convert_close(handle);
    free(input_buf);
    free(output_buf);
}

void test_esp_imgfx_color_convert_close(void)
{
    esp_imgfx_color_convert_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    esp_imgfx_color_convert_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_color_convert_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    ret = esp_imgfx_color_convert_close(handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
}

#define DEFINE_CC_FUNC0(func_name, diff_func, out_fmt)                                                                            \
    void func_name(int16_t *r_max, int16_t *g_max, int16_t *b_max)                                                                \
    {                                                                                                                             \
        *r_max = 0;                                                                                                               \
        *g_max = 0;                                                                                                               \
        *b_max = 0;                                                                                                               \
        esp_imgfx_data_t in_image = {0};                                                                                          \
        esp_imgfx_data_t out_image = {0};                                                                                         \
        int16_t width = 1;                                                                                                        \
        int16_t height = 1;                                                                                                       \
        int16_t r = 255, g = 0, b = 0;                                                                                            \
        int16_t r_diff = 0, g_diff = 0, b_diff = 0;                                                                               \
        esp_imgfx_color_convert_handle_t handle = NULL;                                                                           \
        esp_imgfx_color_convert_cfg_t cfg = {                                                                                     \
            .in_res.width = width,                                                                                                \
            .in_res.height = height,                                                                                              \
            .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_YUV_PACKET,                                                                       \
            .out_pixel_fmt = out_fmt,                                                                                             \
            .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT709,                                                                   \
        };                                                                                                                        \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_open(&cfg, &handle));                                         \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len));         \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.out_pixel_fmt, &cfg.in_res, &out_image.data_len));       \
        in_image.data = (uint8_t *)malloc(in_image.data_len);                                                                     \
        TEST_ASSERT_NOT_NULL(in_image.data);                                                                                      \
        out_image.data = (uint8_t *)malloc(out_image.data_len);                                                                   \
        TEST_ASSERT_NOT_NULL(out_image.data);                                                                                     \
        for (r = 0; r < 256; r++) {                                                                                               \
            for (g = 0; g < 256; g++) {                                                                                           \
                for (b = 0; b < 256; b++) {                                                                                       \
                    yuv_packet_image_gen(cfg.in_res.width, cfg.in_res.height, (uint8_t)r, (uint8_t)g, (uint8_t)b, in_image.data); \
                    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_process(handle, &in_image, &out_image));          \
                    diff_func(out_image.data, (uint8_t)r, (uint8_t)g, (uint8_t)b, &r_diff, &g_diff, &b_diff);                     \
                    *r_max = r_diff > *r_max ? r_diff : *r_max;                                                                   \
                    *g_max = g_diff > *g_max ? g_diff : *g_max;                                                                   \
                    *b_max = b_diff > *b_max ? b_diff : *b_max;                                                                   \
                    printf("0diff: %d %d %d ", r_diff, g_diff, b_diff);                                                           \
                }                                                                                                                 \
            }                                                                                                                     \
        }                                                                                                                         \
        esp_imgfx_color_convert_close(handle);                                                                                    \
        free(in_image.data);                                                                                                      \
        free(out_image.data);                                                                                                     \
    }

#define DEFINE_CC_FUNC1(func_name, get_img_func, diff_func, in_fmt, out_fmt)                                                \
    void func_name(int16_t *r_max, int16_t *g_max, int16_t *b_max)                                                          \
    {                                                                                                                       \
        *r_max = 0;                                                                                                         \
        *g_max = 0;                                                                                                         \
        *b_max = 0;                                                                                                         \
        uint8_t *rgb_image = NULL;                                                                                          \
        esp_imgfx_data_t in_image = {0};                                                                                    \
        esp_imgfx_data_t out_image = {0};                                                                                   \
        int16_t width = 4;                                                                                                  \
        int16_t height = 4;                                                                                                 \
        int16_t r = 0, g = 0, b = 0;                                                                                        \
        int16_t r_diff = 0, g_diff = 0, b_diff = 0;                                                                         \
        esp_imgfx_color_convert_handle_t handle = NULL;                                                                     \
        esp_imgfx_color_convert_cfg_t cfg = {                                                                               \
            .in_res.width = width,                                                                                          \
            .in_res.height = height,                                                                                        \
            .in_pixel_fmt = in_fmt,                                                                                         \
            .out_pixel_fmt = out_fmt,                                                                                       \
            .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT709,                                                             \
        };                                                                                                                  \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_open(&cfg, &handle));                                   \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len));   \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.out_pixel_fmt, &cfg.in_res, &out_image.data_len)); \
        in_image.data = (uint8_t *)malloc(in_image.data_len);                                                               \
        TEST_ASSERT_NOT_NULL(in_image.data);                                                                                \
        out_image.data = (uint8_t *)malloc(out_image.data_len);                                                             \
        TEST_ASSERT_NOT_NULL(out_image.data);                                                                               \
        for (r = 0; r < 240; r++) {                                                                                         \
            for (g = 0; g < 240; g++) {                                                                                     \
                for (b = 0; b < 240; b++) {                                                                                 \
                    get_img_func(cfg.in_res.width, cfg.in_res.height, r, g, b, rgb_image, in_image.data);                   \
                    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_process(handle, &in_image, &out_image));    \
                    diff_func(rgb_image, out_image.data, cfg.in_res.width, cfg.in_res.height, &r_diff, &g_diff, &b_diff);   \
                    *r_max = r_diff > *r_max ? r_diff : *r_max;                                                             \
                    *g_max = g_diff > *g_max ? g_diff : *g_max;                                                             \
                    *b_max = b_diff > *b_max ? b_diff : *b_max;                                                             \
                }                                                                                                           \
            }                                                                                                               \
        }                                                                                                                   \
        esp_imgfx_color_convert_close(handle);                                                                              \
        free(in_image.data);                                                                                                \
        free(out_image.data);                                                                                               \
        free(rgb_image);                                                                                                    \
    }

#define DEFINE_CC_FUNC2(func_name, get_img_func, diff_func, in_fmt, out_fmt)                                                \
    void func_name(uint8_t *rgb_image, int32_t width, int32_t height, int16_t *r_max, int16_t *g_max, int16_t *b_max)       \
    {                                                                                                                       \
        *r_max = 0;                                                                                                         \
        *g_max = 0;                                                                                                         \
        *b_max = 0;                                                                                                         \
        esp_imgfx_data_t in_image = {0};                                                                                    \
        esp_imgfx_data_t out_image = {0};                                                                                   \
        int16_t r_diff = 0, g_diff = 0, b_diff = 0;                                                                         \
        esp_imgfx_color_convert_handle_t handle = NULL;                                                                     \
        esp_imgfx_color_convert_cfg_t cfg = {                                                                               \
            .in_res.width = width,                                                                                          \
            .in_res.height = height,                                                                                        \
            .in_pixel_fmt = in_fmt,                                                                                         \
            .out_pixel_fmt = out_fmt,                                                                                       \
            .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT709,                                                             \
        };                                                                                                                  \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_open(&cfg, &handle));                                   \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len));   \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.out_pixel_fmt, &cfg.in_res, &out_image.data_len)); \
        in_image.data = (uint8_t *)malloc(in_image.data_len);                                                               \
        TEST_ASSERT_NOT_NULL(in_image.data);                                                                                \
        out_image.data = (uint8_t *)malloc(out_image.data_len);                                                             \
        TEST_ASSERT_NOT_NULL(out_image.data);                                                                               \
        get_img_func(cfg.in_res.width, cfg.in_res.height, rgb_image, in_image);                                             \
        TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_color_convert_process(handle, &in_image, &out_image));                \
        diff_func(rgb_image, out_image.data, cfg.in_res.width, cfg.in_res.height, &r_diff, &g_diff, &b_diff);               \
        *r_max = r_diff > *r_max ? r_diff : *r_max;                                                                         \
        *g_max = g_diff > *g_max ? g_diff : *g_max;                                                                         \
        *b_max = b_diff > *b_max ? b_diff : *b_max;                                                                         \
        esp_imgfx_color_convert_close(handle);                                                                              \
        free(in_image.data);                                                                                                \
        free(out_image.data);                                                                                               \
    }

DEFINE_CC_FUNC0(yuv444packet_rgb888_pure, rgb888_image_diff_pixel, ESP_IMGFX_PIXEL_FMT_RGB888);
DEFINE_CC_FUNC0(yuv444packet_rgb565_le_pure, rgb888_rgb565le_image_diff_pixel, ESP_IMGFX_PIXEL_FMT_RGB565_LE);

esp_imgfx_err_t img_cc(esp_imgfx_data_t in_image, esp_imgfx_color_convert_cfg_t *cfg, esp_imgfx_data_t out_image)
{
    esp_imgfx_color_convert_handle_t handle = NULL;
    esp_imgfx_err_t ret = ESP_IMGFX_ERR_OK;
    ret = esp_imgfx_color_convert_open(cfg, &handle);
    if (ret != ESP_IMGFX_ERR_OK) {
        return ret;
    }
    float start = esp_timer_get_time() / 1000.0;
    ret = esp_imgfx_color_convert_process(handle, &in_image, &out_image);
    if (ret != ESP_IMGFX_ERR_OK) {
        return ret;
    }
    float stop = esp_timer_get_time() / 1000.0;
    float sum_enc = (stop - start);
    printf("width:%d height:%d infmt:%d outfmt:%d time:%.4f fps:%.4f \n", cfg->in_res.width, cfg->in_res.height, cfg->in_pixel_fmt, cfg->out_pixel_fmt, sum_enc, 1000 / sum_enc);

    return esp_imgfx_color_convert_close(handle);
}

void print_cfg(esp_imgfx_color_convert_cfg_t *cfg)
{
    printf("cfg.in_res.width: %d \n", cfg->in_res.width);
    printf("cfg.in_res.height: %d \n", cfg->in_res.height);
    printf("cfg.in_pixel_fmt: %d \n", cfg->in_pixel_fmt);
    printf("cfg.out_pixel_fmt: %d \n", cfg->out_pixel_fmt);
    printf("cfg.color_space_std: %d \n", cfg->color_space_std);
}
extern uint32_t pixel_fmt[15];
void img_cc_process()
{
    esp_imgfx_color_convert_cfg_t cfg
        = {
            .in_res.width = 1920,
            .in_res.height = 1080,
            .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
            .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
            .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT709,
        };
    FILE *in_file = NULL;
    FILE *out_file = NULL;
    char filename[50];
    esp_imgfx_err_t ret = ESP_IMGFX_ERR_OK;
    esp_imgfx_color_convert_handle_t handle = NULL;

    esp_imgfx_data_t in_image = {0};
    esp_imgfx_data_t out_image = {0};

    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len));
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, esp_imgfx_get_image_size(cfg.out_pixel_fmt, &cfg.in_res, &out_image.data_len));
    in_image.data = (uint8_t *)malloc(in_image.data_len);
    if (in_image.data == NULL) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }
    out_image.data = (uint8_t *)malloc(out_image.data_len);
    if (out_image.data == NULL) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    ret = esp_imgfx_color_convert_open(&cfg, &handle);
    if (ret != ESP_IMGFX_ERR_OK) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    for (int in_fmt = 0; in_fmt <= 12; in_fmt++) {
        sprintf(filename, "/sdcard/%d_9_1.raw", in_fmt);
        // printf("filename: %s\n", filename);
        in_file = fopen(filename, "rb");
        if (in_file == NULL) {
            printf("%s:%d\n", __func__, __LINE__);
            continue;
            // goto __exit;
        }
        for (int out_fmt = 0; out_fmt <= 12; out_fmt++) {
            // cfg.in_res.width = width,
            // cfg.in_res.height = height,
            cfg.in_pixel_fmt = pixel_fmt[in_fmt];
            cfg.out_pixel_fmt = pixel_fmt[out_fmt];
            ret = esp_imgfx_color_convert_set_cfg(handle, &cfg);
            if (ret != ESP_IMGFX_ERR_OK) {
                print_cfg(&cfg);
                printf("%s:%d\n", __func__, __LINE__);
                continue;
                // goto __exit;
            }
            esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len);
            esp_imgfx_get_image_size(cfg.out_pixel_fmt, &cfg.in_res, &out_image.data_len);
            // print_cfg(&cfg);
            int fret = fread(in_image.data, 1, in_image.data_len, in_file);
            if (fret != in_image.data_len) {
                printf("%s:%d\n", __func__, __LINE__);
                goto __exit;
            }

            float start = esp_timer_get_time() / 1000.0;
            ret = esp_imgfx_color_convert_process(handle, &in_image, &out_image);
            float stop = esp_timer_get_time() / 1000.0;
            float sum_enc = (stop - start);
            printf("width:%d height:%d infmt:%d outfmt:%d time:%.4f fps:%.4f \n", cfg.in_res.width, cfg.in_res.height, cfg.in_pixel_fmt, cfg.out_pixel_fmt, sum_enc, 1000 / sum_enc);

            if (ret == ESP_IMGFX_ERR_OK) {
                sprintf(filename, "/sdcard/asmn%d_%d_%d.raw", cfg.out_pixel_fmt, cfg.in_pixel_fmt, cfg.color_space_std);
                // printf("out filename: %s\n", filename);
                out_file = fopen(filename, "wb");
                if (out_file == NULL) {
                    perror("out file name:");
                    printf("%s: file name %s %d\n", __func__, filename, __LINE__);
                    goto __exit;
                }
                fret = fwrite(out_image.data, 1, out_image.data_len, out_file);
                if (fret != out_image.data_len) {
                    printf("%s:%d\n", __func__, __LINE__);
                    goto __exit;
                }
                fclose(out_file);
                out_file = NULL;
            }
            fseek(in_file, 0, SEEK_SET);
        }
        fclose(in_file);
        in_file = NULL;
    }

__exit:
    if (in_image.data) {
        free(in_image.data);
    }
    if (out_image.data) {
        free(out_image.data);
    }
    if (in_file) {
        fclose(in_file);
    }
    if (out_file) {
        fclose(out_file);
    }
    esp_imgfx_color_convert_close(handle);
}
