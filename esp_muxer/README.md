# ESP-Muxer

- [![Component Registry](https://components.espressif.com/components/espressif/esp_muxer/badge.svg)](https://components.espressif.com/components/espressif/esp_muxer)

ESP_Muxer is a module for muxing audio and video data to container files or direct stream output.

## Features
   
- Muxing both audio and video data
- Support for multiple audio and video tracks if supported in containers
- Compatible with common video and audio formats
- Direct file saving
- File slice setting and customized file storage name
- Support for streaming callback data
- Customizable write function to use customized storage media

It is suitable for applications such as personal video recorder, HLS media segment provider, HTTP FLV sources etc.

## Supported Containers and Codecs

|           | MP4  | TS   | FLV  | WAV  | CAF  | OGG  |
| :-------- | :--- | :--- | :--- | :--- | :--- | :--- |
| PCM       | Y    | N    | Y    | Y    | Y    | N    |
| AAC       | Y    | Y    | Y    | Y    | Y    | N    |
| MP3       | Y    | Y    | Y    | Y    | N    | N    |
| ADPCM     | N    | N    | N    | Y    | Y    | N    |
| G711 Alaw | N    | N    | N    | Y    | Y    | N    |
| G711 Ulaw | N    | N    | N    | Y    | Y    | N    |
| AMR-NB    | N    | N    | N    | Y    | N    | N    |
| AMR-WB    | N    | N    | N    | Y    | N    | N    |
| OPUS      | N    | N    | N    | N    | N    | Y    |
| ALAC      | N    | N    | N    | N    | Y    | N    |
| H264      | Y    | Y    | Y    | N    | N    | N    |
| MJPEG     | Y    | Y    | Y    | N    | N    | N    |

Notes:
    Since TS and FLV do not officially support MJPEG, MJPEG support for TS and FLV is added using the following methods:
  - FLV uses codec ID 1 for the MJPEG codec.
  - TS uses stream ID 6 for the MJPEG codec.

    You can check [ffmpeg_mjpeg.patch](ffmpeg_mjpeg.patch) for technical details and patch it to support them in FFmpeg.  
    Since the MP4 and WAV container header sizes can vary depending on their data size, please do not use them for streaming out data.
 

## Usage

The following example demonstrates how to mux AAC audio data from a file to all the supported containers.
```c
#include "esp_muxer.h"
#include "flv_muxer.h"
#include "mp4_muxer.h"
#include "ts_muxer.h"
#include "wav_muxer.h"
#include <stdio.h>
#include <stdlib.h>

#define STORE_PATH          "/sdcard/"
#define FILE_SLICE_DURATION (60000)  // Unit milliseconds
#define ADTS_HEAD_SIZE      (7)

typedef union {
    ts_muxer_config_t  ts_cfg;
    mp4_muxer_config_t mp4_cfg;
    flv_muxer_config_t flv_cfg;
    wav_muxer_config_t wav_cfg;
} muxer_all_cfg_t;

static esp_muxer_type_t muxer_type;

static int file_pattern_cb(char *file_name, int len, int slice_idx)
{
    switch (muxer_type) {
        case ESP_MUXER_TYPE_TS:
            snprintf(file_name, len, STORE_PATH "slice_%d.ts", slice_idx);
            break;
        case ESP_MUXER_TYPE_MP4:
            snprintf(file_name, len, STORE_PATH "slice_%d.mp4", slice_idx);
            break;
        case ESP_MUXER_TYPE_FLV:
            snprintf(file_name, len, STORE_PATH "slice_%d.flv", slice_idx);
            break;
        case ESP_MUXER_TYPE_WAV:
            snprintf(file_name, len, STORE_PATH "slice_%d.wav", slice_idx);
            break;
        default:
            return -1;
    }
    return 0;
}

static esp_muxer_handle_t setup_muxer(void)
{
    muxer_all_cfg_t cfg = {0};
    esp_muxer_config_t *base_cfg = &cfg.ts_cfg.base_config;
    base_cfg->muxer_type = muxer_type;
    base_cfg->url_pattern = file_pattern_cb;        // File storage name pattern
    base_cfg->slice_duration = FILE_SLICE_DURATION; // Record file duration
    switch (muxer_type) {
        case ESP_MUXER_TYPE_TS:
            ts_muxer_register();
            cfg.ts_cfg.pat_resend_duration = 0;
            return esp_muxer_open(base_cfg, sizeof(ts_muxer_config_t));
        case ESP_MUXER_TYPE_MP4:
            mp4_muxer_register();
            cfg.mp4_cfg.display_in_order = true;
            cfg.mp4_cfg.moov_before_mdat = true;
            return esp_muxer_open(base_cfg, sizeof(mp4_muxer_config_t));
        case ESP_MUXER_TYPE_FLV:
            flv_muxer_register();
            return esp_muxer_open(base_cfg, sizeof(flv_muxer_config_t));
        case ESP_MUXER_TYPE_WAV:
            wav_muxer_register();
            return esp_muxer_open(base_cfg, sizeof(wav_muxer_config_t));
        default:
            return NULL;
    }
}

static void clearup_muxer(esp_muxer_handle_t muxer)
{
    if (muxer) {
        esp_muxer_close(muxer);
    }
    esp_muxer_unreg_all();
}

static int muxer_from_aac_file(esp_muxer_handle_t muxer, const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL) {
        printf("aac file %s not exists\n", file);
        return -1;
    }
    int max_frame_size = 2048;
    uint8_t *frame = malloc(max_frame_size);
    int samples = 0;
    int sample_rate = 0;
    int ret = 0;
    int stream_idx = 0;
    while (1) {
        if (frame == NULL) {
            printf("no memory for %d\n", max_frame_size);
            break;
        }
        uint8_t *adts_header = frame;
        ret = fread(adts_header, 1, ADTS_HEAD_SIZE, fp);
        if (ret != ADTS_HEAD_SIZE) {
            printf("File eof reached\n");
            break;
        }
        int frame_size = ((adts_header[3] & 0x03) << 11) | (adts_header[4] << 3) | (adts_header[5] >> 5);
        if (frame_size > max_frame_size) {
            frame = realloc(frame, frame_size);
            max_frame_size = frame_size;
            if (frame == NULL) {
                printf("no memory for %d\n", max_frame_size);
                break;
            }
        }
        ret = fread(frame + ADTS_HEAD_SIZE, 1, frame_size - ADTS_HEAD_SIZE, fp);
        if (ret != frame_size - ADTS_HEAD_SIZE) {
            printf("File is truncated\n");
            break;
        }
        if (sample_rate == 0) {
            const int sample_rate_table[16] = {
                96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350,
            };
            sample_rate = sample_rate_table[(adts_header[2] & 0x3c) >> 2];
            if (sample_rate == 0) {
                printf("Get bad sample rate\n");
                break;
            }
            int channel = ((adts_header[2] & 1) << 2) | (adts_header[3] >> 6);
            esp_muxer_audio_stream_info_t audio_stream = {
                .min_packet_duration = 15,
                .bits_per_sample = 16,
                .sample_rate = sample_rate,
                .channel = channel == 7 ? 8 : channel,
                .codec = ESP_MUXER_ADEC_AAC,
            };
            // Add stream and set stream information firstly, stream_idx is used to identify tracks
            ret = esp_muxer_add_audio_stream(muxer, &audio_stream, &stream_idx);
            if (ret != ESP_MUXER_ERR_OK) {
                printf("Fail to add audio stream %d\n", ret);
                break;
            }
        }
        esp_muxer_audio_packet_t audio_packet = {
            .data = frame,
            .len = frame_size,
        };
        audio_packet.pts = (uint32_t) ((uint64_t) 1000 * samples / sample_rate);
        samples += 1024; // Suppose all frame is 1024 samples
        // Add audio data afterwards
        ret = esp_muxer_add_audio_packet(muxer, stream_idx, &audio_packet);
        if (ret != ESP_MUXER_ERR_OK) {
            printf("Fail to add audio packet %d\n", ret);
            break;
        }
    }
    if (frame) {
        free(frame);
    }
    if (fp) {
        fclose(fp);
    }
    return ret;
}

static int muxer_test(char *aac_file)
{
    esp_muxer_handle_t muxer = setup_muxer();
    int ret = -1;
    if (muxer) {
        ret = muxer_from_aac_file(muxer, aac_file);
    }
    clearup_muxer(muxer);
    return ret;
}

int simple_muxer_test()
{
    char *aac_file = "test.aac";
    muxer_type = (esp_muxer_type_t) 0;
    int ret = 0;
    for (; muxer_type < ESP_MUXER_TYPE_MAX; muxer_type++) {
        ret = muxer_test(aac_file);
        printf("Muxer to %d return %d\n", muxer_type, ret);
    }
    return ret;
}
```

The following example demonstrates how to do write speed test using `esp_muxer`.
```c
#include "esp_muxer.h"
#include "mp4_muxer.h"
#include "esp_timer.h"

static int file_pattern_cb(char *file_name, int len, int slice_idx)
{
    snprintf(file_name, len, "/sdcard/st.mp4");
    return 0;
}

static int muxer_speed_test(int cache_size, int duration)
{
    mp4_muxer_config_t cfg = {0};
    esp_muxer_config_t *base_cfg = &cfg.base_config;
    base_cfg->muxer_type = ESP_MUXER_TYPE_MP4;
    base_cfg->url_pattern = file_pattern_cb;
    base_cfg->slice_duration = duration + 1000;
    base_cfg->ram_cache_size = cache_size;
    cfg.display_in_order = true;
    cfg.moov_before_mdat = true;
    mp4_muxer_register();
    esp_muxer_handle_t muxer = esp_muxer_open(base_cfg, sizeof(mp4_muxer_config_t));
    esp_muxer_audio_stream_info_t audio_stream = {
        .min_packet_duration = 15,
        .bits_per_sample = 16,
        .sample_rate = 44100,
        .channel = 2,
        .codec = ESP_MUXER_ADEC_PCM,
    };
    int stream_idx = 0;
    int ret = esp_muxer_add_audio_stream(muxer, &audio_stream, &stream_idx);
    int write_size = 100 * 1024;
    uint8_t *pcm_data = (uint8_t *) malloc(100 * 1024);
    int start_time = (int) (esp_timer_get_time() / 1000);
    int end_time = start_time;
    int total_write_size = 0;
    while (end_time < start_time + duration) {
        esp_muxer_audio_packet_t audio_packet = {
            .data = pcm_data,
            .len = write_size,
        };
        audio_packet.pts = end_time - start_time;
        total_write_size += write_size;
        int ret = esp_muxer_add_audio_packet(muxer, stream_idx, &audio_packet);
        end_time = (int) (esp_timer_get_time() / 1000);
        if (ret != 0) {
            break;
        }
    }
    int elapse = end_time - start_time;
    printf("Cache %d write %d/%d = %d\n", cache_size, total_write_size, elapse,
           (int) ((uint64_t) total_write_size * 1000 / elapse));
    free(pcm_data);
    esp_muxer_close(muxer);
    return 0;
}

static void speed_test(void)
{
    muxer_speed_test(0, 30000);
    muxer_speed_test(1024, 30000);
    muxer_speed_test(4096, 30000);
    muxer_speed_test(8192, 30000);
    muxer_speed_test(16 * 1024, 30000);
    muxer_speed_test(32 * 1024, 30000);
    muxer_speed_test(64 * 1024, 30000);
}
```
