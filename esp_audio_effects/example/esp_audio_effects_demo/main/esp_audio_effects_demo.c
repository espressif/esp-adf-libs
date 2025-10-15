/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ae_drc.h"
#include "esp_ae_mbc.h"
#include "esp_ae_alc.h"
#include "esp_ae_fade.h"
#include "esp_ae_eq.h"
#include "esp_ae_mixer.h"
#include "esp_ae_sonic.h"
#include "esp_ae_bit_cvt.h"
#include "esp_ae_ch_cvt.h"
#include "esp_ae_rate_cvt.h"
#include "esp_ae_types.h"
#include "esp_board_manager_includes.h"
#include "esp_codec_dev.h"

#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER)
extern const uint8_t test_stream_start[] asm("_binary_voice_with_music_pcm_start");
extern const uint8_t test_stream_end[] asm("_binary_voice_with_music_pcm_end");
extern const uint8_t tone_pcm_start[] asm("_binary_tone_pcm_start");
extern const uint8_t tone_pcm_end[] asm("_binary_tone_pcm_end");
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC)
#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO)
extern const uint8_t test_stream_start[] asm("_binary_voice_flick_up_and_down_pcm_start");
extern const uint8_t test_stream_end[] asm("_binary_voice_flick_up_and_down_pcm_end");
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_REMOVE_HIT_DEMO)
extern const uint8_t test_stream_start[] asm("_binary_voice_with_hit_pcm_start");
extern const uint8_t test_stream_end[] asm("_binary_voice_with_hit_pcm_end");
#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO) */
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT)
extern const uint8_t test_stream_start[] asm("_binary_manen_48000_2_24_10_pcm_start");
extern const uint8_t test_stream_end[] asm("_binary_manen_48000_2_24_10_pcm_end");
#else
extern const uint8_t test_stream_start[] asm("_binary_voice_with_music_pcm_start");
extern const uint8_t test_stream_end[] asm("_binary_voice_with_music_pcm_end");
#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER) */

#define TAG               "ESP_AUDIO_EFFECTS_DEMO"
#define SAMPLE_RATE       16000
#define CHANNELS          1
#define BITS_PER_SAMPLE   16
#define SAMPLES_PER_CHUNK 256
#define BYTES_PER_SAMPLE  (BITS_PER_SAMPLE >> 3)
#define CHUNK_SIZE_BYTES  (SAMPLES_PER_CHUNK * CHANNELS * BYTES_PER_SAMPLE)
#define INIT_EMBED_DATA(_start_symbol, _end_symbol) \
    (embed_flash_data_t)                            \
    {                                               \
        .data = (_start_symbol),                    \
        .size = (_end_symbol) - (_start_symbol),    \
        .pos  = 0,                                  \
    }

#define INIT_READ_IO(_embed_data_ptr)         \
    (ae_demo_read_io_t)                       \
    {                                         \
        .read_cb  = ae_demo_read_embed_flash, \
        .read_ctx = (_embed_data_ptr),        \
    }

#define INIT_WRITE_IO()                   \
    (ae_demo_write_io_t)                  \
    {                                     \
        .write_cb  = ae_demo_write_codec, \
        .write_ctx = audio_codec_handle,  \
    }

#define ALLOC_BUFFER(buf, buf_size, exit_label) do {                                                  \
    buf = (int16_t *)heap_caps_aligned_calloc(16, 1, buf_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);  \
    if (buf == NULL) {                                                                                \
        ESP_LOGE(TAG, "Failed to allocate buffer, line %d", __LINE__);                                \
        goto exit_label;                                                                              \
    }                                                                                                 \
} while (0)

#define AE_DEMO_RET_CHECK(ret, action, msg) do {                                   \
    if ((ret) != ESP_AE_ERR_OK) {                                                  \
        ESP_LOGE(TAG, "%s Error: %d at %s:%d", (msg), (ret), __FILE__, __LINE__);  \
        action;                                                                    \
    }                                                                              \
} while (0)

#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ)
#define EQ_SET_FILTER_PARA(_filter_type, _fc, _q, _gain) \
    (esp_ae_eq_filter_para_t)                            \
    {                                                    \
        .filter_type = (_filter_type),                   \
        .fc          = (_fc),                            \
        .q           = (_q),                             \
        .gain        = (_gain),                          \
    }
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER)

#define AE_DEMO_MIXER_INFO_INIT(w1, w2, t) \
    (esp_ae_mixer_info_t) { .weight1 = (w1), .weight2 = (w2), .transit_time = (t) }

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT)

#define SRC_SAMPLE_RATE      48000
#define SRC_CHANNELS         2
#define SRC_BITS_PER_SAMPLE  24
#define SRC_BYTES_PER_SAMPLE (SRC_BITS_PER_SAMPLE >> 3)
#define DST_BYTES_PER_SAMPLE (BITS_PER_SAMPLE >> 3)
#define SRC_CHUNK_SIZE_BYTES (SAMPLES_PER_CHUNK * SRC_CHANNELS * SRC_BYTES_PER_SAMPLE)

#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ) */

/**
 * @brief  Audio effects demo I/O callback function type
 */
typedef int (*ae_demo_io_cb_t)(void *buf, size_t size, void *ctx);

typedef struct {
    const uint8_t  *data;  /*!< Pointer to the embedded data in flash */
    size_t          size;  /*!< Total size of the embedded data */
    size_t          pos;   /*!< Current read/write position in the data */
} embed_flash_data_t;

typedef struct {
    ae_demo_io_cb_t  read_cb;   /*!< Read callback function */
    void            *read_ctx;  /*!< Context pointer for read callback */
} ae_demo_read_io_t;

typedef struct {
    ae_demo_io_cb_t  write_cb;   /*!< Write callback function */
    void            *write_ctx;  /*!< Context pointer for write callback */
} ae_demo_write_io_t;

static dev_audio_codec_handles_t *audio_codec_handle = NULL;

static int ae_demo_read_embed_flash(void *buf, size_t size, void *ctx)
{
    embed_flash_data_t *embed_data = (embed_flash_data_t *)ctx;
    if (embed_data == NULL || buf == NULL) {
        return -1;
    }
    size_t remaining = embed_data->size - embed_data->pos;
    size_t to_read = (size < remaining) ? size : remaining;
    if (to_read == 0) {
        return 0;
    }
    memcpy(buf, embed_data->data + embed_data->pos, to_read);
    embed_data->pos += to_read;
    return (int)to_read;
}

static int ae_demo_write_codec(void *buf, size_t size, void *ctx)
{
    dev_audio_codec_handles_t *codec_handle = (dev_audio_codec_handles_t *)ctx;
    if (codec_handle == NULL || buf == NULL) {
        return -1;
    }
    int written = esp_codec_dev_write(codec_handle->codec_dev, buf, size);
    return written;
}

#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC)

static void demo_alc_process(void)
{
    ESP_LOGI(TAG, "=== ALC Demo ===");
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_alc_handle_t alc_handle = NULL;
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _ALC_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _ALC_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize ALC");
    esp_ae_alc_cfg_t alc_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE};
    esp_ae_err_t ret = esp_ae_alc_open(&alc_cfg, &alc_handle);
    AE_DEMO_RET_CHECK(ret, {goto _ALC_EXIT;}, "Failed to create ALC handle");
    ESP_LOGI(TAG, "[ 2 ] Process audio data in chunks and play through codec");
    uint32_t total_samples = 0;
    const uint32_t samples_4s = 4 * SAMPLE_RATE;  // Samples for 4 seconds
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;  // EOF or error
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        ret = esp_ae_alc_process(alc_handle, samples_read, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _ALC_EXIT;}, "ALC process failed");
        if (total_samples < samples_4s && (total_samples + samples_read) >= samples_4s) {
            ESP_LOGI(TAG, "Setting ALC gain at 4 seconds");
            ret = esp_ae_alc_set_gain(alc_handle, 0, 6);  // Set gain to +6dB
            AE_DEMO_RET_CHECK(ret, {goto _ALC_EXIT;}, "Failed to set ALC gain");
        }
        int bytes_to_write = samples_read * CHANNELS * BYTES_PER_SAMPLE;
        int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _ALC_EXIT;
        }
        total_samples += samples_read;
    }
    ESP_LOGI(TAG, "ALC demo completed");
_ALC_EXIT:
    if (alc_handle != NULL) {
        esp_ae_alc_close(alc_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_FADE)

static void demo_fade_process(void)
{
    ESP_LOGI(TAG, "=== Fade Demo ===");
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_fade_handle_t fade_handle = NULL;
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _FADE_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _FADE_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize Fade (Fade In)");
    esp_ae_fade_cfg_t fade_cfg = {
        .mode = ESP_AE_FADE_MODE_FADE_IN,
        .curve = ESP_AE_FADE_CURVE_LINE,
        .transit_time = 500,
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE};
    esp_ae_err_t ret = esp_ae_fade_open(&fade_cfg, &fade_handle);
    AE_DEMO_RET_CHECK(ret, {goto _FADE_EXIT;}, "Failed to create Fade handle");
    uint32_t total_samples = 0;
    const uint32_t samples_4s = 4 * SAMPLE_RATE;
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        if (total_samples < samples_4s && (total_samples + samples_read) >= samples_4s) {
            ESP_LOGI(TAG, "Setting Fade mode to FADE_OUT at 4 seconds");
            ret = esp_ae_fade_set_mode(fade_handle, ESP_AE_FADE_MODE_FADE_OUT);
            AE_DEMO_RET_CHECK(ret, {goto _FADE_EXIT;}, "Failed to set Fade mode");
        }
        ret = esp_ae_fade_process(fade_handle, samples_read, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _FADE_EXIT;}, "Fade process failed");
        int bytes_to_write = samples_read * CHANNELS * BYTES_PER_SAMPLE;
        int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _FADE_EXIT;
        }
        total_samples += samples_read;
    }
    ESP_LOGI(TAG, "Fade demo completed");
_FADE_EXIT:
    if (fade_handle != NULL) {
        esp_ae_fade_close(fade_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ)

static void demo_eq_process(void)
{
    ESP_LOGI(TAG, "=== EQ Demo ===");
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_eq_handle_t eq_handle = NULL;
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _EQ_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _EQ_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize EQ");
    esp_ae_eq_filter_para_t eq_filters[] = {
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_HIGH_PASS, 120, 0.7f, 0.0f),
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 350, 1.1f, -2.5f),
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 2200, 0.5f, 8.0f),
        EQ_SET_FILTER_PARA(ESP_AE_EQ_FILTER_PEAK, 4500, 1.2f, 1.5f)};

    esp_ae_eq_cfg_t eq_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .filter_num = sizeof(eq_filters) / sizeof(eq_filters[0]),
        .para = eq_filters};

    esp_ae_err_t ret = esp_ae_eq_open(&eq_cfg, &eq_handle);
    AE_DEMO_RET_CHECK(ret, {goto _EQ_EXIT;}, "Failed to create EQ handle");
    for (int i = 0; i < eq_cfg.filter_num; i++) {
        ret = esp_ae_eq_disable_filter(eq_handle, i);
        AE_DEMO_RET_CHECK(ret, {goto _EQ_EXIT;}, "Failed to enable EQ filter");
    }
    ESP_LOGI(TAG, "[ 2 ] Process audio data in chunks and play through codec");
    uint32_t total_samples = 0;
    const uint32_t samples_4s = 4 * SAMPLE_RATE;
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        if (total_samples < samples_4s && (total_samples + samples_read) >= samples_4s) {
            ESP_LOGI(TAG, "Enabling all EQ filters");
            for (int i = 0; i < eq_cfg.filter_num; i++) {
                ret = esp_ae_eq_enable_filter(eq_handle, i);
                AE_DEMO_RET_CHECK(ret, {goto _EQ_EXIT;}, "Failed to disable EQ filter");
            }
        }
        ret = esp_ae_eq_process(eq_handle, samples_read, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _EQ_EXIT;}, "EQ process failed");
        int bytes_to_write = samples_read * CHANNELS * BYTES_PER_SAMPLE;
        int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _EQ_EXIT;
        }
        total_samples += samples_read;
    }
    ESP_LOGI(TAG, "EQ demo completed");
_EQ_EXIT:
    if (eq_handle != NULL) {
        esp_ae_eq_close(eq_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_SONIC)

static void demo_sonic_process(void)
{
    ESP_LOGI(TAG, "=== Sonic Demo ===");
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_sonic_handle_t sonic_handle = NULL;
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _SONIC_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _SONIC_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize Sonic");
    esp_ae_sonic_cfg_t sonic_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE};
    esp_ae_err_t ret = esp_ae_sonic_open(&sonic_cfg, &sonic_handle);
    AE_DEMO_RET_CHECK(ret, {goto _SONIC_EXIT;}, "Failed to create Sonic handle");
    ESP_LOGI(TAG, "[ 2 ] Process audio data in chunks and play through codec");
    uint32_t total_output_samples = 0;
    uint32_t total_input_samples = 0;
    const uint32_t samples_4s = 4 * SAMPLE_RATE;
    esp_ae_sonic_in_data_t in_data = {0};
    esp_ae_sonic_out_data_t out_data = {0};
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        if (total_input_samples < samples_4s && (total_input_samples + samples_read) >= samples_4s) {
            ESP_LOGI(TAG, "Setting Sonic speed and pitch at 4 seconds");
            ret = esp_ae_sonic_set_speed(sonic_handle, 1.2f);
            AE_DEMO_RET_CHECK(ret, {goto _SONIC_EXIT;}, "Failed to set Sonic speed");
            ret = esp_ae_sonic_set_pitch(sonic_handle, 0.8f);
            AE_DEMO_RET_CHECK(ret, {goto _SONIC_EXIT;}, "Failed to set Sonic pitch");
        }
        uint32_t remain_num = samples_read;
        void *in_ptr = input_buffer;
        while (remain_num > 0 || out_data.out_num > 0) {
            in_data.samples = in_ptr;
            in_data.num = remain_num;
            in_data.consume_num = 0;
            out_data.samples = output_buffer;
            out_data.needed_num = SAMPLES_PER_CHUNK;
            out_data.out_num = 0;
            ret = esp_ae_sonic_process(sonic_handle, &in_data, &out_data);
            AE_DEMO_RET_CHECK(ret, {goto _SONIC_EXIT;}, "Sonic process failed");
            if (out_data.out_num > 0) {
                int bytes_to_write = out_data.out_num * CHANNELS * BYTES_PER_SAMPLE;
                int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
                if (written < 0) {
                    ESP_LOGE(TAG, "Failed to write data: %d", written);
                    goto _SONIC_EXIT;
                }
                total_output_samples += out_data.out_num;
            }
            if (in_data.consume_num > 0) {
                int consume_bytes = in_data.consume_num * CHANNELS * BYTES_PER_SAMPLE;
                in_ptr = (uint8_t *)in_ptr + consume_bytes;
                remain_num -= in_data.consume_num;
            } else {
                if (out_data.out_num == 0) {
                    break;
                }
            }
        }
        total_input_samples += samples_read;
    }
    ESP_LOGI(TAG, "Sonic demo completed");
_SONIC_EXIT:
    if (sonic_handle != NULL) {
        esp_ae_sonic_close(sonic_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC)

static void demo_drc_process(void)
{
    ESP_LOGI(TAG, "=== DRC Demo ===");
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_drc_handle_t drc_handle = NULL;
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    embed_flash_data_t original_embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _DRC_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _DRC_EXIT);
    ESP_LOGI(TAG, "Playback the original audio");
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &original_embed_data);
        if (bytes_read <= 0) {
            break;
        }
        int written = write_cb(input_buffer, bytes_read, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _DRC_EXIT;
        }
    }
    ESP_LOGI(TAG, "[ 1 ] Initialize DRC");
#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO)
    esp_ae_drc_curve_point curve_points[3] = {
        {0.0, -25.0},
        {-50.0, -35.0},
        {-100.0, -100.0},
    };
    uint16_t attack_time = 3;
    uint16_t release_time = 50;
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_REMOVE_HIT_DEMO)
    esp_ae_drc_curve_point curve_points[3] = {
        {0.0, -20.0},
        {-20.0, -20.0},
        {-100.0, -100.0},
    };
    uint16_t attack_time = 1;
    uint16_t release_time = 200;
#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO) */
    esp_ae_drc_cfg_t drc_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .drc_para = {
            .point = curve_points,
            .point_num = sizeof(curve_points) / sizeof(curve_points[0]),
            .knee_width = 0.0,
            .attack_time = attack_time,
            .release_time = release_time,
            .hold_time = 0,
            .makeup_gain = 0.0,
        },
    };
    esp_ae_err_t ret = esp_ae_drc_open(&drc_cfg, &drc_handle);
    AE_DEMO_RET_CHECK(ret, {goto _DRC_EXIT;}, "Failed to create DRC handle");
    uint32_t total_samples = 0;
    ESP_LOGI(TAG, "Playback the DRC processed audio");
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        ret = esp_ae_drc_process(drc_handle, samples_read, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _DRC_EXIT;}, "DRC process failed");
        int bytes_to_write = samples_read * CHANNELS * BYTES_PER_SAMPLE;
        int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _DRC_EXIT;
        }
        total_samples += samples_read;
    }
    ESP_LOGI(TAG, "DRC demo completed");
_DRC_EXIT:
    if (drc_handle != NULL) {
        esp_ae_drc_close(drc_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}

#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MBC)

static void demo_mbc_process(void)
{
    ESP_LOGI(TAG, "=== MBC Demo ===");
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    int16_t *input_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_mbc_handle_t mbc_handle = NULL;
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, CHUNK_SIZE_BYTES, _MBC_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _MBC_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize MBC");
    esp_ae_mbc_config_t mbc_cfg = {
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel = CHANNELS,
        .fc = {400, 2100, 6000},
        .mbc_para = {
            {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
            {.makeup_gain = 6, .attack_time = 1, .release_time = 200, .hold_time = 0, .ratio = 4, .knee_width = 0, .threshold = -18},
            {.makeup_gain = 7, .attack_time = 1, .release_time = 200, .hold_time = 0, .ratio = 4, .knee_width = 0, .threshold = -20},
            {.makeup_gain = 0, .attack_time = 10, .release_time = 100, .hold_time = 0, .ratio = 1, .knee_width = 0, .threshold = -0.1},
        }};
    esp_ae_err_t ret = esp_ae_mbc_open(&mbc_cfg, &mbc_handle);
    AE_DEMO_RET_CHECK(ret, {goto _MBC_EXIT;}, "Failed to create MBC handle");
    for (int band = 0; band < 4; band++) {
        ret = esp_ae_mbc_set_bypass(mbc_handle, band, true);
        AE_DEMO_RET_CHECK(ret, {goto _MBC_EXIT;}, "Failed to set bypass for band");
    }
    ESP_LOGI(TAG, "[ 2 ] Process audio data in chunks and play through codec");
    uint32_t total_samples = 0;
    const uint32_t samples_4s = 4 * SAMPLE_RATE;
    while (1) {
        int bytes_read = read_cb(input_buffer, CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t samples_read = bytes_read / (CHANNELS * BYTES_PER_SAMPLE);
        if (total_samples < samples_4s && (total_samples + samples_read) >= samples_4s) {
            ESP_LOGI(TAG, "Enabling all MBC bands");
            for (int band = 0; band < 4; band++) {
                ret = esp_ae_mbc_set_bypass(mbc_handle, band, false);
                AE_DEMO_RET_CHECK(ret, {goto _MBC_EXIT;}, "Failed to set bypass for band");
            }
        }
        ret = esp_ae_mbc_process(mbc_handle, samples_read, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _MBC_EXIT;}, "MBC process failed");
        int bytes_to_write = samples_read * CHANNELS * BYTES_PER_SAMPLE;
        int written = write_cb(output_buffer, bytes_to_write, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _MBC_EXIT;
        }
        total_samples += samples_read;
    }
    ESP_LOGI(TAG, "MBC demo completed");
_MBC_EXIT:
    if (mbc_handle != NULL) {
        esp_ae_mbc_close(mbc_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER)

typedef struct {
    uint8_t  *data;
    size_t    size;
} tone_stream_data_t;

typedef struct {
    embed_flash_data_t    *tone_stream;
    esp_ae_mixer_handle_t  mixer_handle;
} tone_stream_sender_task_param_t;

static QueueHandle_t tone_stream_queue = NULL;

static esp_ae_err_t ae_demo_mixer_set_fade(esp_ae_mixer_handle_t mixer_handle, uint8_t src_idx, bool fade_in)
{
    if (fade_in) {
        return esp_ae_mixer_set_mode(mixer_handle, src_idx, ESP_AE_MIXER_MODE_FADE_UPWARD);
    } else {
        return esp_ae_mixer_set_mode(mixer_handle, src_idx, ESP_AE_MIXER_MODE_FADE_DOWNWARD);
    }
}

static void tone_stream_sender_task(void *pvParameters)
{
    tone_stream_sender_task_param_t *param = (tone_stream_sender_task_param_t *)pvParameters;
    if (param == NULL) {
        ESP_LOGE(TAG, "tone_stream_sender_task: tone_stream is NULL");
        vTaskDelete(NULL);
        return;
    }
    param->tone_stream->pos = 0;
    vTaskDelay(pdMS_TO_TICKS(4000));
    ae_demo_mixer_set_fade(param->mixer_handle, 0, false);
    ae_demo_mixer_set_fade(param->mixer_handle, 1, true);
    while (1) {
        uint8_t *data = (uint8_t *)heap_caps_aligned_calloc(16, 1, CHUNK_SIZE_BYTES, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for tone stream data");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        tone_stream_data_t chunk = {
            .data = data,
            .size = 0};
        size_t remaining = param->tone_stream->size - param->tone_stream->pos;
        size_t to_read = (CHUNK_SIZE_BYTES < remaining) ? CHUNK_SIZE_BYTES : remaining;
        if (to_read == 0) {
            chunk.size = 0;
            ae_demo_mixer_set_fade(param->mixer_handle, 0, true);
            ae_demo_mixer_set_fade(param->mixer_handle, 1, false);
            if (xQueueSend(tone_stream_queue, &chunk, portMAX_DELAY) != pdTRUE) {
                if (data != NULL) {
                    heap_caps_free(data);
                }
            }
            ESP_LOGI(TAG, "Tone stream sender task finished");
            vTaskDelete(NULL);
            return;
        }
        memcpy(chunk.data, param->tone_stream->data + param->tone_stream->pos, to_read);
        chunk.size = to_read;
        if (xQueueSend(tone_stream_queue, &chunk, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send tone stream data to queue");
            if (data != NULL) {
                heap_caps_free(data);
            }
        }
        param->tone_stream->pos += to_read;
    }
}

int mixer_read_tone_stream(void *buf, size_t size, void *ctx)
{
    if (buf == NULL) {
        return -1;
    }
    tone_stream_data_t chunk = {0};
    if (xQueueReceive(tone_stream_queue, &chunk, 0) != pdTRUE) {
        if (chunk.data != NULL) {
            heap_caps_free(chunk.data);
        }
        return -1;
    }
    if (chunk.size == 0) {
        if (chunk.data != NULL) {
            heap_caps_free(chunk.data);
        }
        return 0;
    }
    int to_copy = chunk.size;
    memcpy(buf, chunk.data, to_copy);
    heap_caps_free(chunk.data);
    return to_copy;
}

static void demo_mixer_process(void)
{
    ESP_LOGI(TAG, "=== Mixer Demo ===");
    int16_t *input_buffer1 = NULL;
    int16_t *input_buffer2 = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_mixer_handle_t mixer_handle = NULL;
    embed_flash_data_t embed_data1 = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    embed_flash_data_t embed_data2 = INIT_EMBED_DATA(tone_pcm_start, tone_pcm_end);
    tone_stream_queue = xQueueCreate(10, sizeof(tone_stream_data_t));
    if (tone_stream_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create tone stream queue");
        return;
    }
    ALLOC_BUFFER(input_buffer1, CHUNK_SIZE_BYTES, _MIXER_EXIT);
    ALLOC_BUFFER(input_buffer2, CHUNK_SIZE_BYTES, _MIXER_EXIT);
    ALLOC_BUFFER(output_buffer, CHUNK_SIZE_BYTES, _MIXER_EXIT);
    esp_ae_sample_t input_buffer[] = {(esp_ae_sample_t)input_buffer1, (esp_ae_sample_t)input_buffer2};
    ESP_LOGI(TAG, "[ 1 ] Initialize Mixer");
    esp_ae_mixer_info_t mixer_info[2] = {
        AE_DEMO_MIXER_INFO_INIT(0.6f, 1.0f, 500),
        AE_DEMO_MIXER_INFO_INIT(0.0f, 0.8f, 200)};
    esp_ae_mixer_cfg_t mixer_cfg = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .src_num = 2,
        .src_info = mixer_info};
    esp_ae_err_t ret = esp_ae_mixer_open(&mixer_cfg, &mixer_handle);
    AE_DEMO_RET_CHECK(ret, {goto _MIXER_EXIT;}, "Failed to create Mixer handle");
    ae_demo_mixer_set_fade(mixer_handle, 0, true);
    ae_demo_mixer_set_fade(mixer_handle, 1, false);
    ESP_LOGI(TAG, "[ 2 ] Process audio data in chunks and play through codec");
    int bytes_read[2] = {0};
    tone_stream_sender_task_param_t tone_stream_sender_task_param = {
        .tone_stream = &embed_data2,
        .mixer_handle = mixer_handle};
    ae_demo_io_cb_t read_cb[] = {ae_demo_read_embed_flash, mixer_read_tone_stream};
    void *read_ctx[] = {&embed_data1, &tone_stream_sender_task_param};
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    xTaskCreate(tone_stream_sender_task, "tone_stream_sender_task", 2048, &tone_stream_sender_task_param, 5, NULL);
    while (1) {
        for (int i = 0; i < mixer_cfg.src_num; i++) {
            bytes_read[i] = read_cb[i](input_buffer[i], CHUNK_SIZE_BYTES, read_ctx[i]);
            if (bytes_read[i] > 0 && bytes_read[i] < CHUNK_SIZE_BYTES) {
                memset((uint8_t *)input_buffer[i] + bytes_read[i], 0, CHUNK_SIZE_BYTES - bytes_read[i]);
            }
        }
        if (bytes_read[0] <= 0 && bytes_read[1] <= 0) {
            ESP_LOGE(TAG, "All streams ended");
            break;
        }
        esp_ae_sample_t in_samples[2] = {
            (bytes_read[0] > 0) ? (esp_ae_sample_t)input_buffer1 : NULL,
            (bytes_read[1] > 0) ? (esp_ae_sample_t)input_buffer2 : NULL};

        ret = esp_ae_mixer_process(mixer_handle, CHUNK_SIZE_BYTES / (CHANNELS * BYTES_PER_SAMPLE), in_samples, (esp_ae_sample_t)output_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _MIXER_EXIT;}, "Mixer process failed");
        int written = write_cb(output_buffer, CHUNK_SIZE_BYTES, audio_codec_handle);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data: %d", written);
            goto _MIXER_EXIT;
        }
    }
    ESP_LOGI(TAG, "Mixer demo completed");
_MIXER_EXIT:
    if (mixer_handle != NULL) {
        esp_ae_mixer_close(mixer_handle);
    }
    if (input_buffer1 != NULL) {
        free(input_buffer1);
    }
    if (input_buffer2 != NULL) {
        free(input_buffer2);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT)

static void demo_basic_audio_info_cvt_process(void)
{
    ESP_LOGI(TAG, "=== Basic Audio Info Convert Demo ===");
    ESP_LOGI(TAG, "Convert: %dHz/%dch/%dbit -> %dHz/%dch/%dbit",
             SRC_SAMPLE_RATE, SRC_CHANNELS, SRC_BITS_PER_SAMPLE,
             SAMPLE_RATE, CHANNELS, BITS_PER_SAMPLE);
    int16_t *input_buffer = NULL;
    int16_t *ch_cvt_buffer = NULL;
    int16_t *bit_cvt_buffer = NULL;
    int16_t *rate_cvt_buffer = NULL;
    int16_t *output_buffer = NULL;
    esp_ae_bit_cvt_handle_t bit_cvt_handle = NULL;
    esp_ae_ch_cvt_handle_t ch_cvt_handle = NULL;
    esp_ae_rate_cvt_handle_t rate_cvt_handle = NULL;
    embed_flash_data_t embed_data = INIT_EMBED_DATA(test_stream_start, test_stream_end);
    ae_demo_io_cb_t read_cb = ae_demo_read_embed_flash;
    ae_demo_io_cb_t write_cb = ae_demo_write_codec;
    ALLOC_BUFFER(input_buffer, SRC_CHUNK_SIZE_BYTES, _CVT_EXIT);
    ALLOC_BUFFER(ch_cvt_buffer, SAMPLES_PER_CHUNK * CHANNELS * SRC_BYTES_PER_SAMPLE, _CVT_EXIT);
    ALLOC_BUFFER(bit_cvt_buffer, SAMPLES_PER_CHUNK * CHANNELS * BYTES_PER_SAMPLE, _CVT_EXIT);
    ALLOC_BUFFER(rate_cvt_buffer, SAMPLES_PER_CHUNK * CHANNELS * BYTES_PER_SAMPLE, _CVT_EXIT);
    ESP_LOGI(TAG, "[ 1 ] Initialize Channel Conversion (2ch -> 1ch, 24bit)");
    esp_ae_ch_cvt_cfg_t ch_cvt_cfg = {
        .sample_rate = SRC_SAMPLE_RATE,
        .bits_per_sample = SRC_BITS_PER_SAMPLE,
        .src_ch = SRC_CHANNELS,
        .dest_ch = CHANNELS,
        .weight = NULL,
        .weight_len = 0};
    esp_ae_err_t ret = esp_ae_ch_cvt_open(&ch_cvt_cfg, &ch_cvt_handle);
    AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Failed to create Channel Conversion handle");
    ESP_LOGI(TAG, "[ 2 ] Initialize Bit Conversion (24bit -> 16bit, 1ch)");
    esp_ae_bit_cvt_cfg_t bit_cvt_cfg = {
        .sample_rate = SRC_SAMPLE_RATE,
        .channel = CHANNELS,
        .src_bits = SRC_BITS_PER_SAMPLE,
        .dest_bits = BITS_PER_SAMPLE};
    ret = esp_ae_bit_cvt_open(&bit_cvt_cfg, &bit_cvt_handle);
    AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Failed to create Bit Conversion handle");
    ESP_LOGI(TAG, "[ 3 ] Initialize Rate Conversion (48kHz -> 16kHz, 1ch, 16bit)");
    esp_ae_rate_cvt_cfg_t rate_cvt_cfg = {
        .src_rate = SRC_SAMPLE_RATE,
        .dest_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
        .complexity = 3,
        .perf_type = ESP_AE_RATE_CVT_PERF_TYPE_MEMORY};
    ret = esp_ae_rate_cvt_open(&rate_cvt_cfg, &rate_cvt_handle);
    AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Failed to create Rate Conversion handle");
    uint32_t max_rate_cvt_samples = 0;
    ret = esp_ae_rate_cvt_get_max_out_sample_num(rate_cvt_handle, SAMPLES_PER_CHUNK, &max_rate_cvt_samples);
    ALLOC_BUFFER(output_buffer, max_rate_cvt_samples * CHANNELS * BYTES_PER_SAMPLE, _CVT_EXIT);
    ESP_LOGI(TAG, "[ 4 ] Process audio data through conversion pipeline");
    while (1) {
        int bytes_read = read_cb(input_buffer, SRC_CHUNK_SIZE_BYTES, &embed_data);
        if (bytes_read <= 0) {
            break;
        }
        uint32_t src_samples = bytes_read / (SRC_CHANNELS * SRC_BYTES_PER_SAMPLE);
        ret = esp_ae_ch_cvt_process(ch_cvt_handle, src_samples, (esp_ae_sample_t)input_buffer, (esp_ae_sample_t)ch_cvt_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Channel conversion failed");
        ret = esp_ae_bit_cvt_process(bit_cvt_handle, src_samples, (esp_ae_sample_t)ch_cvt_buffer, (esp_ae_sample_t)bit_cvt_buffer);
        AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Bit conversion failed");
        uint32_t out_sample_num = max_rate_cvt_samples;
        ret = esp_ae_rate_cvt_process(rate_cvt_handle, (esp_ae_sample_t)bit_cvt_buffer,
                                      src_samples, (esp_ae_sample_t)rate_cvt_buffer, &out_sample_num);
        AE_DEMO_RET_CHECK(ret, {goto _CVT_EXIT;}, "Rate conversion failed");
        if (out_sample_num > 0) {
            int bytes_to_write = out_sample_num * CHANNELS * BYTES_PER_SAMPLE;
            int written = write_cb(rate_cvt_buffer, bytes_to_write, audio_codec_handle);
            if (written < 0) {
                ESP_LOGE(TAG, "Failed to write data: %d", written);
                goto _CVT_EXIT;
            }
        }
    }
    ESP_LOGI(TAG, "Basic Audio Info Convert demo completed");
_CVT_EXIT:
    if (rate_cvt_handle != NULL) {
        esp_ae_rate_cvt_close(rate_cvt_handle);
    }
    if (bit_cvt_handle != NULL) {
        esp_ae_bit_cvt_close(bit_cvt_handle);
    }
    if (ch_cvt_handle != NULL) {
        esp_ae_ch_cvt_close(ch_cvt_handle);
    }
    if (input_buffer != NULL) {
        free(input_buffer);
    }
    if (ch_cvt_buffer != NULL) {
        free(ch_cvt_buffer);
    }
    if (bit_cvt_buffer != NULL) {
        free(bit_cvt_buffer);
    }
    if (rate_cvt_buffer != NULL) {
        free(rate_cvt_buffer);
    }
    if (output_buffer != NULL) {
        free(output_buffer);
    }
}
#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC) */

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing board manager...");
    int ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize codec device");
        return;
    }
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, (void **)&audio_codec_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get audio codec device");
        return;
    }
    esp_codec_dev_close(audio_codec_handle->codec_dev);
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = CHANNELS,
        .bits_per_sample = BITS_PER_SAMPLE,
    };
    ret = esp_codec_dev_open(audio_codec_handle->codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open audio codec device");
        return;
    }
    ret = esp_codec_dev_set_out_vol(audio_codec_handle->codec_dev, 80);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to set DAC volume");
    }
#if defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC)
    ESP_LOGI(TAG, "Running ALC demo...");
    demo_alc_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_FADE)
    ESP_LOGI(TAG, "Running Fade demo...");
    demo_fade_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ)
    ESP_LOGI(TAG, "Running EQ demo...");
    demo_eq_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER)
    ESP_LOGI(TAG, "Running Mixer demo...");
    demo_mixer_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_SONIC)
    ESP_LOGI(TAG, "Running Sonic demo...");
    demo_sonic_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC)
    ESP_LOGI(TAG, "Running DRC demo...");
    demo_drc_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MBC)
    ESP_LOGI(TAG, "Running MBC demo...");
    demo_mbc_process();
#elif defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT)
    ESP_LOGI(TAG, "Running Basic Audio Info Convert demo...");
    demo_basic_audio_info_cvt_process();
#endif  /* defined(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC) */
    esp_board_device_deinit(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    ESP_LOGI(TAG, "Demo finished.");
}
