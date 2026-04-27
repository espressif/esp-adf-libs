/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <string.h>
#include "esp_gmf_new_databus.h"
#include "media_lib_os.h"
#include "esp_hls_io.h"
#include "hls_fetcher.h"
#include "esp_log.h"

#define TAG  "HLS_IO"

#define HLS_MULTIPLE_IO_INST  (0)
#define HLS_MAX_RETRY_COUNT   (200)
#define HLS_READ_TIMEOUT      (5000)
#define HLS_READ_BLOCK_SIZE   (512)
#define IS_WORD(c)            ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))

typedef struct _hls_io_t {
    esp_gmf_io_t          base;
    hls_fetcher_handle_t  hls_fetcher;
    bool                  aborted;
    esp_gmf_io_handle_t   io;
    esp_gmf_db_handle_t   data_bus;
    char                 *prev_io_tag;
    uint32_t              media_remain;  // Remaining bytes of the current media chunk on data_bus
} hls_io_t;

typedef struct {
    hls_io_t            *hls_io;
    esp_gmf_io_handle_t  io;
} cached_io_t;

typedef struct {
    bool      bos;
    uint32_t  format;
    uint32_t  valid_size;
} hls_meta_info_t;

static int cache_close(void *ctx)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    if (cached_io == NULL) {
        return -1;
    }
#if HLS_MULTIPLE_IO_INST
    if (cached_io->io) {
        esp_gmf_io_close(cached_io->io);
        esp_gmf_obj_delete(cached_io->io);
        cached_io->io = NULL;
    }
#endif  /* HLS_MULTIPLE_IO_INST */
    esp_gmf_oal_free(cached_io);
    return 0;
}

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

static void *cache_open(char *url, void *input_ctx)
{
    hls_io_t *hls_io = (hls_io_t *)input_ctx;
    esp_hls_io_cfg_t *cfg = (esp_hls_io_cfg_t *)OBJ_GET_CFG(hls_io);
    cached_io_t *cached_io = esp_gmf_oal_calloc(1, sizeof(cached_io_t));
    ESP_GMF_MEM_VERIFY(TAG, cached_io, return NULL, "cached io", sizeof(cached_io_t));
    esp_hls_file_type_t type = ESP_HLS_FILE_TYPE_AUDIO;
    hls_fetcher_get_file_type(hls_io->hls_fetcher, url, &type);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    do {
        char *io_tag = get_matched_io(cfg->pool, url);
        if (io_tag == NULL) {
            ESP_LOGE(TAG, "Not supported url %s", url);
            break;
        }
        esp_gmf_io_handle_t new_io = NULL;
#if !HLS_MULTIPLE_IO_INST
        if (io_tag == hls_io->prev_io_tag) {
            ret = esp_gmf_io_reload(hls_io->io, url);
            if (ret != ESP_GMF_ERR_OK) {
                ESP_LOGE(TAG, "Failed to reload %s ret %d", url, ret);
            } else {
                cached_io->io = hls_io->io;
            }
            break;
        }
        if (hls_io->io) {
            esp_gmf_io_close(hls_io->io);
            esp_gmf_io_deinit(hls_io->io);
            hls_io->io = NULL;
        }
#endif  /* !HLS_MULTIPLE_IO_INST */
        esp_gmf_pool_new_io(cfg->pool, io_tag, ESP_GMF_IO_DIR_READER, &new_io);
        if (new_io == NULL) {
            ESP_LOGE(TAG, "Failed to create io %s", io_tag);
            ret = ESP_GMF_ERR_FAIL;
            break;
        }
        // Configuration according user setting
        if (cfg->get_io_cfg_cb) {
            esp_gmf_io_cfg_t io_cfg = {0};
            ret = cfg->get_io_cfg_cb(type, &io_cfg, cfg->ctx);
            if (ret == 0) {
                esp_gmf_io_init(new_io, &io_cfg);
            }
        }
#if !HLS_MULTIPLE_IO_INST
        if (hls_io->io == NULL) {
            hls_io->io = new_io;
        }
        hls_io->prev_io_tag = io_tag;
#endif  /* !HLS_MULTIPLE_IO_INST */
        cached_io->io = new_io;
        esp_gmf_io_reset(new_io);
        esp_gmf_io_set_uri(new_io, url);
        ret = esp_gmf_io_open(new_io);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to open %s ret %d", url, ret);
            break;
        }
    } while (0);
    if (ret == ESP_GMF_ERR_OK) {
        return cached_io;
    }
    cache_close(cached_io);
    return NULL;
}

static int cache_reload(void *ctx, char *url)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    ESP_LOGI(TAG, "Reload %s", url);
    return esp_gmf_io_reload(cached_io->io, url);
}

static int cache_read(void *buffer, uint32_t size, void *ctx)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    if (cached_io == NULL || cached_io->io == NULL) {
        return -1;
    }
    bool is_done = false;
    int fill_size = 0;
    while (!is_done && fill_size < size) {
        int buf_len = size - fill_size;
        esp_gmf_payload_t payload = {
            .buf = buffer,
            .buf_length = buf_len,
        };
        esp_gmf_err_io_t ret = esp_gmf_io_acquire_read(cached_io->io, &payload, buf_len, HLS_READ_TIMEOUT);
        if (ret != ESP_GMF_IO_OK) {
            ESP_LOGE(TAG, "Failed to read ret %d", ret);
            return -1;
        }
        if (payload.buf != buffer) {
            memcpy(buffer, payload.buf, payload.valid_size);
        }
        esp_gmf_io_release_read(cached_io->io, &payload, 0);
        fill_size += payload.valid_size;
        buffer += payload.valid_size;
        is_done = payload.is_done;
    }
    return fill_size;
}

static int cache_abort(void *ctx)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    if (cached_io == NULL || cached_io->io == NULL) {
        return -1;
    }
    esp_gmf_io_t *io = (esp_gmf_io_t *)cached_io->io;
    if (io->prev_close) {
        io->prev_close(io);
    }
    return 0;
}

static int cache_seek(uint32_t position, void *ctx)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    if (cached_io == NULL || cached_io->io == NULL) {
        return -1;
    }
    return esp_gmf_io_seek(cached_io->io, position);
}

static uint32_t cache_file_size(void *ctx)
{
    cached_io_t *cached_io = (cached_io_t *)ctx;
    if (cached_io == NULL || cached_io->io == NULL) {
        return 0;
    }
    uint64_t size = 0;
    esp_gmf_io_get_size(cached_io->io, &size);
    return (uint32_t)size;
}

static void hls_build_data_io(hls_io_t *io, esp_hls_stream_io_t *data_io)
{
    memset(data_io, 0, sizeof(esp_hls_stream_io_t));
    data_io->open = cache_open;
    data_io->read = cache_read;
    data_io->read_abort = cache_abort;
    data_io->reload = cache_reload;
    data_io->seek = cache_seek;
    data_io->get_file_size = cache_file_size;
    data_io->close = cache_close;
    data_io->input_ctx = io;
}

static esp_gmf_err_t _hls_new(void *cfg, esp_gmf_obj_handle_t *io)
{
    return esp_gmf_io_hls_init(cfg, io);
}

static esp_gmf_err_t _hls_get_score(esp_gmf_io_handle_t obj, const char *url, int *score)
{
    *score = 0;
    const char *file_name = strrchr(url, '/');
    if (file_name == NULL) {
        file_name = url;
    } else {
        file_name++;
    }
    int ext_len = strlen(".m3u8");
    char *matched = strstr(file_name, ".m3u8");
    if (matched == NULL) {
        matched = strstr(file_name, ".M3U8");
    }
    if (matched && !IS_WORD(matched[ext_len])) {
        *score = ESP_GMF_IO_SCORE_PERFECT;
        return ESP_GMF_ERR_OK;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _hls_open(esp_gmf_io_handle_t io)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    char *uri = NULL;
    esp_gmf_io_get_uri((esp_gmf_io_handle_t)hls_io, &uri);
    if (uri == NULL) {
        ESP_LOGE(TAG, "Error, uri is not set, handle: %p", io);
        return ESP_GMF_ERR_FAIL;
    }
    ESP_LOGI(TAG, "Open uri:%s", uri);

    esp_hls_io_cfg_t *cfg = (esp_hls_io_cfg_t *)OBJ_GET_CFG(hls_io);

    hls_fetch_cfg_t fetch_cfg = {
        .extract_mask = ESP_EXTRACT_MASK_AUDIO,
        .ctx = cfg->ctx,
    };

    hls_build_data_io(hls_io, &fetch_cfg.io);
    fetch_cfg.io.m3u8 = uri;
    int ret = hls_fetcher_open(&fetch_cfg, &hls_io->hls_fetcher);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to open fetcher ret %d", ret);
        return ESP_GMF_ERR_FAIL;
    }
    hls_fetcher_enable_stream(hls_io->hls_fetcher, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, true);
    return ESP_GMF_ERR_OK;
}

static int checksum(uint8_t *buf, int size)
{
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += buf[i];
    }
    return sum;
}

static esp_gmf_err_io_t _hls_pull_next_meta(hls_io_t *hls_io, int block_ticks, uint32_t *out_remain)
{
    const int meta_hdr_bytes = (int)sizeof(hls_meta_info_t);
    esp_gmf_data_bus_block_t head_blk = {};
    hls_meta_info_t meta_info = {};

    while (true) {
        esp_gmf_err_io_t ret = esp_gmf_db_acquire_read(hls_io->base.data_bus, &head_blk, meta_hdr_bytes, block_ticks);
        if (ret != ESP_GMF_IO_OK) {
            return ret;
        }
        memcpy(&meta_info, head_blk.buf, sizeof(meta_info));
        esp_gmf_io_update_pos((esp_gmf_io_handle_t)hls_io, head_blk.valid_size);
        esp_gmf_db_release_read(hls_io->base.data_bus, &head_blk, 0);

        if (meta_info.bos) {
            esp_hls_io_cfg_t *cfg = (esp_hls_io_cfg_t *)OBJ_GET_CFG(hls_io);
            if (cfg->file_seg_cb) {
                esp_hls_file_seg_info_t seg_info = {
                    .format = meta_info.format,
                };
                cfg->file_seg_cb(&seg_info, cfg->ctx);
            }
        }
        if (meta_info.valid_size > HLS_READ_BLOCK_SIZE) {
            ESP_LOGE(TAG, "Invalid meta data size %u", (unsigned)meta_info.valid_size);
            return ESP_GMF_IO_FAIL;
        }
        if (meta_info.valid_size == 0) {
            continue;  /* skip empty slot, wait for next meta */
        }
        *out_remain = meta_info.valid_size;
        return ESP_GMF_IO_OK;
    }
}

static esp_gmf_err_t _hls_user_read_filter(esp_gmf_io_handle_t handle, void *payload, uint32_t wanted_size, int block_ticks)
{
    hls_io_t *hls_io = (hls_io_t *)handle;
    esp_gmf_payload_t *load = (esp_gmf_payload_t *)payload;
    if (hls_io == NULL || hls_io->hls_fetcher == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    if (wanted_size == 0) {
        load->valid_size = 0;
        return ESP_GMF_IO_OK;
    }

    uint32_t max_copy = wanted_size;
    if (load->buf_length > 0 && max_copy > load->buf_length) {
        max_copy = load->buf_length;
    }

    // Mode 1: payload has buffer (copy mode)
    if (load->buf != NULL) {
        uint8_t *dst_buf = (uint8_t *)load->buf;
        uint32_t total_copied = 0;
        bool any_is_done = false;
        esp_gmf_err_io_t ret = ESP_GMF_IO_OK;

        while (total_copied < max_copy) {
            if (hls_io->media_remain == 0) {
                ret = _hls_pull_next_meta(hls_io, block_ticks, &hls_io->media_remain);
                if (ret != ESP_GMF_IO_OK) {
                    /* Nothing more we can pull; report what we've already got. */
                    if (total_copied == 0) {
                        return ret;
                    }
                    break;
                }
            }

            uint32_t request_len = max_copy - total_copied;
            if (request_len > hls_io->media_remain) {
                request_len = hls_io->media_remain;
            }

            esp_gmf_data_bus_block_t piece = {};
            ret = esp_gmf_db_acquire_read(hls_io->base.data_bus, &piece, request_len, block_ticks);
            if (ret != ESP_GMF_IO_OK) {
                if (total_copied == 0) {
                    return ret;
                }
                break;
            }

            uint32_t got = piece.valid_size;
            if (got > hls_io->media_remain) {
                got = hls_io->media_remain;
            }
            memcpy(dst_buf + total_copied, piece.buf, got);
            total_copied += got;
            hls_io->media_remain -= got;
            bool segment_end = (hls_io->media_remain == 0) && piece.is_last;
            any_is_done = any_is_done || segment_end;
            bool filled_enough = segment_end || (total_copied >= max_copy);
            esp_gmf_db_release_read(hls_io->base.data_bus, &piece, 0);
            if (filled_enough) {
                break;
            }
        }

        memset(&hls_io->base.db_block, 0, sizeof(hls_io->base.db_block));
        load->valid_size = total_copied;
        load->is_done = any_is_done;
        return ESP_GMF_IO_OK;
    }

    // Mode 2: payload has no buffer (zero-copy mode)
    if (hls_io->media_remain == 0) {
        esp_gmf_err_io_t ret = _hls_pull_next_meta(hls_io, block_ticks, &hls_io->media_remain);
        if (ret != ESP_GMF_IO_OK) {
            return ret;
        }
    }

    esp_gmf_err_io_t ret = esp_gmf_db_acquire_read(hls_io->base.data_bus, payload, hls_io->media_remain, block_ticks);
    if (ret != ESP_GMF_IO_OK) {
        return ret;
    }
    esp_gmf_data_bus_block_t *data_blk = (esp_gmf_data_bus_block_t *)payload;
    uint32_t copied = data_blk->valid_size;
    if (copied > hls_io->media_remain) {
        copied = hls_io->media_remain;
    }
    load->valid_size = copied;
    hls_io->media_remain -= copied;
    load->is_done = (hls_io->media_remain == 0) && data_blk->is_last;
    hls_io->base.db_block = *data_blk;
    return ret;
}

static esp_gmf_err_io_t _hls_acquire_read(esp_gmf_io_handle_t handle, void *payload, uint32_t wanted_size, int block_ticks)
{
    hls_io_t *hls_io = (hls_io_t *)handle;
    if (hls_io == NULL || hls_io->hls_fetcher == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    esp_gmf_payload_t *payload_info = (esp_gmf_payload_t *)payload;
    hls_fetch_stream_data_t stream_data = {};
    int ret = 0;
    stream_data.data = payload_info->buf + sizeof(hls_meta_info_t);
    const uint32_t meta_sz = (uint32_t)sizeof(hls_meta_info_t);
    stream_data.size = HLS_READ_BLOCK_SIZE;
    if (payload_info->buf_length > meta_sz) {
        uint32_t media_room = payload_info->buf_length - meta_sz;
        if (media_room < stream_data.size) {
            stream_data.size = media_room;
        }
    }
    int retry_count = HLS_MAX_RETRY_COUNT;
RETRY:
    ret = hls_fetcher_read_data(hls_io->hls_fetcher, ESP_EXTRACTOR_STREAM_TYPE_AUDIO,
                                &stream_data);

    if (ret != 0) {
        if (ret == ESP_EXTRACTOR_ERR_WAITING_OUTPUT) {
            media_lib_thread_sleep(500);
            retry_count--;
            if (retry_count <= 0) {
                ESP_LOGE(TAG, "Waiting for new URL timeout");
                return ESP_GMF_IO_FAIL;
            }
            goto RETRY;
        }
        ESP_LOGE(TAG, "Fetch return %d", ret);
        if (ret == ESP_EXTRACTOR_ERR_EOS) {
            esp_gmf_db_done_write(hls_io->base.data_bus);
            ret = ESP_GMF_JOB_ERR_DONE;
        } else {
            ret = ESP_GMF_IO_FAIL;
            esp_gmf_db_abort(hls_io->base.data_bus);
        }
    } else {
        hls_meta_info_t *meta_info = (hls_meta_info_t *)payload_info->buf;
        meta_info->bos = stream_data.bos;
        meta_info->format = stream_data.format;
        meta_info->valid_size = stream_data.valid_size;
        payload_info->valid_size = sizeof(hls_meta_info_t) + stream_data.valid_size;
        if (meta_info->bos) {
            ESP_LOGD(TAG, "S %02x %02x cs:%d", stream_data.data[0], stream_data.data[1],
                     checksum(stream_data.data, stream_data.valid_size));
        }
    }
    return ret;
}

static esp_gmf_err_io_t _hls_release_read(esp_gmf_io_handle_t handle, void *payload, int block_ticks)
{
    hls_io_t *hls_io = (hls_io_t *)handle;
    if (hls_io == NULL || hls_io->hls_fetcher == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    return ESP_GMF_IO_OK;
}

static esp_gmf_err_t _hls_seek(esp_gmf_io_handle_t io, uint64_t seek_time)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    if (hls_io == NULL || hls_io->hls_fetcher == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    ESP_LOGI(TAG, "Seek to time %d", (int)seek_time);
    int ret = hls_fetcher_seek(hls_io->hls_fetcher, (uint32_t)seek_time);
    if (ret == ESP_EXTRACTOR_ERR_OK) {
        hls_io->media_remain = 0;
        return ESP_GMF_ERR_OK;
    }
    return ESP_GMF_ERR_FAIL;
}

static esp_gmf_err_t _hls_reset(esp_gmf_io_handle_t io)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    hls_io->media_remain = 0;
    if (hls_io->base.data_bus) {
        esp_gmf_db_reset(hls_io->base.data_bus);
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _hls_close(esp_gmf_io_handle_t io)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    if (hls_io == NULL || hls_io->hls_fetcher == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    hls_io->media_remain = 0;
    hls_fetcher_close(hls_io->hls_fetcher);
    hls_io->hls_fetcher = NULL;
    hls_io->prev_io_tag = NULL;

    if (hls_io->io) {
        esp_gmf_io_close(hls_io->io);
        esp_gmf_obj_delete(hls_io->io);
        hls_io->io = NULL;
    }
    if (hls_io->base.data_bus) {
        esp_gmf_db_deinit(hls_io->base.data_bus);
        hls_io->base.data_bus = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _hls_prev_close(esp_gmf_io_handle_t io)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    if (hls_io == NULL) {
        return ESP_GMF_IO_FAIL;
    }
    if (hls_io->hls_fetcher) {
        hls_fetcher_read_abort(hls_io->hls_fetcher);
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _hls_delete(esp_gmf_io_handle_t io)
{
    hls_io_t *hls_io = (hls_io_t *)io;
    ESP_LOGD(TAG, "Delete, %s-%p", OBJ_GET_TAG(hls_io), hls_io);
    void *cfg = OBJ_GET_CFG(io);
    if (cfg) {
        esp_gmf_oal_free(cfg);
    }
    esp_gmf_io_deinit(io);
    esp_gmf_oal_free(hls_io);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_io_hls_init(esp_hls_io_cfg_t *config, esp_gmf_io_handle_t *io)
{
    ESP_GMF_NULL_CHECK(TAG, config, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, config->pool, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, io, return ESP_GMF_ERR_INVALID_ARG);
    *io = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    hls_io_t *hls_io = esp_gmf_oal_calloc(1, sizeof(hls_io_t));
    ESP_GMF_MEM_VERIFY(TAG, hls_io, return ESP_GMF_ERR_MEMORY_LACK, "HLS stream", sizeof(hls_io_t));
    hls_io->base.dir = ESP_GMF_IO_DIR_READER;
    hls_io->base.type = ESP_GMF_IO_TYPE_BLOCK;

    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)hls_io;
    obj->new_obj = _hls_new;
    obj->del_obj = _hls_delete;
    esp_hls_io_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto _hls_fail;}, "hls_io_cfg", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    esp_gmf_obj_set_config(obj, cfg, sizeof(*config));
    ret = esp_gmf_obj_set_tag(obj, (config->name == NULL ? "io_hls" : config->name));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto _hls_fail, "Failed to set obj tag");

    hls_io->base.get_score = _hls_get_score;
    hls_io->base.close = _hls_close;
    hls_io->base.open = _hls_open;
    hls_io->base.seek = _hls_seek;
    hls_io->base.reset = _hls_reset;
    hls_io->base.prev_close = _hls_prev_close;
    hls_io->base.acquire_read = _hls_acquire_read;
    hls_io->base.release_read = _hls_release_read;

    esp_gmf_io_cfg_t io_cfg = DEFAULT_HLS_IO_CFG();
    io_cfg.buffer_cfg.read_filter = _hls_user_read_filter;
    io_cfg.buffer_cfg.io_size = HLS_READ_BLOCK_SIZE + sizeof(hls_meta_info_t);
    if (config->io_cfg.buffer_cfg.buffer_size > 0) {
        io_cfg.buffer_cfg.buffer_size = config->io_cfg.buffer_cfg.buffer_size;
    }
    if (config->io_cfg.thread.stack > 0) {
        io_cfg.thread.stack = config->io_cfg.thread.stack;
        io_cfg.thread.stack_in_ext = config->io_cfg.thread.stack_in_ext;
        io_cfg.thread.core = config->io_cfg.thread.core;
        io_cfg.thread.prio = config->io_cfg.thread.prio;
    }
    ret = esp_gmf_io_init(obj, &io_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto _hls_fail, "Failed to init io");

    *io = obj;
    return ESP_GMF_ERR_OK;
_hls_fail:
    esp_gmf_obj_delete(obj);
    return ret;
}
