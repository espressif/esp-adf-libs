/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include "esp_hls_helper.h"
#include "esp_gmf_io.h"
#include "esp_gmf_pool.h"
#include "esp_log.h"

#define HLS_DEFAULT_OUT_POOL_SIZE  (100 * 1024)
#define HLS_DEFAULT_OUT_ALIGN      (64)
#define HLS_READ_TIMEOUT           (5000)
#define TAG                        "HLS_IO_HELPER"

typedef struct {
    esp_gmf_io_handle_t  fd;
    bool                 is_ref;
} io_src_t;

typedef struct {
    esp_hls_extractor_cfg_t  hls_cfg;
    esp_gmf_pool_handle_t    pool;
    esp_gmf_io_handle_t      playlist_fd;
} hls_io_cfg_t;

static int file_close(void *ctx);

static char *get_matched_io(esp_gmf_pool_handle_t pool, char *url)
{
    char io_scheme[32];
    strncpy(io_scheme, url, sizeof(io_scheme) - 1);
    io_scheme[sizeof(io_scheme) - 1] = 0;
    char *ext = strrchr(url, '/');
    if (ext) {
        int pos = ext - url;
        if (pos < sizeof(io_scheme) - 1) {
            io_scheme[pos] = 0;
        }
    }
    char *io_tag = NULL;
    esp_gmf_pool_get_io_tag_by_url(pool, io_scheme, ESP_GMF_IO_DIR_READER, &io_tag);
    return io_tag;
}

static void *file_open(char *url, void *ctx)
{
    hls_io_cfg_t *cfg = (hls_io_cfg_t *)ctx;
    char *io_tag = get_matched_io(cfg->pool, url);
    if (io_tag == NULL) {
        ESP_LOGE(TAG, "Not supported url %s", url);
        return NULL;
    }
    io_src_t *src = calloc(1, sizeof(io_src_t));
    if (src == NULL) {
        return NULL;
    }
    char *origin_url = cfg->hls_cfg.hls_io.m3u8;
    ESP_LOGI(TAG, "Open %s origin %s %p", url, origin_url, cfg->playlist_fd);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    do {
        if (strcmp(url, origin_url) == 0) {
            ret = esp_gmf_io_reload(cfg->playlist_fd, url);
            if (ret != ESP_GMF_ERR_OK) {
                ESP_LOGE(TAG, "Failed to reload %s ret %d", url, ret);
                break;
            }
            src->fd = cfg->playlist_fd;
            src->is_ref = true;
        } else {
            esp_gmf_io_handle_t fd = NULL;
            esp_gmf_pool_new_io(cfg->pool, io_tag, ESP_GMF_IO_DIR_READER, &fd);
            if (fd == NULL) {
                break;
            }
            esp_gmf_io_set_uri(fd, url);
            src->fd = fd;
            ret = esp_gmf_io_open(fd);
            if (ret != 0) {
                ESP_LOGE(TAG, "Failed to open %s ret %d", url, ret);
                return NULL;
            }
        }
        return src;
    } while (0);
    file_close(src);
    return NULL;
}

static int file_reload(void *ctx, char *url)
{
    io_src_t *src = (io_src_t *)ctx;
    if (src == NULL || src->fd == NULL) {
        return -1;
    }
    ESP_LOGI(TAG, "Reload %s", url);
    return esp_gmf_io_reload(src->fd, url);
}

static int read_from_io(esp_gmf_io_handle_t io, void *buffer, uint32_t size)
{
    bool is_done = false;
    int fill_size = 0;
    while (!is_done && fill_size < size) {
        int buf_len = size - fill_size;
        esp_gmf_payload_t payload = {
            .buf = buffer,
            .buf_length = buf_len,
        };
        esp_gmf_err_io_t ret = esp_gmf_io_acquire_read(io, &payload, buf_len, HLS_READ_TIMEOUT);
        if (ret != ESP_GMF_IO_OK) {
            ESP_LOGE(TAG, "Failed to read ret %d", ret);
            return -1;
        }
        if (payload.buf != buffer) {
            memcpy(buffer, payload.buf, payload.valid_size);
        }
        esp_gmf_io_release_read(io, &payload, 0);
        fill_size += payload.valid_size;
        buffer += payload.valid_size;
        is_done = payload.is_done;
    }
    return fill_size;
}

static int file_read(void *buffer, uint32_t size, void *ctx)
{
    io_src_t *src = (io_src_t *)ctx;
    if (src == NULL || src->fd == NULL) {
        return -1;
    }
    return read_from_io(src->fd, buffer, size);
}

static int file_seek(uint32_t position, void *ctx)
{
    io_src_t *src = (io_src_t *)ctx;
    if (src == NULL || src->fd == NULL) {
        return -1;
    }
    esp_gmf_info_file_t info = {};
    esp_gmf_io_get_info(src->fd, &info);
    ESP_LOGI(TAG, "Seek from %d to position %d", (int)info.pos, (int)position);
    return esp_gmf_io_seek(src->fd, position);
}

static uint32_t file_size(void *ctx)
{
    io_src_t *src = (io_src_t *)ctx;
    if (src == NULL || src->fd == NULL) {
        return 0;
    }
    uint64_t total_size = 0;
    esp_gmf_io_get_size(src->fd, &total_size);
    return (uint32_t)total_size;
}

static int file_close(void *ctx)
{
    io_src_t *src = (io_src_t *)ctx;
    if (src == NULL) {
        return -1;
    }
    if (src->fd) {
        esp_gmf_io_close(src->fd);
        if (!src->is_ref) {
            esp_gmf_obj_delete(src->fd);
            src->fd = NULL;
        }
    }
    free(src);
    return 0;
}

static int origin_pending_open(hls_io_cfg_t *cfg)
{
    if (cfg->playlist_fd == NULL) {
        esp_gmf_io_handle_t fd = NULL;
        char *url = cfg->hls_cfg.hls_io.m3u8;
        char *io_tag = get_matched_io(cfg->pool, url);
        if (io_tag == NULL) {
            ESP_LOGE(TAG, "Not supported url %s", url);
            return -1;
        }
        esp_gmf_pool_new_io(cfg->pool, io_tag, ESP_GMF_IO_DIR_READER, &fd);
        if (fd == NULL) {
            ESP_LOGE(TAG, "Failed to init IO for %s", url);
            return -1;
        }
        esp_gmf_io_reset(fd);
        esp_gmf_io_set_uri(fd, url);
        esp_gmf_err_t ret = esp_gmf_io_open(fd);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to open %s ret %d", url, ret);
            return ret;
        }
        cfg->playlist_fd = fd;
    }
    return 0;
}

static int origin_read(void *buffer, uint32_t size, void *ctx)
{
    hls_io_cfg_t *cfg = (hls_io_cfg_t *)ctx;
    origin_pending_open(cfg);
    if (cfg->playlist_fd == NULL) {
        return -1;
    }
    return read_from_io(cfg->playlist_fd, buffer, size);
}

static int origin_seek(uint32_t position, void *ctx)
{
    hls_io_cfg_t *cfg = (hls_io_cfg_t *)ctx;
    origin_pending_open(cfg);
    if (cfg->playlist_fd == NULL) {
        return -1;
    }
    return esp_gmf_io_seek(cfg->playlist_fd, position);
}

static uint32_t origin_size(void *ctx)
{
    hls_io_cfg_t *cfg = (hls_io_cfg_t *)ctx;
    origin_pending_open(cfg);
    if (cfg->playlist_fd == NULL) {
        return 0;
    }
    uint64_t total_size = 0;
    esp_gmf_io_get_size(cfg->playlist_fd, &total_size);
    return (uint32_t)total_size;
}

void esp_hls_extractor_io_cfg_deinit(esp_hls_extractor_cfg_t *hls_cfg)
{
    if (hls_cfg == NULL) {
        return;
    }
    hls_io_cfg_t *cfg = (hls_io_cfg_t *)hls_cfg;
    if (cfg->playlist_fd) {
        esp_gmf_io_close(cfg->playlist_fd);
        esp_gmf_obj_delete(cfg->playlist_fd);
        cfg->playlist_fd = NULL;
    }
    if (hls_cfg->hls_io.m3u8) {
        free(hls_cfg->hls_io.m3u8);
        hls_cfg->hls_io.m3u8 = NULL;
    }
    free(hls_cfg);
}

esp_hls_extractor_cfg_t *esp_hls_extractor_io_cfg_init(const char *url,
                                                       void *gmf_pool,
                                                       uint8_t extract_mask,
                                                       uint32_t out_pool_size,
                                                       uint16_t out_align)
{
    hls_io_cfg_t *cfg = calloc(1, sizeof(hls_io_cfg_t));
    if (cfg == NULL) {
        ESP_LOGE(TAG, "No memory for HLS config");
        return NULL;
    }
    esp_hls_stream_io_t *hls_io = &cfg->hls_cfg.hls_io;
    hls_io->open = file_open;
    hls_io->reload = file_reload;
    hls_io->read = file_read;
    hls_io->seek = file_seek;
    hls_io->get_file_size = file_size;
    hls_io->close = file_close;
    hls_io->input_ctx = cfg;
    cfg->pool = gmf_pool;
    do {
        hls_io->m3u8 = strdup((char *)url);
        if (hls_io->m3u8 == NULL) {
            ESP_LOGE(TAG, "No memory for m3u8 URL");
            break;
        }
        cfg->hls_cfg.base_cfg.type = ESP_EXTRACTOR_TYPE_HLS;
        cfg->hls_cfg.base_cfg.extract_mask = extract_mask;
        cfg->hls_cfg.base_cfg.in_read_cb = origin_read;
        cfg->hls_cfg.base_cfg.in_seek_cb = origin_seek;
        cfg->hls_cfg.base_cfg.in_size_cb = origin_size;
        cfg->hls_cfg.base_cfg.in_ctx = cfg;
        cfg->hls_cfg.base_cfg.out_pool_size = out_pool_size ? out_pool_size : HLS_DEFAULT_OUT_POOL_SIZE;
        cfg->hls_cfg.base_cfg.out_align = out_align ? out_align : HLS_DEFAULT_OUT_ALIGN;
        return &cfg->hls_cfg;
    } while (0);
    esp_hls_extractor_io_cfg_deinit(&cfg->hls_cfg);
    return NULL;
}
