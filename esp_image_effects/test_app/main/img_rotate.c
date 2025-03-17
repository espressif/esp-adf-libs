#include "img_rotate.h"
#include "esp_imgfx_rotate.h"

void test_esp_imgfx_rotate_open(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 1920, .height = 1080},
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_rotate_close(handle);
}

void test_esp_imgfx_rotate_process(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 1920, .height = 1080},
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    // 初始化图像数据
    esp_imgfx_data_t in_image = {
        .data_len = 1920 * 1080 * 3};
    esp_imgfx_data_t out_image = {
        .data_len = 1920 * 1080 * 3};
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&in_image.data, 128, in_image.data_len));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&out_image.data, 128, out_image.data_len));
    ret = esp_imgfx_rotate_process(handle, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    free(in_image.data);
    free(out_image.data);
    esp_imgfx_rotate_close(handle);
}

void test_esp_imgfx_rotate_open_invalid_pixel_format(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = (esp_imgfx_pixel_fmt_t)999,  // Invalid pixel format
        .in_res = {.width = 1920, .height = 1080},
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_NOT_SUPPORTED, ret);
}

void test_esp_imgfx_rotate_open_invalid_resolution(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 0, .height = 0},  // Invalid resolution
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_rotate_process_invalid_handle(void)
{
    esp_imgfx_data_t in_image = {
        .data = (uint8_t *)malloc(1920 * 1080 * 3),  // RGB888 image
        .data_len = 1920 * 1080 * 3};
    esp_imgfx_data_t out_image = {
        .data = (uint8_t *)malloc(1080 * 1920 * 3),  // Rotated image
        .data_len = 1080 * 1920 * 3};

    esp_imgfx_err_t ret = esp_imgfx_rotate_process(NULL, &in_image, &out_image);  // Invalid handle
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);

    free(in_image.data);
    free(out_image.data);
}

void test_esp_imgfx_rotate_process_invalid_image_length(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 1920, .height = 1080},
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    esp_imgfx_data_t in_image = {
        .data = (uint8_t *)malloc(1920 * 1080 * 3),  // RGB888 image
        .data_len = 1920 * 1080 * 2                  // Invalid length
    };
    esp_imgfx_data_t out_image = {
        .data = (uint8_t *)malloc(1080 * 1920 * 3),  // Rotated image
        .data_len = 1080 * 1920 * 3};

    ret = esp_imgfx_rotate_process(handle, &in_image, &out_image);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_DATA_LACK, ret);

    free(in_image.data);
    free(out_image.data);
    esp_imgfx_rotate_close(handle);
}

void test_esp_imgfx_rotate_get_resolution(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 1920, .height = 1080},
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);

    esp_imgfx_resolution_t res;
    ret = esp_imgfx_rotate_get_rotated_resolution(handle, &res);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    TEST_ASSERT_EQUAL(1080, res.width);
    TEST_ASSERT_EQUAL(1920, res.height);

    esp_imgfx_rotate_close(handle);
}

void test_esp_imgfx_rotate_get_resolution_invalid_handle(void)
{
    esp_imgfx_resolution_t res;
    esp_imgfx_err_t ret = esp_imgfx_rotate_get_rotated_resolution(NULL, &res);  // Invalid handle
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_rotate_set_cfg(void)
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .degree = 90};
    esp_imgfx_rotate_handle_t handle;
    esp_imgfx_err_t err = esp_imgfx_rotate_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, err);

    esp_imgfx_rotate_cfg_t new_cfg = {
        .in_res = {800, 600},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .degree = 180};
    err = esp_imgfx_rotate_set_cfg(handle, &new_cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, err);

    esp_imgfx_rotate_cfg_t cfg_out;
    err = esp_imgfx_rotate_get_cfg(handle, &cfg_out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, err);
    TEST_ASSERT_EQUAL(new_cfg.in_res.width, cfg_out.in_res.width);
    TEST_ASSERT_EQUAL(new_cfg.in_res.height, cfg_out.in_res.height);
    TEST_ASSERT_EQUAL(new_cfg.in_pixel_fmt, cfg_out.in_pixel_fmt);
    TEST_ASSERT_EQUAL(new_cfg.degree, cfg_out.degree);

    esp_imgfx_rotate_close(handle);
}

void print_rotate_cfg(esp_imgfx_rotate_cfg_t *cfg)
{
    printf("cfg.in_res.width: %d \n", cfg->in_res.width);
    printf("cfg.in_res.height: %d \n", cfg->in_res.height);
    printf("cfg.in_pixel_fmt: %d \n", cfg->in_pixel_fmt);
    printf("cfg.degree: %d \n", cfg->degree);
}
extern uint32_t pixel_fmt[15];
void img_rotate_process()
{
    esp_imgfx_rotate_cfg_t cfg = {
        .in_res.width = 1920,
        .in_res.height = 1024,
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .degree = 90,
    };
    esp_imgfx_data_t out_image = {0};
    FILE *in_file = NULL;
    FILE *out_file = NULL;
    char filename[50];
    esp_imgfx_err_t ret = ESP_IMGFX_ERR_OK;
    int fret = 0;
    esp_imgfx_data_t in_image = {0};
    esp_imgfx_rotate_handle_t handle = NULL;
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
    ret = esp_imgfx_rotate_open(&cfg, &handle);
    if (ret != ESP_IMGFX_ERR_OK) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }
    for (int in_fmt = 3; in_fmt <= 3; in_fmt++) {
        cfg.in_pixel_fmt = pixel_fmt[in_fmt];
        // sprintf(filename, "/sdcard/gradient_1920x1088_rgb565_le.bin");
        sprintf(filename, "/sdcard/%d_9_1.raw", in_fmt);
        printf("filename: %s\n", filename);
        in_file = fopen(filename, "rb");
        if (in_file == NULL) {
            printf("%s:%d\n", __func__, __LINE__);
            continue;
        }
        esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.in_res, &in_image.data_len);
        fret = fread(in_image.data, 1, in_image.data_len, in_file);
        if (fret != in_image.data_len) {
            printf("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }
        for (uint32_t degree = 0; degree <= 360; degree += 90) {
            // cfg.in_res.width = width,
            // cfg.in_res.height = height,

            cfg.degree = 180;  // degree;
            ret = esp_imgfx_rotate_set_cfg(handle, &cfg);
            if (ret != ESP_IMGFX_ERR_OK) {
                printf("%s:%d\n", __func__, __LINE__);
                continue;
                // goto __exit;
            }
            esp_imgfx_resolution_t res;
            esp_imgfx_rotate_get_rotated_resolution(handle, &res);
            printf("rotate res: %d %d  ", res.width, res.height);
            esp_imgfx_get_image_size(cfg.in_pixel_fmt, &res, &out_image.data_len);
            // print_rotate_cfg(&cfg);
            float start = esp_timer_get_time() / 1000.0;
            ret = esp_imgfx_rotate_process(handle, &in_image, &out_image);
            float stop = esp_timer_get_time() / 1000.0;
            float sum_enc = (stop - start);
            printf("width:%d height:%d infmt:%d degree:%d time:%.4f fps:%.4f \n", cfg.in_res.width, cfg.in_res.height, cfg.in_pixel_fmt, cfg.degree, sum_enc, 1000 / sum_enc);
            int *tmp = (int *)out_image.data;
            printf("%d %d %d %d\n", tmp[0], tmp[1], tmp[2], tmp[3]);
            if (ret == ESP_IMGFX_ERR_OK) {
                sprintf(filename, "/sdcard/cr%ld_%d.raw", degree, cfg.in_pixel_fmt);
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
    esp_imgfx_rotate_close(handle);
}
