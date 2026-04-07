/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_hls_helper.h"
#include "esp_extractor_ctrl.h"
#include "esp_log.h"

#define HLS_DEFAULT_OUT_POOL_SIZE  (100 * 1024)
#define HLS_DEFAULT_OUT_ALIGN      (64)
#define TAG                        "HLS_HELPER"

typedef struct {
    FILE *fp;
    bool  is_ref;
} file_src_t;

typedef struct {
    esp_hls_extractor_cfg_t  hls_cfg;
    void                    *playlist_fd;
} hls_file_cfg_t;

static void *file_open(char *url, void *ctx)
{
    hls_file_cfg_t *cfg = (hls_file_cfg_t *)ctx;
    file_src_t *src = calloc(1, sizeof(file_src_t));
    if (src == NULL) {
        return NULL;
    }
    char *origin_url = cfg->hls_cfg.hls_io.m3u8;
    ESP_LOGI(TAG, "Open %s origin %s %p", url, origin_url, cfg->playlist_fd);
    if (strcmp(url, origin_url) == 0) {
        src->fp = cfg->playlist_fd;
        fseek(src->fp, 0, SEEK_SET);
        src->is_ref = true;
    } else {
        src->fp = fopen(url, "rb");
        if (src->fp == NULL) {
            free(src);
            return NULL;
        }
    }
    return src;
}

static int file_reload(void *ctx, char *url)
{
    file_src_t *src = (file_src_t *)ctx;
    if (src == NULL || src->fp == NULL) {
        return -1;
    }
    if (src->is_ref) {
        return 0;
    }
    fclose(src->fp);
    src->fp = fopen(url, "rb");
    return src->fp ? 0 : -1;
}

static int file_read(void *buffer, uint32_t size, void *ctx)
{
    file_src_t *src = (file_src_t *)ctx;
    if (src == NULL || src->fp == NULL) {
        return -1;
    }
    return fread(buffer, 1, size, src->fp);
}

static int file_seek(uint32_t position, void *ctx)
{
    file_src_t *src = (file_src_t *)ctx;
    if (src == NULL || src->fp == NULL) {
        return -1;
    }
    return fseek(src->fp, position, SEEK_SET);
}

static uint32_t file_size(void *ctx)
{
    file_src_t *src = (file_src_t *)ctx;
    if (src == NULL || src->fp == NULL) {
        return 0;
    }
    long old = ftell(src->fp);
    fseek(src->fp, 0, SEEK_END);
    long s = ftell(src->fp);
    fseek(src->fp, old, SEEK_SET);
    return s > 0 ? (uint32_t)s : 0;
}

static int file_close(void *ctx)
{
    file_src_t *src = (file_src_t *)ctx;
    if (src) {
        if (src->fp && !src->is_ref) {
            fclose(src->fp);
        }
        free(src);
    }
    return 0;
}

static int origin_read(void *buffer, uint32_t size, void *ctx)
{
    hls_file_cfg_t *cfg = (hls_file_cfg_t *)ctx;
    return fread(buffer, 1, size, cfg->playlist_fd);
}

static int origin_seek(uint32_t position, void *ctx)
{
    hls_file_cfg_t *cfg = (hls_file_cfg_t *)ctx;
    printf("Seek to %d cur %d\n", (int)position, (int)ftell(cfg->playlist_fd));
    return fseek(cfg->playlist_fd, position, SEEK_SET);
}

static uint32_t origin_size(void *ctx)
{
    hls_file_cfg_t *cfg = (hls_file_cfg_t *)ctx;
    long old = ftell(cfg->playlist_fd);
    fseek(cfg->playlist_fd, 0, SEEK_END);
    long s = ftell(cfg->playlist_fd);
    fseek(cfg->playlist_fd, old, SEEK_SET);
    return s > 0 ? (uint32_t)s : 0;
}

void esp_hls_extractor_file_cfg_deinit(esp_hls_extractor_cfg_t *hls_cfg)
{
    if (hls_cfg == NULL) {
        return;
    }
    hls_file_cfg_t *cfg = (hls_file_cfg_t *)hls_cfg;
    if (cfg->playlist_fd) {
        fclose(cfg->playlist_fd);
        cfg->playlist_fd = NULL;
    }
    if (hls_cfg->hls_io.m3u8) {
        free(hls_cfg->hls_io.m3u8);
        hls_cfg->hls_io.m3u8 = NULL;
    }
    free(hls_cfg);
}

esp_hls_extractor_cfg_t *esp_hls_extractor_file_cfg_init(const char *url,
                                                         uint8_t extract_mask,
                                                         uint32_t out_pool_size,
                                                         uint16_t out_align)
{
    hls_file_cfg_t *cfg = calloc(1, sizeof(hls_file_cfg_t));
    if (cfg == NULL) {
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
    do {
        cfg->playlist_fd = fopen(url, "rb");
        if (cfg->playlist_fd == NULL) {
            ESP_LOGE(TAG, "Failed to open file %s", url);
            break;
        }
        hls_io->m3u8 = strdup((char *)url);
        if (hls_io->m3u8 == NULL) {
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
    esp_hls_extractor_file_cfg_deinit(&cfg->hls_cfg);
    return NULL;
}

esp_extractor_err_t esp_hls_extractor_open_with_cfg(esp_hls_extractor_cfg_t *cfg,
                                                    esp_extractor_handle_t *extractor)
{
    if (cfg == NULL || extractor == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    esp_extractor_err_t ret = esp_extractor_open(&cfg->base_cfg, extractor);
    if (ret != ESP_EXTRACTOR_ERR_OK) {
        return ret;
    }
    ret = esp_extractor_ctrl(*extractor, ESP_EXTRACTOR_CTRL_TYPE_SET_HLS_IO_EX,
                             &cfg->hls_io, sizeof(cfg->hls_io));
    if (ret != ESP_EXTRACTOR_ERR_OK) {
        esp_extractor_close(*extractor);
        *extractor = NULL;
        return ret;
    }
    return ESP_EXTRACTOR_ERR_OK;
}
