// /*
//  * ESPRESSIF MIT License
//  *
//  * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
//  *
//  * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
//  * it is free of charge, to any person obtaining a copy of this software and associated
//  * documentation files (the "Software"), to deal in the Software without restriction, including
//  * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
//  * to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in all copies or
//  * substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//  * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
//  * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
//  * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//  *
//  */
// #if 1
// #include <stdio.h>
// #include <string.h>
// #include <math.h>
// #include <time.h>
// #include "unity.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "freertos/semphr.h"
// #include "freertos/xtensa_timer.h"
// #include "esp_system.h"
// // #include "esp_spi_flash.h"
// #include "esp_heap_caps.h"
// #include "esp_log.h"
// #include "esp_err.h"
// #include "esp_timer.h"
// // #include "board.h"
// #include "test_common.h"
// #include "esp_ae_alc.h"
// #include "esp_ae_bit_cvt.h"
// #include "esp_ae_ch_cvt.h"
// #include "esp_ae_sonic.h"
// #include "esp_ae_eq.h"
// #include "esp_ae_fade.h"
// #include "esp_ae_mixer.h"
// #include "esp_ae_rate_cvt.h"
// #include "media_lib_adapter.h"
// #include "media_lib_mem_trace.h"

// #define TAG "TEST_AE_PERFORMANCE"

// // if want test cpu-loading must close mem check.

// void wrap_read_8(uint8_t *buf, int byte_len)
// {
//     srand(time(NULL));
//     for (int i = 0; i < byte_len; i++) {
//         buf[i] = (uint8_t)((rand() % (1 << 8)));
//     }
// }

// void wrap_read_16(int16_t *buf, int byte_len)
// {
//     srand(time(NULL));
//     for (int i = 0; i < byte_len; i++) {
//         buf[i] = (short)((rand() % (1 << 16)) - (1 << 15));
//     }
// }

// void wrap_read_24(int8_t *buf, int byte_len)
// {
//     srand(time(NULL));
//     for (int i = 0; i < byte_len; i++) {
//         *(buf++) = 0;
//         int16_t *out = (int16_t *)buf;
//         *out = (int16_t)((rand() % (1 << 16)) - (1 << 15));
//         buf += 2;
//     }
// }

// void wrap_read_32(int16_t *buf, int byte_len)
// {
//     srand(time(NULL));
//     memset(buf, 0, byte_len * 4);
//     int16_t *out = buf;
//     out++;
//     for (int i = 0; i < byte_len; i++) {
//         *out = (int16_t)((rand() % (1 << 16)) - (1 << 15));
//         out += 2;
//     }
// }

// void wrap_read(void *buf, int byte_len, int bit)
// {
//     switch (bit) {
//         case 8:
//             wrap_read_8((uint8_t *)buf, byte_len);
//             break;
//         case 16:
//             wrap_read_16((int16_t *)buf, byte_len);
//             break;
//         case 24:
//             wrap_read_24((int8_t *)buf, byte_len);
//             break;
//         case 32:
//             wrap_read_32((int16_t *)buf, byte_len);
//             break;
//         default:
//             break;
//     }
// }

// void start_mem_cnt()
// {
//     media_lib_mem_trace_cfg_t cfg = {0};
//     cfg.trace_type |= MEDIA_LIB_MEM_TRACE_MODULE_USAGE;
//     cfg.stack_depth = CONFIG_MEDIA_LIB_MEM_TRACE_DEPTH;
//     cfg.record_num = CONFIG_MEDIA_LIB_MEM_TRACE_NUM;
//     media_lib_start_mem_trace(&cfg);
// }

// void end_mem_cnt()
// {
//     media_lib_stop_mem_trace();
//     uint32_t used_mem = 0;
//     uint32_t peak_mem = 0;
//     media_lib_get_mem_usage(NULL, &used_mem, &peak_mem);
// }

// TEST_CASE("Alc performance test", "AUDIO_EFFECTS")
// {
//     ESP_LOGI(TAG, "alc test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int channel = 1;
//     int gain[3] = {-5, 0, 5};
//     int bit[3] = {16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int k = 0; k < 3; k++) {
//         for (int i = 0; i < 3; i++) {
//             void *alc_hd = NULL;
//             char *in_buf = NULL;
//             char *out_buf = NULL;
//             int cnt = 0;
//             int in_num = 0;
//             int out_num = 0;
//             // config
//             esp_ae_alc_cfg_t cfg;
//             cfg.sample_rate = sample_rate;
//             cfg.channel = channel;
//             cfg.bits_per_sample = bit[k];
//             start_mem_cnt();
//             int ret = esp_ae_alc_open(&cfg, &alc_hd);
//             TEST_ASSERT_NOT_EQUAL(alc_hd, NULL);
//             in_num = 512;
//             out_num = 512;
//             in_buf = calloc(1, in_num * channel * (bit[k] >> 3));
//             TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//             out_buf = calloc(1, out_num * channel * (bit[k] >> 3));
//             TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//             int8_t gain_val[2] = {0};
//             for (int j = 0; j < channel; j++) {
//                 esp_ae_alc_set_gain(alc_hd, j, gain[i]);
//             }
//             diff = 0;
//             sample_cnt = 0;
//             while (cnt <= 2000) {
//                 wrap_read(in_buf, in_num * channel, bit[k]);
//                 sample_cnt += in_num;
//                 st = esp_timer_get_time();
//                 ret = esp_ae_alc_process(alc_hd, in_num, (void *)in_buf, (void *)out_buf);
//                 et = esp_timer_get_time();
//                 TEST_ASSERT_EQUAL(ret, ESP_AE_ERR_OK);
//                 diff += et - st;
//                 cnt++;
//             }
//             uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//             ESP_LOGI(TAG, "alc: bit:%d, vol:%d, ratio:%02f", bit[k], gain[i], (double)diff / (duration * 10));
//             esp_ae_alc_close(alc_hd);
//             end_mem_cnt();
//             free(in_buf);
//             free(out_buf);
//         }
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Bit convert performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "bit cvt test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int channel = 1;
//     int bit[4] = {8, 16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int i = 0; i < 4; i++) {
//         for (int j = 0; j < 4; j++) {
//             void *bit_hd = NULL;
//             char *in_buf = NULL;
//             char *out_buf = NULL;
//             int cnt = 0;
//             int in_num = 0;
//             int out_num = 0;
//             esp_ae_bit_cvt_cfg_t cfg = {.sample_rate = sample_rate, .channel = channel, .src_bits = bit[i], .dest_bits = bit[j]};
//             start_mem_cnt();
//             esp_ae_bit_cvt_open(&cfg, &bit_hd);
//             TEST_ASSERT_NOT_EQUAL(bit_hd, NULL);
//             in_num = 512;
//             out_num = 512;
//             in_buf = calloc(1, in_num * channel * (bit[i] >> 3));
//             TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//             out_buf = calloc(1, out_num * channel * (bit[j] >> 3));
//             TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//             diff = 0;
//             sample_cnt = 0;
//             while (cnt <= 2000) {
//                 wrap_read(in_buf, in_num * channel, bit[i]);
//                 st = esp_timer_get_time();
//                 esp_ae_bit_cvt_process(bit_hd, in_num, in_buf, out_buf);
//                 et = esp_timer_get_time();
//                 diff += et - st;
//                 sample_cnt += in_num;
//                 cnt++;
//             }
//             uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//             ESP_LOGI(TAG, "bitcvt: src_bit:%d, dest_bit:%d, ratio:%02f", bit[i], bit[j], (double)diff / (duration * 10));
//             esp_ae_bit_cvt_close(bit_hd);
//             end_mem_cnt();
//             free(in_buf);
//             free(out_buf);
//         }
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Channel convert performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "ch cvt test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int bit[3] = {16, 24, 32};
//     int src_ch = 2;
//     int dest_ch = 1;
//     float weight[2] = {0.5, 0.5};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int i = 0; i < 3; i++) {
//         void *ch_hd = NULL;
//         char *in_buf = NULL;
//         char *out_buf = NULL;
//         int cnt = 0;
//         int in_num = 0;
//         int out_num = 0;
//         // config
//         esp_ae_ch_cvt_cfg_t cfg = {0};
//         cfg.sample_rate = sample_rate;
//         cfg.src_ch = src_ch;
//         cfg.dest_ch = dest_ch;
//         cfg.bits_per_sample = bit[i];
//         cfg.weight = weight;
//         cfg.weight_len = dest_ch * src_ch;
//         in_num = 512;
//         out_num = 512;
//         in_buf = calloc(1, in_num * src_ch * (bit[i] >> 3));
//         TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//         out_buf = calloc(1, out_num * dest_ch * (bit[i] >> 3));
//         TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//         start_mem_cnt();
//         int ret = esp_ae_ch_cvt_open(&cfg, &ch_hd);
//         TEST_ASSERT_NOT_EQUAL(ch_hd, NULL);
//         diff = 0;
//         sample_cnt = 0;
//         while (cnt <= 2000) {
//             wrap_read(in_buf, in_num * src_ch, bit[i]);
//             st = esp_timer_get_time();
//             esp_ae_ch_cvt_process(ch_hd, in_num, in_buf, out_buf);
//             et = esp_timer_get_time();
//             diff += et - st;
//             sample_cnt += in_num;
//             cnt++;
//         }
//         uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//         ESP_LOGI(TAG, "chcvt: bit:%d, ratio:%02f", bit[i], (double)diff / (duration * 10));
//         esp_ae_ch_cvt_close(ch_hd);
//         end_mem_cnt();
//         free(in_buf);
//         free(out_buf);
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Eq performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "eq test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int channel = 1;
//     int bit[3] = {16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int i = 0; i < 3; i++) {
//         void *eq_hd = NULL;
//         char *in_buf = NULL;
//         char *out_buf = NULL;
//         int cnt = 0;
//         int in_num = 0;
//         int out_num = 0;
//         // config
//         esp_ae_eq_cfg_t *cfg = calloc(1, sizeof(esp_ae_eq_cfg_t));
//         TEST_ASSERT_NOT_EQUAL(cfg, NULL);
//         cfg->bits_per_sample = bit[i];
//         cfg->sample_rate = sample_rate;
//         cfg->channel = channel;
//         cfg->filter_num = 1;
//         cfg->para = calloc(1, sizeof(esp_ae_eq_filter_para_t) * cfg->filter_num);
//         TEST_ASSERT_NOT_EQUAL(cfg->para, NULL);
//         cfg->para->filter_type = ESP_AE_EQ_FILTER_PEAK;
//         if (cfg->para->filter_type == ESP_AE_EQ_FILTER_HIGH_PASS
//             || cfg->para->filter_type == ESP_AE_EQ_FILTER_LOW_PASS) {
//             cfg->para->fc = 2000;
//             cfg->para->q = 1.0;
//         } else {
//             cfg->para->fc = 2000;
//             cfg->para->q = 1.0;
//             cfg->para->gain = 5.0;
//         }
//         // create handle
//         start_mem_cnt();
//         int ret = esp_ae_eq_open(cfg, &eq_hd);
//         TEST_ASSERT_NOT_EQUAL(eq_hd, NULL);
//         in_num = 512;
//         out_num = 512;
//         in_buf = calloc(1, in_num * channel * (bit[i] >> 3));
//         TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//         out_buf = calloc(1, out_num * channel * (bit[i] >> 3));
//         TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//         diff = 0;
//         sample_cnt = 0;
//         while (cnt <= 2000) {
//             wrap_read(in_buf, in_num * channel, bit[i]);
//             st = esp_timer_get_time();
//             esp_ae_eq_process(eq_hd, in_num, in_buf, out_buf);
//             et = esp_timer_get_time();
//             diff += et - st;
//             sample_cnt += in_num;
//             cnt++;
//         }
//         uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//         ESP_LOGI(TAG, "eq: bit:%d, ratio:%02f", bit[i], (double)diff / (duration * 10));
//         esp_ae_eq_close(eq_hd);
//         end_mem_cnt();
//         free(in_buf);
//         free(out_buf);
//         if (cfg != NULL) {
//             if (cfg->para != NULL) {
//                 free(cfg->para);
//             }
//             free(cfg);
//         }
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Fade performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "fade test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int channel = 1;
//     int bit[3] = {16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int k = 0; k < 3; k++) {
//         esp_ae_fade_handle_t fade_hd = NULL;
//         char *in_buf = NULL;
//         char *out_buf = NULL;
//         int cnt = 0;
//         int in_num = 0;
//         int out_num = 0;
//         // config
//         esp_ae_fade_cfg_t cfg = {0};
//         cfg.mode = ESP_AE_FADE_MODE_FADE_IN;
//         cfg.curve = ESP_AE_FADE_CURVE_QUAD;
//         cfg.transit_time = 120000;
//         cfg.sample_rate = sample_rate;
//         cfg.channel = channel;
//         cfg.bits_per_sample = bit[k];
//         // create handle
//         start_mem_cnt();
//         int ret = esp_ae_fade_open(&cfg, &fade_hd);
//         TEST_ASSERT_NOT_EQUAL(fade_hd, NULL);
//         // allocate buffer
//         in_num = 512;
//         out_num = 512;
//         in_buf = calloc(1, in_num * channel * (bit[k] >> 3));
//         TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//         out_buf = calloc(1, out_num * channel * (bit[k] >> 3));
//         TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//         diff = 0;
//         sample_cnt = 0;
//         // process
//         while (cnt <= 2000) {
//             wrap_read(in_buf, in_num * channel, bit[k]);
//             st = esp_timer_get_time();
//             ret = esp_ae_fade_process(fade_hd, in_num, in_buf, out_buf);
//             et = esp_timer_get_time();
//             diff += et - st;
//             sample_cnt += in_num;
//             cnt++;
//         }
//         uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//         ESP_LOGI(TAG, "fade: bit:%d, ratio:%02f", bit[k], (double)diff / (duration * 10));
//         // deinit
//         esp_ae_fade_close(fade_hd);
//         end_mem_cnt();
//         free(in_buf);
//         free(out_buf);
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Mixer performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "mixer test");
//     media_lib_add_default_adapter();
//     int sample_rate = 8000;
//     int channel = 1;
//     int bit[3] = {16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int i = 0; i < 3; i++) {
//         int cnt = 0;
//         int in_num = 0;
//         int out_num = 0;
//         void *downmix_hd = NULL;
//         // config
//         esp_ae_mixer_info_t source_info[2] = {0};
//         esp_ae_mixer_info_t info1 = {
//             .weight1 = 0.0,
//             .weight2 = 1.0,
//             .transit_time = 12000,
//         };
//         source_info[0] = info1;
//         esp_ae_mixer_info_t info2 = {
//             .weight1 = 0.0,
//             .weight2 = 1.0,
//             .transit_time = 12000,
//         };
//         source_info[1] = info2;
//         esp_ae_mixer_cfg_t cfg;
//         cfg.sample_rate = sample_rate,
//         cfg.channel = channel,
//         cfg.bits_per_sample = bit[i],
//         cfg.source_info = source_info;
//         cfg.source_num = 2;
//         // create handle
//         start_mem_cnt();
//         int ret = esp_ae_mixer_open(&cfg, &downmix_hd);
//         TEST_ASSERT_NOT_EQUAL(downmix_hd, NULL);
//         // allocate buffer
//         in_num = 512;
//         out_num = 512;
//         unsigned char *inbuf1 = heap_caps_aligned_calloc(16, 1, in_num * channel * (bit[i] >> 3), MALLOC_CAP_SPIRAM);
//         TEST_ASSERT_NOT_EQUAL(inbuf1, NULL);
//         unsigned char *inbuf2 = heap_caps_aligned_calloc(16, 1, in_num * channel * (bit[i] >> 3), MALLOC_CAP_SPIRAM);
//         TEST_ASSERT_NOT_EQUAL(inbuf2, NULL);
//         unsigned char *in[3];
//         in[0] = inbuf1;
//         in[1] = inbuf2;
//         unsigned char *outbuf = heap_caps_aligned_calloc(16, 1, out_num * channel * (bit[i] >> 3), MALLOC_CAP_SPIRAM);
//         TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
//         diff = 0;
//         sample_cnt = 0;
//         esp_ae_mixer_set_mode(downmix_hd, 0, ESP_AE_MIXER_MODE_FADE_UPWARD);
//         esp_ae_mixer_set_mode(downmix_hd, 1, ESP_AE_MIXER_MODE_FADE_UPWARD);
//         while (cnt <= 200) {
//             wrap_read(inbuf1, in_num * channel, bit[i]);
//             wrap_read(inbuf2, in_num * channel, bit[i]);
//             st = esp_timer_get_time();
//             ret = esp_ae_mixer_process(downmix_hd, in_num, (void **)in, outbuf);
//             et = esp_timer_get_time();
//             diff += et - st;
//             sample_cnt += in_num;
//             cnt++;
//         }
//         uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//         ESP_LOGI(TAG, "mixer: bit:%d, ratio:%02f", bit[i], (double)diff / (duration * 10));
//         esp_ae_mixer_close(downmix_hd);
//         end_mem_cnt();
//         free(inbuf1);
//         free(inbuf2);
//         free(outbuf);
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Rsp performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "rsp test");
//     media_lib_add_default_adapter();
//     int channel = 1;
//     int bit[3] = {16, 24, 32};
//     int in_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
//     int out_rate[20] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int k = 0; k < 3; k++) {
//         for (int i = 0; i < 12; i++) {
//             for (int j = 0; j < 12; j++) {
//                 for (int l = 1; l <= 3; l++) {
//                     int complexity = l;
//                     void *rsp_hd = NULL;
//                     char *in_buf = NULL;
//                     char *out_buf = NULL;
//                     int cnt = 0;
//                     uint32_t in_num = 0;
//                     uint32_t out_num = 0;
//                     uint32_t out_samples = 0;
//                     // config
//                     esp_ae_rate_cvt_cfg_t cfg;
//                     cfg.bits_per_sample = bit[k];
//                     cfg.src_rate = in_rate[i];
//                     cfg.dest_rate = out_rate[j];
//                     cfg.channel = channel;
//                     cfg.complexity = complexity;
//                     start_mem_cnt();
//                     int ret = esp_ae_rate_cvt_open(&cfg, &rsp_hd);
//                     TEST_ASSERT_NOT_EQUAL(rsp_hd, NULL);
//                     // create buffer
//                     in_num = 512;
//                     in_buf = calloc(1, in_num * channel * (bit[k] >> 3));
//                     TEST_ASSERT_NOT_EQUAL(in_buf, NULL);
//                     esp_ae_rate_cvt_get_max_out_sample_num(rsp_hd, in_num, &out_num);
//                     out_buf = calloc(1, out_num * channel * (bit[k] >> 3));
//                     TEST_ASSERT_NOT_EQUAL(out_buf, NULL);
//                     diff = 0;
//                     sample_cnt = 0;
//                     while (cnt <= 2000) {
//                         wrap_read(in_buf, in_num * channel, bit[k]);
//                         out_samples = out_num;
//                         st = esp_timer_get_time();
//                         ret = esp_ae_rate_cvt_process(rsp_hd, in_buf, in_num, out_buf, &out_samples);
//                         et = esp_timer_get_time();
//                         diff += et - st;
//                         sample_cnt += in_num;
//                         cnt++;
//                     }
//                     uint64_t duration = 1000.0 * (double)sample_cnt / (double)cfg.src_rate;
//                     ESP_LOGI(TAG, "rsp: src_rate:%d, dest_rate:%d, bit:%d, com:%d, ratio:%02f",
//                            in_rate[i], out_rate[j], bit[k], complexity, (double)diff / (duration * 10));
//                     esp_ae_rate_cvt_close(rsp_hd);
//                     end_mem_cnt();
//                     free(in_buf);
//                     free(out_buf);
//                 }
//             }
//         }
//     }
//     media_lib_stop_mem_trace();
// }

// TEST_CASE("Sonic performance test", "AUDIO_EFFECT")
// {
//     ESP_LOGI(TAG, "sonic test");
//     media_lib_add_default_adapter();
//     float para[6] = {0.5, 0.75, 1.25, 1.5, 1.75, 2.0};
//     int sample_rate = 8000;
//     int channel = 1;
//     int mode[2] = {1, 2};
//     int bit[3] = {16, 24, 32};
//     uint64_t st;
//     uint64_t et;
//     uint64_t diff;
//     uint64_t sample_cnt = 0;
//     for (int k = 0; k < 2; k++) {
//         for (int j = 0; j < 3; j++) {
//             for (int i = 0; i < 6; i++) {
//                 int cnt = 0;
//                 esp_ae_sonic_in_data_t in_samples = {0};
//                 esp_ae_sonic_out_data_t out_samples = {0};
//                 int in_num = 0;
//                 int out_num = 0;
//                 short *inbuf = NULL;
//                 short *outbuf = NULL;
//                 void *sonic_handle = NULL;
//                 int ret = 0;
//                 // config
//                 esp_ae_sonic_cfg_t cfg = {0};
//                 cfg.sample_rate = sample_rate;
//                 cfg.channel = channel;
//                 cfg.bits_per_sample = bit[j];
//                 // buffer allocate
//                 in_num = 512;
//                 out_num = 512;
//                 inbuf = calloc(1, in_num * channel * (bit[j] >> 3));
//                 TEST_ASSERT_NOT_EQUAL(inbuf, NULL);
//                 outbuf = calloc(1, out_num * channel * (bit[j] >> 3));
//                 TEST_ASSERT_NOT_EQUAL(outbuf, NULL);
//                 in_samples.samples = inbuf;
//                 out_samples.samples = outbuf;
//                 // create handle
//                 start_mem_cnt();
//                 esp_ae_sonic_open(&cfg, &sonic_handle);
//                 TEST_ASSERT_NOT_EQUAL(sonic_handle, NULL);
//                 // para set
//                 if (mode[k] == 1) {
//                     esp_ae_sonic_set_speed(sonic_handle, para[i]);
//                 } else if (mode[k] == 2) {
//                     esp_ae_sonic_set_pitch(sonic_handle, para[i]);
//                 }
//                 while (cnt <= 2000) {
//                     int in_read = in_num * channel * (bit[j] >> 3);
//                     wrap_read(inbuf, in_num * channel, bit[j]);
//                     int sample_num = in_read / (channel * (bit[j] >> 3));
//                     int remain_num = sample_num;
//                     in_samples.samples = inbuf;
//                     in_samples.num = sample_num;
//                     out_samples.needed_num = 512;
//                     diff = 0;
//                     sample_cnt = 0;
//                     sample_cnt += in_num;
//                     while (remain_num > 0 || out_samples.out_num > 0) {
//                         st = esp_timer_get_time();
//                         ret = esp_ae_sonic_process(sonic_handle, &in_samples, &out_samples);
//                         et = esp_timer_get_time();
//                         diff += et - st;
//                         if (ret != 0) {
//                             ESP_LOGI(TAG, "error");
//                             break;
//                         }
//                         char *in = inbuf + in_samples.consume_num;
//                         remain_num -= in_samples.consume_num;
//                         in_samples.num = remain_num;
//                         in_samples.samples = in;
//                     }
//                     cnt++;
//                 }
//                 uint64_t duration = 1000.0 * (double)sample_cnt / (double)sample_rate;
//                 ESP_LOGI(TAG, "sonic: bit:%d, mode:%d, para:%02f, ratio:%02f",
//                        bit[j], mode[k], para[i], (double)diff / (duration * 10));
//                 esp_ae_sonic_close(sonic_handle);
//                 end_mem_cnt();
//                 free(inbuf);
//                 inbuf = NULL;
//                 free(outbuf);
//                 outbuf = NULL;
//             }
//         }
//     }
//     media_lib_stop_mem_trace();
// }
// #endif