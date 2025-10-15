/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#define TAG "AE_TEST_COMMON"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "test_utils.h"
#include "unity.h"
#include "esp_log.h"
#include "test_common.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

static esp_err_t ae_http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t ae_http_download_init(ae_http_context_t *ctx, const char *filename, char *download_url)
{
    char url[256];
    snprintf(url, sizeof(url), "%s/%s.wav", download_url, filename);
    ESP_LOGI(TAG, "Initializing HTTP download from: %s", url);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 150 * 1000,
    };
    ctx->client = esp_http_client_init(&config);
    TEST_ASSERT_NOT_NULL(ctx->client);
    esp_err_t err = esp_http_client_open(ctx->client, 0);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    int content_length = esp_http_client_fetch_headers(ctx->client);
    TEST_ASSERT_GREATER_THAN(0, content_length);
    ctx->buffer_size = content_length;
    ctx->buffer = NULL;
    ctx->buffer_pos = 0;
    ctx->total_size = content_length;
    ctx->current_pos = 0;
    ctx->is_upload = false;
    ESP_LOGI(TAG, "Download initialized for streaming, content length: %d", content_length);
    return ESP_OK;
}

esp_err_t ae_http_upload_init(ae_http_context_t *ctx, const char *filename, ae_audio_info_t *audio_info, char *upload_url)
{
    char url[256];
    snprintf(url, sizeof(url), "%s", upload_url);
    ESP_LOGI(TAG, "Initializing HTTP upload to: %s", url);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = ae_http_event_handler,
        .timeout_ms = 20 * 1000,
        .buffer_size_tx = 40960,
    };
    ctx->client = esp_http_client_init(&config);
    TEST_ASSERT_NOT_NULL(ctx->client);

    char data[10] = {0};
    snprintf(data, sizeof(data), "%d", audio_info->sample_rate);
    esp_http_client_set_method(ctx->client, HTTP_METHOD_POST);
    esp_err_t ret = esp_http_client_set_header(ctx->client, "x-audio-sample-rates", data);
    TEST_ASSERT_EQUAL(ret, ESP_OK);
    ret = esp_http_client_set_header(ctx->client, "Content-Type", "audio/pcm");
    TEST_ASSERT_EQUAL(ret, ESP_OK);
    snprintf(data, sizeof(data), "%d", audio_info->bits_per_sample);
    ret = esp_http_client_set_header(ctx->client, "x-audio-bits", data);
    TEST_ASSERT_EQUAL(ret, ESP_OK);
    snprintf(data, sizeof(data), "%d", audio_info->channels);
    ret = esp_http_client_set_header(ctx->client, "x-audio-channel", data);
    TEST_ASSERT_EQUAL(ret, ESP_OK);
    char disposition_header[512];
    ret = snprintf(disposition_header, sizeof(disposition_header), "%s", filename);
    ret = esp_http_client_set_header(ctx->client, "X-Filename", disposition_header);

    printf("disposition_header: %s\n", disposition_header);
    if (ret >= sizeof(disposition_header)) {
        ESP_LOGW(TAG, "Filename too long, truncated");
    }
    esp_err_t err = esp_http_client_open(ctx->client, -1);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    ctx->buffer = NULL;
    ctx->buffer_size = 0;
    ctx->buffer_pos = 0;
    ctx->total_size = 0;
    ctx->current_pos = 0;
    ctx->is_upload = true;
    return ESP_OK;
}

int ae_http_read(void *ptr, size_t size, size_t count, ae_http_context_t *ctx)
{
    if (!ctx || ctx->is_upload) {
        return -1;
    }
    if (ctx->current_pos >= ctx->total_size) {
        return -1;
    }
    size_t bytes_to_read = size * count;
    size_t bytes_available = ctx->total_size - ctx->current_pos;
    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
    }
    if (bytes_to_read > 0 && ctx->client) {
        int read_len = esp_http_client_read(ctx->client, ptr, bytes_to_read);
        if (read_len <= 0) {
            ESP_LOGE(TAG, "Error reading data from HTTP stream");
            return -1;
        }
        ctx->current_pos += read_len;
        return read_len / size;
    }
    return -1;
}

int ae_http_write(const void *ptr, size_t size, size_t count, bool is_finish, ae_http_context_t *ctx)
{
    if (!ctx || ctx->is_upload == false) {
        return -1;
    }
    
    if (is_finish) {
        int written = esp_http_client_write(ctx->client, "0\r\n\r\n", 5);
        printf("written finish\n");
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write end chunked marker");
            return -1;
        }
        
        // Fetch response headers to trigger HTTP_EVENT_ON_FINISH
        int content_length = esp_http_client_fetch_headers(ctx->client);
        int status_code = esp_http_client_get_status_code(ctx->client);
        ESP_LOGI(TAG, "HTTP Upload completed - Status: %d, Content-Length: %d",
                 status_code, content_length);
        
        return 1; // Return success
    }
    
    size_t bytes_to_write = size * count;
    if (ctx->client && bytes_to_write > 0) {
        // Write chunk size header
        char header_chunk_buffer[16] = {0};
        int header_chunk_len = snprintf(header_chunk_buffer, sizeof(header_chunk_buffer),
                                        "%x\r\n", bytes_to_write);
        if (header_chunk_len < 0) {
            ESP_LOGE(TAG, "Failed to create header chunk");
            return -1;
        }
        
        int written = esp_http_client_write(ctx->client, header_chunk_buffer, header_chunk_len);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write header chunk");
            return -1;
        }
        written = esp_http_client_write(ctx->client, ptr, bytes_to_write);
        if (written < 0) {
            ESP_LOGE(TAG, "Failed to write data to HTTP stream: %d", written);
            return -1;
        }
        if (written != (int)bytes_to_write) {
            ESP_LOGW(TAG, "Partial write: expected %d, got %d", bytes_to_write, written);
        }
        written = esp_http_client_write(ctx->client, "\r\n", 2);
        if (written <= 0) {
            ESP_LOGE(TAG, "Error write chunked tail, write_len=%d", written);
            return -1;
        }

        ctx->total_size += written;
        return written / size;
    }
    return -1;
}

void ae_http_deinit(ae_http_context_t *ctx)
{
    if (ctx) {
        if (ctx->client) {
            esp_http_client_close(ctx->client);
            esp_http_client_cleanup(ctx->client);
            ctx->client = NULL;
        }
        memset(ctx, 0, sizeof(ae_http_context_t));
    }
}

void ae_http_parse_wav_header(ae_http_context_t *ctx, int *sample_rate, int *channels,
                              int *bits_per_sample, long *data_offset, uint32_t *data_size)
{
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    int found_fmt = 0, found_data = 0;
    long current_pos = 0;
    while (!found_fmt || !found_data) {
        if (ae_http_read(chunk_id, 1, 4, ctx) != 4) {
            ESP_LOGE(TAG, "Failed to read chunk ID");
            break;
        }
        current_pos += 4;
        if (ae_http_read(&chunk_size, 1, 4, ctx) != 4) {
            ESP_LOGE(TAG, "Failed to read chunk size");
            break;
        }
        current_pos += 4;
        if (memcmp(chunk_id, "RIFF", 4) == 0) {
            char wave[4];
            if (ae_http_read(wave, 1, 4, ctx) != 4) {
                ESP_LOGE(TAG, "Failed to read WAVE identifier");
                break;
            }
            current_pos += 4;
            continue;
        }
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            uint16_t audio_format, num_channels, bits;
            uint32_t sample_rate_val;
            uint32_t byte_rate;
            uint16_t block_align;
            if (ae_http_read(&audio_format, 1, 2, ctx) != 2) {
                break;
            }
            if (ae_http_read(&num_channels, 1, 2, ctx) != 2) {
                break;
            }
            if (ae_http_read(&sample_rate_val, 1, 4, ctx) != 4) {
                break;
            }
            if (ae_http_read(&byte_rate, 1, 4, ctx) != 4) {
                break;
            }
            if (ae_http_read(&block_align, 1, 2, ctx) != 2) {
                break;
            }
            if (ae_http_read(&bits, 1, 2, ctx) != 2) {
                break;
            }
            *sample_rate = sample_rate_val;
            *channels = num_channels;
            *bits_per_sample = bits;
            found_fmt = 1;
            current_pos += 16;
            if (chunk_size > 16) {
                char skip_buffer[256];
                uint32_t remaining = chunk_size - 16;
                while (remaining > 0) {
                    uint32_t to_skip = (remaining > sizeof(skip_buffer)) ? sizeof(skip_buffer) : remaining;
                    if (ae_http_read(skip_buffer, 1, to_skip, ctx) != to_skip) {
                        break;
                    }
                    remaining -= to_skip;
                    current_pos += to_skip;
                }
            }
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            if (data_offset) {
                *data_offset = current_pos;
            }
            if (data_size) {
                *data_size = chunk_size;
            }
            found_data = 1;
            break;
        } else {
            char skip_buffer[256];
            uint32_t remaining = chunk_size;
            while (remaining > 0) {
                uint32_t to_skip = (remaining > sizeof(skip_buffer)) ? sizeof(skip_buffer) : remaining;
                if (ae_http_read(skip_buffer, 1, to_skip, ctx) != to_skip) {
                    break;
                }
                remaining -= to_skip;
                current_pos += to_skip;
            }
        }
    }
    TEST_ASSERT_EQUAL(found_fmt, 1);
    TEST_ASSERT_EQUAL(found_data, 1);
}
