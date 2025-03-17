#include "img_scale.h"
#include "esp_imgfx_scale.h"

void test_esp_imgfx_scale_open(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_process(void)
{
    // 初始化缩放配置
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    // 初始化图像数据
    esp_imgfx_data_t in_image = {
        .data_len = 640 * 480 * 3};
    esp_imgfx_data_t out_image = {
        .data_len = 320 * 240 * 3};
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&in_image.data, 128, in_image.data_len));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&out_image.data, 128, out_image.data_len));
    // 初始化输入图像数据
    ret = esp_imgfx_scale_process(handle, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    free(in_image.data);
    free(out_image.data);
    // 关闭缩放句柄
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_close(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    ret = esp_imgfx_scale_close(handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
}

void test_esp_imgfx_scale_get_cfg(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_scale_cfg_t cfg_out;
    ret = esp_imgfx_scale_get_cfg(handle, &cfg_out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    TEST_ASSERT_EQUAL(cfg.in_pixel_fmt, cfg_out.in_pixel_fmt);
    TEST_ASSERT_EQUAL(cfg.in_res.width, cfg_out.in_res.width);
    TEST_ASSERT_EQUAL(cfg.in_res.height, cfg_out.in_res.height);
    TEST_ASSERT_EQUAL(cfg.scale_res.width, cfg_out.scale_res.width);
    TEST_ASSERT_EQUAL(cfg.scale_res.height, cfg_out.scale_res.height);
    TEST_ASSERT_EQUAL(cfg.filter_type, cfg_out.filter_type);
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_set_cfg(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_scale_cfg_t new_cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {640, 480},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};

    ret = esp_imgfx_scale_set_cfg(handle, &new_cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_scale_cfg_t cfg_out;
    ret = esp_imgfx_scale_get_cfg(handle, &cfg_out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    TEST_ASSERT_EQUAL(new_cfg.in_pixel_fmt, cfg_out.in_pixel_fmt);
    TEST_ASSERT_EQUAL(new_cfg.in_res.width, cfg_out.in_res.width);
    TEST_ASSERT_EQUAL(new_cfg.in_res.height, cfg_out.in_res.height);
    TEST_ASSERT_EQUAL(new_cfg.scale_res.width, cfg_out.scale_res.width);
    TEST_ASSERT_EQUAL(new_cfg.scale_res.height, cfg_out.scale_res.height);
    TEST_ASSERT_EQUAL(new_cfg.filter_type, cfg_out.filter_type);
    esp_imgfx_scale_close(handle);
}
void test_esp_imgfx_scale_open_with_invalid_config(void)
{
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(NULL, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_scale_process_with_invalid_handle(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_data_t in_image = {
        .data = (uint8_t *)malloc(640 * 480 * 3),
        .data_len = 640 * 480 * 3};
    esp_imgfx_data_t out_image = {
        .data = (uint8_t *)malloc(320 * 240 * 3),
        .data_len = 320 * 240 * 3};
    ret = esp_imgfx_scale_process(NULL, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    free(in_image.data);
    free(out_image.data);
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_process_with_invalid_input_image(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_data_t in_image = {
        .data = NULL,
        .data_len = 640 * 480 * 3};
    esp_imgfx_data_t out_image = {
        .data = (uint8_t *)malloc(320 * 240 * 3),
        .data_len = 320 * 240 * 3};
    ret = esp_imgfx_scale_process(handle, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    free(out_image.data);
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_process_with_invalid_output_image(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_data_t in_image = {
        .data = (uint8_t *)malloc(640 * 480 * 3),
        .data_len = 640 * 480 * 3};
    esp_imgfx_data_t out_image = {
        .data = NULL,
        .data_len = 320 * 240 * 3};
    ret = esp_imgfx_scale_process(handle, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    free(in_image.data);
    esp_imgfx_scale_close(handle);
}

void test_esp_imgfx_scale_close_with_invalid_handle(void)
{
    esp_imgfx_err_t ret = esp_imgfx_scale_close(NULL);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
}

void test_esp_imgfx_scale_get_cfg_with_invalid_handle(void)
{
    esp_imgfx_scale_cfg_t cfg_out;
    esp_imgfx_err_t ret = esp_imgfx_scale_get_cfg(NULL, &cfg_out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_scale_set_cfg_with_invalid_handle(void)
{
    esp_imgfx_scale_cfg_t new_cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {640, 480},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_err_t ret = esp_imgfx_scale_set_cfg(NULL, &new_cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_scale_set_cfg_with_invalid_config(void)
{
    esp_imgfx_scale_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    esp_imgfx_scale_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_scale_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    ret = esp_imgfx_scale_set_cfg(handle, NULL);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    esp_imgfx_scale_close(handle);
}

// 打印缩放配置
void print_scale_cfg(esp_imgfx_scale_cfg_t *cfg)
{
    printf("cfg.in_res.width: %d \n", cfg->in_res.width);
    printf("cfg.in_res.height: %d \n", cfg->in_res.height);
    printf("cfg.in_pixel_fmt: %d \n", cfg->in_pixel_fmt);
    printf("cfg.scale_res.width: %d \n", cfg->scale_res.width);
    printf("cfg.scale_res.height: %d \n", cfg->scale_res.height);
    printf("cfg.filter_type: %d \n", cfg->filter_type);
}

extern uint32_t pixel_fmt[15];
// 缩放处理
void img_scale_process()
{
    uint32_t in_fmt;
    esp_imgfx_scale_cfg_t cfg = {
        .in_res.width = 1920,
        .in_res.height = 1080,
        .scale_res.width = 4,
        .scale_res.height = 5,
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
    };
    esp_imgfx_data_t out_image = {0};
    FILE *in_file = NULL;
    FILE *out_file = NULL;
    char filename[50];
    esp_imgfx_err_t ret = ESP_IMGFX_ERR_OK;
    int fret = 0;
    esp_imgfx_data_t in_image = {0};
    esp_imgfx_scale_handle_t handle = NULL;
    esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len);
    int mret = posix_memalign((void **)&in_image.data, 128, in_image.data_len);
    if (mret != 0) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }
    out_image.data_len = in_image.data_len;
    mret = posix_memalign((void **)&out_image.data, 128, out_image.data_len);
    if (mret != 0) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }
    printf("in_image: %p out_image: %p\n", in_image.data, out_image.data);

    ret = esp_imgfx_scale_open(&cfg, &handle);
    if (ret != ESP_IMGFX_ERR_OK) {
        goto __exit;
    }
    for (in_fmt = 3; in_fmt <= 3; in_fmt++) {
        cfg.in_pixel_fmt = pixel_fmt[in_fmt];
        // sprintf(filename, "/sdcard/gradient_1920x1088_rgb565_le.bin");
        sprintf(filename, "/sdcard/c%ld_9_1.raw", in_fmt);
        printf("filename: %s\n", filename);
        in_file = fopen(filename, "rb");
        if (in_file == NULL) {
            printf("%s:%d\n", __func__, __LINE__);
            continue;
            // goto __exit;
        }
        esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len);
        fret = fread(in_image.data, 1, in_image.data_len, in_file);
        if (fret != in_image.data_len) {
            printf("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }
        for (int w = cfg.in_res.width / 8; w <= cfg.in_res.width; w += (cfg.in_res.width / 8)) {
            for (int h = cfg.in_res.height / 8; h <= cfg.in_res.height; h += (cfg.in_res.height / 8)) {
                // cfg.in_res.width = width,
                // cfg.in_res.height = height,

                cfg.scale_res.width = w;
                cfg.scale_res.height = h;
                cfg.filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR,
                ret = esp_imgfx_scale_set_cfg(handle, &cfg);
                if (ret != ESP_IMGFX_ERR_OK) {
                    printf("%s:%d\n", __func__, __LINE__);
                    continue;
                    // goto __exit;
                }

                esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.scale_res, &out_image.data_len);
                // print_scale_cfg(&cfg);
                float start = esp_timer_get_time() / 1000.0;
                ret = esp_imgfx_scale_process(handle, &in_image, &out_image);
                float stop = esp_timer_get_time() / 1000.0;
                float sum_enc = (stop - start);
                printf("width:%d height:%d infmt:%d scale width:%d scale height:%d filter:%d time:%.4f fps:%.4f \n", cfg.in_res.width, cfg.in_res.height, cfg.in_pixel_fmt, cfg.scale_res.width, cfg.scale_res.height, cfg.filter_type, sum_enc, 1000 / sum_enc);
                if (ret == ESP_IMGFX_ERR_OK) {
                    sprintf(filename, "/sdcard/Bres%dx%d_%d_%d.raw", cfg.scale_res.width, cfg.scale_res.height, cfg.filter_type, cfg.in_pixel_fmt);
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
    esp_imgfx_scale_close(handle);
}
