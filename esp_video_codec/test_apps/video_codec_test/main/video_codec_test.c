/**
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <math.h>
#include <stdint.h>
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_video_enc.h"
#include "esp_video_dec.h"
#include "video_codec_test_utils.h"
#include "esp_video_dec_reg.h"
#include "esp_video_enc_reg.h"
#include "esp_video_codec_utils.h"
#include "esp_video_enc_default.h"
#include "esp_video_dec_default.h"
#include "esp_video_codec_version.h"
#include "unity.h"
#include "esp_log.h"

#define TAG "VIDEO_CODEC_TEST"

#define TEST_PATTERN_IS_VERTICAL (false)
#define TEST_PATTERN_BAR_COUNT   (8)

#define CODEC_TYPE_NAME(is_hw)   (is_hw ? "HW" : "SW")
#define MIN_RESOLUTION(a, b)     ((a.width * a.height > b.width * b.height) ? &b : &a)

#define TEST_DECLARE_QUERY_BY_DESC(query, desc) \
    esp_video_codec_query_t query = {           \
        .codec_type = (desc)->codec_type,       \
        .codec_cc = (desc)->codec_cc          \
    };

typedef struct {
    esp_video_codec_pixel_fmt_t fmt;
    esp_video_codec_desc_t      desc;
} test_codec_info_t;

video_codec_test_res_t test_res;

static int prepare_test_res(test_codec_info_t *enc_info, test_codec_info_t *dec_info,
                            esp_video_codec_resolution_t *resolution)
{
    test_res.enc_in_fmt = enc_info->fmt;
    test_res.res = *resolution;

    // Allocate memory for encoder input data
    test_res.raw_size = esp_video_codec_get_image_size(enc_info->fmt, resolution);
    esp_video_enc_caps_t caps = { 0 };
    TEST_DECLARE_QUERY_BY_DESC(enc_query, &enc_info->desc);
    esp_video_enc_query_caps(&enc_query, &caps);
    test_res.raw_data = esp_video_codec_align_alloc(caps.in_frame_align, test_res.raw_size, &test_res.raw_size);
    if (test_res.raw_data == NULL) {
        ESP_LOGE(TAG, "Fail to allocate encoder in buffer");
        return -1;
    }
    // Get encoder output size and needed alignment, if decoder output is used by decoder
    // Get maximum alignment of encoder output and decoder input
    uint8_t enc_out_align = caps.out_frame_align;
    esp_video_dec_caps_t dec_caps = { 0 };
    if (dec_info) {
        TEST_DECLARE_QUERY_BY_DESC(dec_query, &dec_info->desc);
        esp_video_dec_query_caps(&dec_query, &dec_caps);
        if (dec_caps.in_frame_align > enc_out_align) {
            enc_out_align = dec_caps.in_frame_align;
        }
    }
    // TODO estimated encoded size here, need change if not enough
    test_res.encoded_size = test_res.raw_size / 4;
    test_res.encoded_data = esp_video_codec_align_alloc(enc_out_align, test_res.encoded_size, &test_res.encoded_size);
    if (test_res.encoded_data == NULL) {
        ESP_LOGE(TAG, "Fail to allocate encoder out buffer");
        return -1;
    }
    // Allocate memory for decoder output
    if (dec_info) {
        test_res.dec_out_fmt = dec_info->fmt;
        test_res.decoded_size = esp_video_codec_get_image_size(dec_info->fmt, resolution);
        test_res.decoded_data = esp_video_codec_align_alloc(dec_caps.out_frame_align, test_res.decoded_size, &test_res.decoded_size);
        if (test_res.decoded_data == NULL) {
            ESP_LOGE(TAG, "Fail to allocate decoder out buffer");
            return -1;
        }
    }
    // Generate test encoded data pattern
    gen_pattern_color_bar(TEST_PATTERN_IS_VERTICAL, TEST_PATTERN_BAR_COUNT);
    return 0;
}

static void release_test_res(void)
{
    if (test_res.raw_data != NULL) {
        esp_video_codec_free(test_res.raw_data);
        test_res.raw_data = NULL;
    }
    if (test_res.encoded_data != NULL) {
        esp_video_codec_free(test_res.encoded_data);
        test_res.encoded_data = NULL;
    }
    if (test_res.decoded_data != NULL) {
        esp_video_codec_free(test_res.decoded_data);
        test_res.decoded_data = NULL;
    }
}

static int video_encoder_test(test_codec_info_t *enc_info, esp_video_codec_resolution_t *resolution, bool one_frame)
{
    ESP_LOGI(TAG, "Encoder:%s-%s Format:%s Resolution:%dx%d",
             esp_video_codec_get_codec_str(enc_info->desc.codec_type), CODEC_TYPE_NAME(enc_info->desc.is_hw),
             esp_video_codec_get_pixel_fmt_str(enc_info->fmt), (int)resolution->width, (int)resolution->height);
    esp_video_enc_cfg_t enc_cfg = {
        .codec_type = enc_info->desc.codec_type,
        .codec_cc = enc_info->desc.codec_cc,
        .resolution = *resolution,
        .in_fmt = enc_info->fmt,
        .fps = 10,
    };
    esp_video_enc_handle_t enc_handle = NULL;
    int ret = esp_video_enc_open(&enc_cfg, &enc_handle);
    if (ret != ESP_VC_ERR_OK || enc_handle == NULL) {
        ESP_LOGE(TAG, "Fail to open encoder");
        return -1;
    }
    uint32_t bitrate = resolution->width * resolution->height * enc_cfg.fps / 10;
    ret = esp_video_enc_set_bitrate(enc_handle, bitrate);
    uint32_t gop_size = enc_cfg.fps * 2;
    ret = esp_video_enc_set_gop(enc_handle, gop_size);
    // Use 422 chroma subsampling when output is YUV422
    if (test_res.dec_out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_YUV422 || test_res.dec_out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422) {
        ret = esp_video_enc_set_chroma_subsampling(enc_handle, ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422);
    }
    uint32_t pts = 0;
    uint32_t delta_pts = 1000 / enc_cfg.fps;
    esp_video_enc_out_frame_t out_frame = {
        .data = test_res.encoded_data,
        .size = test_res.encoded_size,
    };
    int frame_count = one_frame ? 1 : gop_size + 1;
    for (int i = 0; i < frame_count; i++) {
        esp_video_enc_in_frame_t in_frame = {
            .pts = pts,
            .data = test_res.raw_data,
            .size = test_res.raw_size,
        };
        while (in_frame.consumed < in_frame.size) {
            ret = esp_video_enc_process(enc_handle, &in_frame, &out_frame);
            // Allow output decoded size to be 0
            if (ret != ESP_VC_ERR_OK) {
                ESP_LOGE(TAG, "Fail to encode frame");
                break;
            }
            in_frame.data += in_frame.consumed;
            in_frame.size -= in_frame.consumed;
            in_frame.consumed = 0;
            ESP_LOGI(TAG, "Encoded %d PTS:%d frame size: %d", i, (int)pts, (int)out_frame.encoded_size);
        }
        pts += delta_pts;
    }
    esp_video_enc_close(enc_handle);
    return 0;
}

static int video_decoder_test_one_frame(test_codec_info_t *dec_info)
{
    ESP_LOGI(TAG, "Decoder:%s-%s Format:%s",
             esp_video_codec_get_codec_str(dec_info->desc.codec_type), CODEC_TYPE_NAME(dec_info->desc.is_hw),
             esp_video_codec_get_pixel_fmt_str(dec_info->fmt));
    esp_video_dec_cfg_t dec_cfg = {
        .codec_type = dec_info->desc.codec_type,
        .codec_cc = dec_info->desc.codec_cc,
        .out_fmt = dec_info->fmt,
    };
    esp_video_dec_handle_t dec_handle = NULL;
    int ret = esp_video_dec_open(&dec_cfg, &dec_handle);
    if (ret != ESP_VC_ERR_OK || dec_handle == NULL) {
        ESP_LOGE(TAG, "Fail to open decoder");
        return -1;
    }
    esp_video_dec_in_frame_t in_frame = {
        .data = test_res.encoded_data,
        .size = test_res.encoded_size,
    };
    esp_video_dec_out_frame_t out_frame = {
        .data = test_res.decoded_data,
        .size = test_res.decoded_size,
    };
    ret = esp_video_dec_process(dec_handle, &in_frame, &out_frame);
    if (ret != ESP_VC_ERR_OK || out_frame.decoded_size == 0) {
        ESP_LOGE(TAG, "Fail to decode frame ret %d", ret);
        ret = -1;
    }
    esp_video_codec_frame_info_t frame_info = {};
    ret = esp_video_dec_get_frame_info(dec_handle, &frame_info);
    if (ret != ESP_VC_ERR_OK) {
        ESP_LOGE(TAG, "Fail to get frame info");
        ret = -1;
    }
    if (frame_info.res.width != test_res.res.width || frame_info.res.height != test_res.res.height) {
        ESP_LOGE(TAG, "Decoded resolution wrong encoded: %dx%d decoded:%dx%d", (int)test_res.res.width,
                 (int)test_res.res.height, (int)frame_info.res.width,
                 (int)frame_info.res.height);
    }
    esp_video_dec_close(dec_handle);
    return ret;
}

static int video_encoder_to_decoder_one_frame(test_codec_info_t *enc_info, test_codec_info_t *dec_info,
                                              esp_video_codec_resolution_t *resolution)
{
    int ret = prepare_test_res(enc_info, dec_info, resolution);
    do {
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to prepare test res");
            break;
        }
        ret = video_encoder_test(enc_info, resolution, true);
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to encode one frame");
            break;
        }
        ret = video_decoder_test_one_frame(dec_info);
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to decode one frame");
            break;
        }
        draw_decode_result();
    } while (0);
    release_test_res();
    return ret;
}

static int video_encoder_to_decoder_multiple_frame(test_codec_info_t *enc_info, test_codec_info_t *dec_info,
                                                   esp_video_codec_resolution_t *resolution)
{
    esp_video_dec_handle_t dec_handle = NULL;
    esp_video_enc_handle_t enc_handle = NULL;
    ESP_LOGW(TAG, "Start multiple frame test for encoder %s-%s fmt:%s decoder %s-%s fmt:%s",
             esp_video_codec_get_codec_str(enc_info->desc.codec_type), CODEC_TYPE_NAME(enc_info->desc.is_hw),
             esp_video_codec_get_pixel_fmt_str(enc_info->fmt),
             esp_video_codec_get_codec_str(dec_info->desc.codec_type), CODEC_TYPE_NAME(dec_info->desc.is_hw),
             esp_video_codec_get_pixel_fmt_str(dec_info->fmt));
    int ret = prepare_test_res(enc_info, dec_info, resolution);
    do {
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to prepare test res");
            break;
        }
        // Open encoder
        esp_video_enc_cfg_t enc_cfg = {
            .codec_type = enc_info->desc.codec_type,
            .codec_cc = enc_info->desc.codec_cc,
            .resolution = *resolution,
            .in_fmt = enc_info->fmt,
            .fps = 10,
        };
        int ret = esp_video_enc_open(&enc_cfg, &enc_handle);
        if (ret != ESP_VC_ERR_OK || enc_handle == NULL) {
            ESP_LOGE(TAG, "Fail to open encoder");
            break;
        }
        uint32_t bitrate = resolution->width * resolution->height * enc_cfg.fps / 10;
        ret = esp_video_enc_set_bitrate(enc_handle, bitrate);
        uint32_t gop_size = enc_cfg.fps * 2;
        ret = esp_video_enc_set_gop(enc_handle, gop_size);
        if (test_res.dec_out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_YUV422 || test_res.dec_out_fmt == ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422) {
            ret = esp_video_enc_set_chroma_subsampling(enc_handle, ESP_VIDEO_CODEC_CHROMA_SUBSAMPLING_422);
        }
        // Open decoder
        esp_video_dec_cfg_t dec_cfg = {
            .codec_type = dec_info->desc.codec_type,
            .codec_cc = dec_info->desc.codec_cc,
            .out_fmt = dec_info->fmt,
        };
        ret = esp_video_dec_open(&dec_cfg, &dec_handle);
        if (ret != ESP_VC_ERR_OK || dec_handle == NULL) {
            ESP_LOGE(TAG, "Fail to open decoder");
            break;
        }
        uint32_t pts = 0;
        uint32_t delta_pts = 1000 / enc_cfg.fps;
        esp_video_enc_out_frame_t out_frame = {
            .data = test_res.encoded_data,
            .size = test_res.encoded_size,
        };
        int frame_count = gop_size + 1;
        for (int i = 0; i < frame_count; i++) {
            esp_video_enc_in_frame_t in_frame = {
                .pts = pts,
                .data = test_res.raw_data,
                .size = test_res.raw_size,
            };
            while (in_frame.consumed < in_frame.size) {
                ret = esp_video_enc_process(enc_handle, &in_frame, &out_frame);
                // Allow output decoded size to be 0
                if (ret != ESP_VC_ERR_OK) {
                    ESP_LOGE(TAG, "Fail to encode frame");
                    break;
                }
                ESP_LOGI(TAG, "Encoded %d PTS:%d frame size: %d", i, (int)pts, (int)out_frame.encoded_size);
                if (out_frame.encoded_size) {
                    // Do decoder
                    esp_video_dec_in_frame_t dec_in_frame = {
                        .pts = out_frame.pts,
                        .dts = out_frame.dts,
                        .data = out_frame.data,
                        .size = out_frame.encoded_size,
                    };
                    esp_video_dec_out_frame_t dec_out_frame = {
                        .data = test_res.decoded_data,
                        .size = test_res.decoded_size,
                    };
                    while (dec_in_frame.consumed < dec_in_frame.size) {
                        ret = esp_video_dec_process(dec_handle, &dec_in_frame, &dec_out_frame);
                        if (ret != ESP_VC_ERR_OK) {
                            ESP_LOGE(TAG, "Fail to decode frame ret %d", ret);
                            ret = -1;
                            break;
                        }
                        ESP_LOGI(TAG, "Decoded %d PTS:%d frame size: %d", i, (int)dec_out_frame.pts,
                                 (int)dec_out_frame.decoded_size);
                        dec_in_frame.data += dec_in_frame.consumed;
                        dec_in_frame.size -= dec_in_frame.consumed;
                        dec_in_frame.consumed = 0;
                    }
                }
                in_frame.data += in_frame.consumed;
                in_frame.size -= in_frame.consumed;
                in_frame.consumed = 0;
            }
            pts += delta_pts;
        }
    } while (0);
    if (enc_handle) {
        esp_video_enc_close(enc_handle);
    }
    if (dec_handle) {
        esp_video_dec_close(dec_handle);
    }
    release_test_res();
    return ret;
}

TEST_CASE("Register Test", "VIDEO_CODEC_TEST")
{
    int heap_size = esp_get_free_heap_size();
    TEST_ESP_OK(esp_video_dec_register_default());
    uint8_t dec_idx = 0;
    int ret = 0;
    while (1) {
        esp_video_codec_desc_t dec_desc = {};
        ret = esp_video_dec_get_desc(dec_idx, &dec_desc);
        dec_idx++;
        if (ret != ESP_VC_ERR_OK) {
            break;
        }
        ESP_LOGI(TAG, "Decoder:%d codec:%s-%s", dec_idx,
                 esp_video_codec_get_codec_str(dec_desc.codec_type),
                 CODEC_TYPE_NAME(dec_desc.is_hw));

        esp_video_dec_caps_t dec_caps = { 0 };
        TEST_DECLARE_QUERY_BY_DESC(query, &dec_desc);
        esp_video_dec_query_caps(&query, &dec_caps);
        ESP_LOGI(TAG, "Typical resolution:%dx%d fps:%d",
                 (int)dec_caps.typical_res.width, (int)dec_caps.typical_res.height, dec_caps.typical_fps);
        for (int i = 0; i < dec_caps.out_fmt_num; i++) {
            ESP_LOGI(TAG, "Format %d:%s", i, esp_video_codec_get_pixel_fmt_str(dec_caps.out_fmts[i]));
        }
        printf("\n");
    }
    esp_video_dec_unregister_default();
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
    TEST_ESP_OK(esp_video_enc_register_default());

    uint8_t enc_idx = 0;
    while (1) {
        esp_video_codec_desc_t enc_desc = {};
        ret = esp_video_enc_get_desc(enc_idx, &enc_desc);
        enc_idx++;
        if (ret != ESP_VC_ERR_OK) {
            break;
        }
        ESP_LOGI(TAG, "Encoder:%d codec:%s-%s", enc_idx,
                 esp_video_codec_get_codec_str(enc_desc.codec_type),
                 CODEC_TYPE_NAME(enc_desc.is_hw));
        TEST_DECLARE_QUERY_BY_DESC(query, &enc_desc);
        esp_video_enc_caps_t enc_caps = { 0 };
        esp_video_enc_query_caps(&query, &enc_caps);
        ESP_LOGI(TAG, "Typical resolution:%dx%d fps:%d",
                 (int)enc_caps.typical_res.width, (int)enc_caps.typical_res.height, enc_caps.typical_fps);
        for (int i = 0; i < enc_caps.in_fmt_num; i++) {
            ESP_LOGI(TAG, "Format %d:%s", i, esp_video_codec_get_pixel_fmt_str(enc_caps.in_fmts[i]));
        }
        printf("\n");
    }
    esp_video_enc_unregister_default();
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("Encoder max resolution Test", "VIDEO_CODEC_TEST")
{
    int heap_size = esp_get_free_heap_size();
    TEST_ESP_OK(esp_video_enc_register_default());
    uint8_t enc_idx = 0;
    test_codec_info_t enc_info = {};
    while (1) {
        if (esp_video_enc_get_desc(enc_idx, &enc_info.desc) != ESP_VC_ERR_OK) {
            break;
        }
        enc_idx++;
        TEST_DECLARE_QUERY_BY_DESC(query, &enc_info.desc);
        esp_video_enc_caps_t enc_caps = { 0 };
        esp_video_enc_query_caps(&query, &enc_caps);
        if (enc_caps.in_fmt_num == 0) {
            continue;
        }
        // Test all supported input codecs
        for (int i = 0; i < enc_caps.in_fmt_num; i++) {
            enc_info.fmt = enc_caps.in_fmts[i];
            esp_video_codec_resolution_t *resolution = &enc_caps.typical_res;
            int ret = prepare_test_res(&enc_info, NULL, resolution);
            if (ret != 0) {
                ESP_LOGE(TAG, "Fail to prepare");
            }
            ret = video_encoder_test(&enc_info, resolution, false);
            if (ret != 0) {
                ESP_LOGE(TAG, "Fail to encode");
            }
            release_test_res();
        }
    }
    esp_video_enc_unregister_default();
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("One encoder decoder test", "VIDEO_CODEC_TEST")
{
    int heap_size = esp_get_free_heap_size();
    TEST_ESP_OK(esp_video_enc_register_default());
    TEST_ESP_OK(esp_video_dec_register_default());
    esp_video_codec_resolution_t resolution = {
        .width = 320,
        .height = 240,
    };
    test_codec_info_t enc_info = {
        .desc.codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .desc.is_hw = true,
        .fmt = ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE,
    };
    test_codec_info_t dec_info = {
        .desc.codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .desc.is_hw = true,
        .fmt = ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE,
    };
    video_encoder_to_decoder_one_frame(&enc_info, &dec_info, &resolution);
    esp_video_enc_unregister_default();
    esp_video_dec_unregister_default();
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("Encoder to Decoder One Frame Test", "VIDEO_CODEC_TEST")
{
    int heap_size = esp_get_free_heap_size();
    TEST_ESP_OK(esp_video_enc_register_default());
    TEST_ESP_OK(esp_video_dec_register_default());
    uint8_t enc_idx = 0;
    test_codec_info_t enc_info = {};
    while (1) {
        if (esp_video_enc_get_desc(enc_idx, &enc_info.desc) != ESP_VC_ERR_OK) {
            break;
        }
        enc_idx++;
        TEST_DECLARE_QUERY_BY_DESC(query, &enc_info.desc);
        esp_video_enc_caps_t enc_caps = { 0 };
        esp_video_enc_query_caps(&query, &enc_caps);
        TEST_ASSERT_GREATER_THAN(0, enc_caps.in_fmt_num);
        uint8_t dec_idx = 0;
        test_codec_info_t dec_info = {};
        while (1) {
            if (esp_video_dec_get_desc(dec_idx, &dec_info.desc) != ESP_VC_ERR_OK) {
                break;
            }
            dec_idx++;
            if (dec_info.desc.codec_type != enc_info.desc.codec_type) {
                continue;
            }
            esp_video_dec_caps_t dec_caps = { 0 };
             TEST_DECLARE_QUERY_BY_DESC(query, &dec_info.desc);
            esp_video_dec_query_caps(&query, &dec_caps);
            TEST_ASSERT_GREATER_THAN(0, dec_caps.out_fmt_num);
            printf("Start one frame test for encoder %d:%s-%s decoder %d:%s-%s\n",
                   enc_idx - 1, esp_video_codec_get_codec_str(enc_info.desc.codec_type), CODEC_TYPE_NAME(enc_info.desc.is_hw),
                   dec_idx - 1, esp_video_codec_get_codec_str(dec_info.desc.codec_type), CODEC_TYPE_NAME(dec_info.desc.is_hw));
            for (int i = 0; i < enc_caps.in_fmt_num; i++) {
                enc_info.fmt = enc_caps.in_fmts[i];
                for (int j = 0; j < dec_caps.out_fmt_num; j++) {
                    dec_info.fmt = dec_caps.out_fmts[j];
                    esp_video_codec_resolution_t *resolution = MIN_RESOLUTION(dec_caps.typical_res, enc_caps.typical_res);
                    int ret = video_encoder_to_decoder_one_frame(&enc_info, &dec_info, resolution);
                    if (ret != 0) {
                        ESP_LOGE(TAG, "Test failed");
                    }
                }
            }
        }
    }
    esp_video_dec_unregister_default();
    esp_video_enc_unregister_default();
    if (heap_size != (int)esp_get_free_heap_size()) {
        ESP_LOGE(TAG, "Leakage %d", heap_size - (int)esp_get_free_heap_size());
    }
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

TEST_CASE("Encoder to Decoder Multiple Frame Test", "VIDEO_CODEC_TEST")
{
    int heap_size = esp_get_free_heap_size();
    TEST_ESP_OK(esp_video_enc_register_default());
    TEST_ESP_OK(esp_video_dec_register_default());
    uint8_t enc_idx = 0;
    test_codec_info_t enc_info = {};
    test_codec_info_t dec_info = {};
    while (1) {
        if (esp_video_enc_get_desc(enc_idx, &enc_info.desc) != ESP_VC_ERR_OK) {
            break;
        }
        enc_idx++;
        esp_video_enc_caps_t enc_caps = { 0 };
        TEST_DECLARE_QUERY_BY_DESC(query, &enc_info.desc);
        esp_video_enc_query_caps(&query, &enc_caps);
        TEST_ASSERT_GREATER_THAN(0, enc_caps.in_fmt_num);
        uint8_t dec_idx = 0;
        while (1) {
            if (esp_video_dec_get_desc(dec_idx, &dec_info.desc) != ESP_VC_ERR_OK) {
                break;
            }
            dec_idx++;
            if (dec_info.desc.codec_type != enc_info.desc.codec_type) {
                continue;
            }
            esp_video_dec_caps_t dec_caps = { 0 };
             TEST_DECLARE_QUERY_BY_DESC(query, &dec_info.desc);
            esp_video_dec_query_caps(&query, &dec_caps);
            TEST_ASSERT_GREATER_THAN(0, dec_caps.out_fmt_num);

            printf("Start multiple frame test for encoder %d:%s-%s decoder %d:%s-%s\n",
                   enc_idx - 1, esp_video_codec_get_codec_str(enc_info.desc.codec_type), CODEC_TYPE_NAME(enc_info.desc.is_hw),
                   dec_idx - 1, esp_video_codec_get_codec_str(dec_info.desc.codec_type), CODEC_TYPE_NAME(dec_info.desc.is_hw));
            for (int i = 0; i < enc_caps.in_fmt_num; i++) {
                enc_info.fmt = enc_caps.in_fmts[i];
                for (int j = 0; j < dec_caps.out_fmt_num; j++) {
                    dec_info.fmt = dec_caps.out_fmts[j];
                    esp_video_codec_resolution_t *resolution = MIN_RESOLUTION(dec_caps.typical_res, enc_caps.typical_res);
                    int ret = video_encoder_to_decoder_multiple_frame(&enc_info, &dec_info, resolution);
                    if (ret != 0) {
                        ESP_LOGE(TAG, "Test failed");
                    }
                }
            }
        }
    }
    esp_video_dec_unregister_default();
    esp_video_enc_unregister_default();
    TEST_ASSERT_EQUAL_INT(heap_size, (int)esp_get_free_heap_size());
}

static int single_mjpeg_encode_test(void)
{
    // Register default encoders
    esp_video_enc_register_default();

    // Open video encoder
    esp_video_enc_cfg_t enc_cfg = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .resolution = {
            .width = 640,
            .height = 480
        },
#ifdef CONFIG_VIDEO_DECODER_HW_MJPEG_SUPPORT
        .in_fmt = ESP_VIDEO_CODEC_PIXEL_FMT_UYVY422,
#else
        .in_fmt = ESP_VIDEO_CODEC_PIXEL_FMT_RGB888,
#endif
        .fps = 10,
    };
    esp_video_enc_handle_t enc_handle = NULL;
    int ret = esp_video_enc_open(&enc_cfg, &enc_handle);
    if (ret != ESP_VC_ERR_OK) {
        ESP_LOGE(TAG, "Fail to open encoder");
        return -1;
    }
    do {
        // Set video bitrate
        uint32_t bitrate = enc_cfg.resolution.width * enc_cfg.resolution.height * enc_cfg.fps / 10;
        esp_video_enc_set_bitrate(enc_handle, bitrate);

        // Allocate input and output buffer
        uint8_t in_frame_align = 0;
        uint8_t out_frame_align = 0;
        esp_video_enc_get_frame_align(enc_handle, &in_frame_align, &out_frame_align);
        test_res.raw_size = esp_video_codec_get_image_size(enc_cfg.in_fmt, &enc_cfg.resolution);
        test_res.raw_data = esp_video_codec_align_alloc(in_frame_align, test_res.raw_size, &test_res.raw_size);
        if (test_res.raw_data == NULL) {
            ESP_LOGE(TAG, "Fail to allocate encoder in buffer");
            break;
        }
        test_res.encoded_size = test_res.raw_size / 4;
        test_res.encoded_data = esp_video_codec_align_alloc(out_frame_align, test_res.encoded_size, &test_res.encoded_size);
        if (test_res.encoded_data == NULL) {
            ESP_LOGE(TAG, "Fail to allocate encoder out buffer");
            break;
        }

        // Use pattern to fill input frame data
        test_res.res = enc_cfg.resolution;
        test_res.enc_in_fmt = enc_cfg.in_fmt;
        gen_pattern_color_bar(TEST_PATTERN_IS_VERTICAL, TEST_PATTERN_BAR_COUNT);

        // Do encoder process
        esp_video_enc_in_frame_t in_frame = {
            .pts = 0,
            .data = test_res.raw_data,
            .size = test_res.raw_size,
        };
        esp_video_enc_out_frame_t out_frame = {
            .data = test_res.encoded_data,
            .size = test_res.encoded_size,
        };
        // Encode one frame, when have B frame, need loop process until input frame  `consumed` equal to `size`
        ret = esp_video_enc_process(enc_handle, &in_frame, &out_frame);
        if (ret != ESP_VC_ERR_OK) {
            ESP_LOGE(TAG, "Fail to encode frame ret %d", ret);
            break;
        }
        ESP_LOGI(TAG, "Encoded frame size %d", (int)out_frame.encoded_size);
        test_res.encoded_size = out_frame.encoded_size;
    } while (0);

    // Close video encoder
    esp_video_enc_close(enc_handle);

    // Unregister video encoders
    esp_video_enc_unregister_default();
    return ret;
}

static int single_mjpeg_decode_test(void)
{
    // Register video decoder
    esp_video_dec_register_default();

    // Open video decoder
    esp_video_dec_cfg_t dec_cfg = {
        .codec_type = ESP_VIDEO_CODEC_TYPE_MJPEG,
        .out_fmt = ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE,
    };
    esp_video_dec_handle_t dec_handle = NULL;
    int ret = esp_video_dec_open(&dec_cfg, &dec_handle);
    if (ret != ESP_VC_ERR_OK) {
        ESP_LOGE(TAG, "Fail to open decoder");
        return ESP_VC_ERR_OK;
    }

    // Allocate memory for output data according alignment
    uint8_t in_frame_align = 0;
    uint8_t out_frame_align = 0;
    esp_video_dec_get_frame_align(dec_handle, &in_frame_align, &out_frame_align);
    test_res.decoded_size = esp_video_codec_get_image_size(dec_cfg.out_fmt, &test_res.res);
    test_res.decoded_data = esp_video_codec_align_alloc(out_frame_align, test_res.decoded_size, &test_res.decoded_size);
    do {
        if (test_res.decoded_data == NULL) {
            ESP_LOGE(TAG, "Fail to allocate decoder out buffer");
            break;
        }
        // Input data is previous encoded data by `single_mjpeg_encode_test`
        esp_video_dec_in_frame_t in_frame = {
            .data = test_res.encoded_data,
            .size = test_res.encoded_size,
        };
        esp_video_dec_out_frame_t out_frame = {
            .data = test_res.decoded_data,
            .size = test_res.decoded_size,
        };
        esp_video_codec_frame_info_t frame_info = {};
    RETRY:
        // need loop process until input frame `consumed` equal to `size`
        ret = esp_video_dec_process(dec_handle, &in_frame, &out_frame);
        if (ret == ESP_VC_ERR_BUF_NOT_ENOUGH) {
            // When output buffer is not enough, reallocate it and retry
            ret = esp_video_dec_get_frame_info(dec_handle, &frame_info);
            if (ret != ESP_VC_ERR_OK) {
                ESP_LOGE(TAG, "Fail to get frame info");
                break;
            }
            esp_video_codec_free(test_res.decoded_data);
            test_res.decoded_size = esp_video_codec_get_image_size(dec_cfg.out_fmt, &frame_info.res);
            test_res.decoded_data = esp_video_codec_align_alloc(out_frame_align, test_res.decoded_size, &test_res.decoded_size);
            if (test_res.decoded_data == NULL) {
                ESP_LOGE(TAG, "Fail to allocate decoder out buffer");
                break;
            }
            out_frame.size = test_res.decoded_size;
            out_frame.data = test_res.decoded_data;
            goto RETRY;
        }
        if (ret != ESP_VC_ERR_OK || out_frame.decoded_size == 0) {
            ESP_LOGE(TAG, "Fail to decode frame ret %d", ret);
            break;
        }
        // Get frame information after decode
        ret = esp_video_dec_get_frame_info(dec_handle, &frame_info);
        if (ret != ESP_VC_ERR_OK) {
            ESP_LOGE(TAG, "Fail to get frame info");
            break;
        }
        test_res.dec_out_fmt = dec_cfg.out_fmt;
        ESP_LOGI(TAG, "Decoded frame size %d width:%dx%d", (int)out_frame.decoded_size,
                 (int)frame_info.res.width, (int)frame_info.res.height);
    } while (0);

    // Close decoder
    esp_video_dec_close(dec_handle);

    // Unregister decoder
    esp_video_dec_unregister_default();
    return ret;
}

TEST_CASE("Single Encoder and Decode Test", "VIDEO_CODEC_TEST")
{
    do {
        int ret = single_mjpeg_encode_test();
        if (ret != ESP_VC_ERR_OK) {
            ESP_LOGE(TAG, "Fail to encode frame");
            break;
        }
        ret = single_mjpeg_decode_test();
        if (ret != ESP_VC_ERR_OK) {
            ESP_LOGE(TAG, "Fail to decode frame");
            break;
        }
        // Gather information and draw result
        draw_decode_result();
    } while (0);
    release_test_res();
}
void app_main(void)
{
    ESP_LOGI(TAG, "Start test for esp_video_codec version %s", esp_video_codec_get_version());
    unity_run_menu();
}
