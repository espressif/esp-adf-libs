#include "img_crop.h"
#include "esp_imgfx_crop.h"

#include "unity.h"
#include "esp_imgfx_crop.h"

void test_esp_imgfx_crop_open(void)
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {320, 240},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_crop_close(handle);
}

void test_esp_imgfx_crop_process()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {50, 50},
        .x_pos = 25,
        .y_pos = 25};
    esp_imgfx_crop_handle_t handle;
    uint8_t *input_buf = NULL;
    uint8_t *output_buf = NULL;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&input_buf, 128, 100 * 100 * 3));
    TEST_ASSERT_EQUAL(0, posix_memalign((void **)&output_buf, 128, 50 * 50 * 3));
    esp_imgfx_data_t in = {.data = input_buf, .data_len = 100 * 100 * 3};
    esp_imgfx_data_t out = {.data = output_buf, .data_len = 50 * 50 * 3};
    ret = esp_imgfx_crop_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_crop_close(handle);
    free(input_buf);
    free(output_buf);
}

void test_esp_imgfx_crop_open_invalid_cfg()
{
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(NULL, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_crop_open_not_supported_pixel_fmt()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = (esp_imgfx_pixel_fmt_t)255,  // Invalid pixel format
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_NOT_SUPPORTED, ret);
}

void test_esp_imgfx_crop_get_cfg_invalid_handle()
{
    esp_imgfx_crop_cfg_t cfg;
    esp_imgfx_err_t ret = esp_imgfx_crop_get_cfg(NULL, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_crop_get_cfg()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    ret = esp_imgfx_crop_get_cfg(handle, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_crop_close(handle);
}

void test_esp_imgfx_crop_set_cfg_invalid_handle()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_err_t ret = esp_imgfx_crop_set_cfg(NULL, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_crop_set_cfg()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    cfg.cropped_res.width = 50;
    ret = esp_imgfx_crop_set_cfg(handle, &cfg);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_crop_close(handle);
}

void test_esp_imgfx_crop_process_invalid_handle()
{
    esp_imgfx_data_t in = {.data = NULL, .data_len = 0};
    esp_imgfx_data_t out = {.data = NULL, .data_len = 0};
    esp_imgfx_err_t ret = esp_imgfx_crop_process(NULL, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
}

void test_esp_imgfx_crop_process_insufficient_input_data()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_data_t in = {.data = NULL, .data_len = 0};
    esp_imgfx_data_t out = {.data = NULL, .data_len = 0};
    ret = esp_imgfx_crop_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    esp_imgfx_crop_close(handle);
}

void test_esp_imgfx_crop_process_insufficient_output_buffer()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    esp_imgfx_data_t in = {.data = NULL, .data_len = 100 * 100 * 3};  // 100x100 RGB888
    esp_imgfx_data_t out = {.data = NULL, .data_len = 50 * 50 * 3};   // 50x50 RGB888
    ret = esp_imgfx_crop_process(handle, &in, &out);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_INVALID_PARAMETER, ret);
    esp_imgfx_crop_close(handle);
}

void test_esp_imgfx_crop_close_invalid_handle()
{
    esp_imgfx_err_t ret = esp_imgfx_crop_close(NULL);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
}

void test_esp_imgfx_crop_close()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {100, 100},
        .x_pos = 0,
        .y_pos = 0};
    esp_imgfx_crop_handle_t handle;
    esp_imgfx_err_t ret = esp_imgfx_crop_open(&cfg, &handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
    ret = esp_imgfx_crop_close(handle);
    TEST_ASSERT_EQUAL(ESP_IMGFX_ERR_OK, ret);
}

void print_crop_cfg(esp_imgfx_crop_cfg_t *cfg)
{
    printf("cfg.in_res.width: %d \n", cfg->in_res.width);
    printf("cfg.in_res.height: %d \n", cfg->in_res.height);
    printf("cfg.in_pixel_fmt: %d \n", cfg->in_pixel_fmt);
    printf("cfg.cropped_res.width: %d \n", cfg->cropped_res.width);
    printf("cfg.cropped_res.height: %d \n", cfg->cropped_res.height);
    printf("cfg.x_pos: %d \n", cfg->x_pos);
    printf("cfg.y_pos: %d \n", cfg->y_pos);
}
extern uint32_t pixel_fmt[15];
void img_crop_process()
{
    esp_imgfx_crop_cfg_t cfg = {
        .in_res.width = 1920,
        .in_res.height = 1080,
        .cropped_res.width = 5,
        .cropped_res.height = 4,
        .x_pos = 0,
        .y_pos = 0,
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
    };
    esp_imgfx_data_t out_image = {0};
    FILE *in_file = NULL;
    FILE *out_file = NULL;
    char filename[50];
    esp_imgfx_err_t ret = ESP_IMGFX_ERR_OK;
    int fret = 0;
    esp_imgfx_crop_handle_t handle = NULL;
    esp_imgfx_data_t in_image = {0};

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
    ret = esp_imgfx_crop_open(&cfg, &handle);
    if (ret != ESP_IMGFX_ERR_OK) {
        printf("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    for (uint32_t in_fmt = 3; in_fmt <= 3; in_fmt++) {
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
            printf("%s:%d %d %ld\n", __func__, __LINE__, fret, in_image.data_len);
            goto __exit;
        }

        for (int w = cfg.in_res.width / 8; w <= cfg.in_res.width; w += (cfg.in_res.width / 8)) {
            for (int h = cfg.in_res.height / 8; h <= cfg.in_res.height; h += (cfg.in_res.height / 8)) {
                // cfg.in_res.width = width,
                // cfg.in_res.height = height,
                cfg.cropped_res.width = w;
                cfg.cropped_res.height = h;
                cfg.x_pos = 0;
                cfg.y_pos = 0;
                ret = esp_imgfx_crop_set_cfg(handle, &cfg);
                if (ret != ESP_IMGFX_ERR_OK) {
                    printf("%s:%d\n", __func__, __LINE__);
                    continue;
                    // goto __exit;
                }

                esp_imgfx_get_image_size(cfg.in_pixel_fmt, &cfg.cropped_res, &out_image.data_len);
                // print_crop_cfg(&cfg);
                float start = esp_timer_get_time() / 1000.0;
                ret = esp_imgfx_crop_process(handle, &in_image, &out_image);
                float stop = esp_timer_get_time() / 1000.0;
                float sum_enc = (stop - start);
                printf("width:%d height:%d infmt:%d crop width:%d crop height:%d time:%.4f fps:%.4f \n", cfg.in_res.width, cfg.in_res.height, cfg.in_pixel_fmt, cfg.cropped_res.width, cfg.cropped_res.height, sum_enc, 1000 / sum_enc);
                if (ret == ESP_IMGFX_ERR_OK) {
                    sprintf(filename, "/sdcard/res%dx%d_%d_%d_%d.raw", cfg.cropped_res.width, cfg.cropped_res.height, cfg.x_pos, cfg.y_pos, cfg.in_pixel_fmt);
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
    esp_imgfx_crop_close(handle);
}
