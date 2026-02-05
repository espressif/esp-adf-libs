/* Raw extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdlib.h>
#include <string.h>
#include "esp_extractor_ctrl.h"
#include "raw_extractor_test.h"
#include "esp_log.h"

#define OPUS_MAX_FRAME_SIZE  (1024)
#define OPUS_MIN_FRAME_SIZE  (512)
#define OPUS_DEC_ALIGN       (16)
#define ELEMS(arr)           (sizeof(arr) / sizeof(arr[0]))
#define TAG                  "RAW_TEST"
#define SAME_STREAM(a, b)    (a.stream_type == b.stream_type && a.audio_info.format == b.audio_info.format && a.audio_info.sample_rate == b.audio_info.sample_rate && a.audio_info.channel == b.audio_info.channel && a.audio_info.bits_per_sample == b.audio_info.bits_per_sample)
typedef struct {
    int       frame_num;
    uint32_t  frame_size;
    uint8_t   pattern;
} opus_feeder_t;

static int opus_read(void *buffer, uint32_t size, void *ctx)
{
    opus_feeder_t *feeder = (opus_feeder_t *)ctx;
    uint32_t frame_size = rand() % (OPUS_MAX_FRAME_SIZE - OPUS_MIN_FRAME_SIZE) + OPUS_MIN_FRAME_SIZE;
    feeder->pattern = feeder->frame_num & 0xFF;
    if (size > frame_size) {
        memset(buffer, feeder->pattern, frame_size);
    }
    feeder->frame_num++;
    feeder->frame_size = frame_size;
    return frame_size;
}

int raw_extractor_test(void)
{
    int ret = 0;
    opus_feeder_t feeder = {};
    esp_extractor_handle_t extractor = NULL;
    do {
        // Register for raw extractor
        ret = esp_raw_extractor_register();
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to register raw extractor");
            break;
        }

        esp_extractor_config_t config = {
            .type = ESP_EXTRACTOR_TYPE_RAW,
            .extract_mask = ESP_EXTRACT_MASK_AV,
            .in_read_cb = opus_read,
            .in_ctx = &feeder,
            .out_align = 16,
            .out_pool_size = 10 * 1024,
        };
        // Open raw extractor
        ret = esp_extractor_open(&config, &extractor);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to open raw extractor ret %d", ret);
            break;
        }
        // Set stream info
        esp_extractor_stream_info_t stream_info = {
            .stream_type = ESP_EXTRACTOR_STREAM_TYPE_AUDIO,
            .audio_info = {
                .format = ESP_EXTRACTOR_AUDIO_FORMAT_OPUS,
                .sample_rate = 48000,
                .channel = 2,
                .bits_per_sample = OPUS_DEC_ALIGN,
            },
        };
        ret = esp_extractor_ctrl(extractor, ESP_EXTRACTOR_CTRL_TYPE_SET_STREAM_INFO, &stream_info, sizeof(stream_info));
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to set stream info ret %d", ret);
            break;
        }
        // Set maximum frame size
        uint32_t max_frame_size = OPUS_MAX_FRAME_SIZE;
        ret = esp_extractor_ctrl(extractor, ESP_EXTRACTOR_CTRL_TYPE_SET_MAX_FRAME_SIZE, &max_frame_size, sizeof(max_frame_size));
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to set max frame size ret %d", ret);
            break;
        }
        // Start extractor
        ret = esp_extractor_parse_stream(extractor);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to parse stream ret %d", ret);
            break;
        }
        uint16_t stream_num = 0;
        // Get stream number
        ret = esp_extractor_get_stream_num(extractor, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, &stream_num);
        if (ret != 0 || stream_num != 1) {
            ESP_LOGE(TAG, "Failed to get stream num ret %d", ret);
            break;
        }
        // Get stream information
        esp_extractor_stream_info_t audio_info = {0};
        ret = esp_extractor_get_stream_info(extractor, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, 0, &audio_info);
        if (ret != 0 || !SAME_STREAM(audio_info, stream_info)) {
            ESP_LOGE(TAG, "Failed to get stream info ret %d", ret);
            break;
        }
        // Loop to read frame
        esp_extractor_frame_info_t frame_info[8];
        uint8_t pattern[ELEMS(frame_info)];
        int loop_count = 100;
        while (loop_count-- > 0) {
            memset(frame_info, 0, sizeof(frame_info));
            memset(pattern, 0, sizeof(pattern));
            for (int i = 0; i < ELEMS(frame_info); i++) {
                ret = esp_extractor_read_frame(extractor, &frame_info[i]);
                if (ret != 0) {
                    ESP_LOGE(TAG, "Failed to read frame ret %d loop %d:%d", ret, loop_count, i);
                    break;
                }
                pattern[i] = feeder.pattern;
                if (frame_info[i].frame_size != feeder.frame_size) {
                    ESP_LOGE(TAG, "Frame size not matched");
                    break;
                }
            }
            if (ret != ESP_EXTRACTOR_ERR_OK) {
                break;
            }
            for (int i = 0; i < ELEMS(frame_info); i++) {
                if ((intptr_t)frame_info[i].frame_buffer & (OPUS_DEC_ALIGN - 1)) {
                    ESP_LOGE(TAG, "Frame alignment error");
                }
                uint8_t *p = frame_info[i].frame_buffer;
                // Verify for content
                if (p[0] != pattern[i] || memcmp(p, p + 1, frame_info[i].frame_size - 1) != 0) {
                    ESP_LOGE(TAG, "Frame content error");
                }
                if (p) {
                    esp_extractor_release_frame(extractor, &frame_info[i]);
                }
            }
        }
    } while (0);
    esp_raw_extractor_unregister();
    return ret;
}
