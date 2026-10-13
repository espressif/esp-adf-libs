/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * On-device decode benchmark for the EAF vs EFC test set produced by
 * pc/make_testset.py. It reads every file from TEST_DIR (the SD card is mounted
 * by the user, not here), detects EAF vs EFC by magic, decodes the whole clip
 * ITERS times, and prints both a human log and machine-readable RESULT,... CSV
 * lines for pc/analyze_bench.py.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_device.h"
#include "esp_board_manager_defs.h"
#include "esp_efc_codec.h"
#include "../managed_components/espressif__esp_lv_eaf_player/src/esp_eaf_dec.h"

/* The user mounts the SD card and may edit this to match their mount point. */
#define TEST_DIR       "/sdcard/totest"
#define ITERS          1
#define MAX_FILE_SIZE  (5 * 1024 * 1024)

static const char *TAG = "bench";

typedef struct {
    int       ok;
    char      codec[8];  /* "eaf" / "efc" */
    int       W, H, frames;
    double    play_us;  /* average time to decode all frames once */
    double    us_per_frame;
    uint32_t  csum;
} result_t;

static uint32_t csum_frame(uint32_t acc, const uint16_t *px, int n)
{
    for (int i = 0; i < n; i++) {
        acc = acc * 2654435761u + px[i];
    }
    return acc;
}

static void *big_alloc(size_t n)
{
    void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : malloc(n);
}

static void bench_efc(const uint8_t *blob, size_t len, result_t *r)
{
    memset(r, 0, sizeof(*r));
    strcpy(r->codec, "efc");

    esp_efc_dec_handle_t d = NULL;
    if (esp_efc_dec_open(blob, len, &d) != ESP_EFC_OK) {
        ESP_LOGW(TAG, "  efc open failed");
        return;
    }
    esp_efc_dec_info_t info;
    if (esp_efc_dec_get_info(d, &info) != ESP_EFC_OK) {
        esp_efc_dec_close(d);
        return;
    }
    int W = info.width, H = info.height, FC = info.frame_count;
    uint16_t *fb = big_alloc((size_t)W * H * 2);
    if (!fb) {
        esp_efc_dec_close(d);
        ESP_LOGW(TAG, "  efc framebuf alloc failed");
        return;
    }

    int64_t total = 0;
    uint32_t cs = 0;
    for (int it = 0; it < ITERS; it++) {
        int64_t t0 = esp_timer_get_time();
        for (int i = 0; i < FC; i++) {
            if (esp_efc_dec_frame(d, i, fb) == ESP_EFC_OK) {
                if (it == 0) {
                    cs = csum_frame(cs, fb, W * H);
                }
            }
        }
        total += esp_timer_get_time() - t0;
    }
    r->ok = 1;
    r->W = W;
    r->H = H;
    r->frames = FC;
    r->play_us = (double)total / ITERS;
    r->us_per_frame = FC ? r->play_us / FC : 0.0;
    r->csum = cs;
    free(fb);
    esp_efc_dec_close(d);
}

/* Decode one EAF frame into `fb` exactly like the reference player: parse the
 * per-frame header (palette + block table), then decode every block in place. */
static int eaf_decode_frame(esp_eaf_format_handle_t h, int idx, uint16_t *fb)
{
    const uint8_t *fd = esp_eaf_format_get_frame_data(h, idx);
    int fsz = esp_eaf_format_get_frame_size(h, idx);
    if (!fd || fsz <= 0) {
        return -1;
    }
    esp_eaf_header_t hdr;
    if (esp_eaf_header_parse(fd, fsz, &hdr) != ESP_EAF_FORMAT_VALID) {
        return -1;
    }
    size_t block_stride = (size_t)hdr.block_height * hdr.width;  /* in uint16 */
    esp_eaf_palette_cache_t cache;
    memset(cache.color, 0xFF, sizeof(cache.color));
    for (int b = 0; b < hdr.blocks; b++) {
        uint8_t *bb = (uint8_t *)(fb + (size_t)b * block_stride);
        esp_eaf_block_decode(&hdr, fd, b, bb, NULL, false, &cache);
    }
    esp_eaf_free_header(&hdr);
    return 0;
}

static void bench_eaf(const uint8_t *blob, size_t len, result_t *r)
{
    memset(r, 0, sizeof(*r));
    strcpy(r->codec, "eaf");

    esp_eaf_format_handle_t h = NULL;
    if (esp_eaf_format_init(blob, len, &h) != ESP_OK || !h) {
        ESP_LOGW(TAG, "  eaf init failed");
        return;
    }
    int FC = esp_eaf_format_get_total_frames(h);

    /* dimensions from frame 0 */
    esp_eaf_header_t hdr0;
    const uint8_t *fd0 = esp_eaf_format_get_frame_data(h, 0);
    if (!fd0 || esp_eaf_header_parse(fd0, esp_eaf_format_get_frame_size(h, 0), &hdr0) != ESP_EAF_FORMAT_VALID) {
        esp_eaf_format_deinit(h);
        ESP_LOGW(TAG, "  eaf header parse failed");
        return;
    }
    int W = hdr0.width, H = hdr0.height;
    esp_eaf_free_header(&hdr0);

    uint16_t *fb = big_alloc((size_t)W * H * 2);
    if (!fb) {
        esp_eaf_format_deinit(h);
        ESP_LOGW(TAG, "  eaf framebuf alloc failed");
        return;
    }

    int64_t total = 0;
    uint32_t cs = 0;
    for (int it = 0; it < ITERS; it++) {
        int64_t t0 = esp_timer_get_time();
        for (int i = 0; i < FC; i++) {
            if (eaf_decode_frame(h, i, fb) == 0) {
                if (it == 0) {
                    cs = csum_frame(cs, fb, W * H);
                }
            }
        }
        total += esp_timer_get_time() - t0;
    }

    r->ok = 1;
    r->W = W;
    r->H = H;
    r->frames = FC;
    r->play_us = (double)total / ITERS;
    r->us_per_frame = FC ? r->play_us / FC : 0.0;
    r->csum = cs;

    free(fb);
    esp_eaf_format_deinit(h);
}

static uint8_t *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > MAX_FILE_SIZE) {
        fclose(f);
        ESP_LOGW(TAG, "  %s: bad size %ld", path, sz);
        return NULL;
    }
    uint8_t *buf = big_alloc((size_t)sz);
    if (!buf) {
        fclose(f);
        ESP_LOGW(TAG, "  %s: alloc %ld failed", path, sz);
        return NULL;
    }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) {
        free(buf);
        return NULL;
    }
    *out_len = (size_t)sz;
    return buf;
}

static void run_one(const char *name)
{
    char path[300];
    snprintf(path, sizeof(path), "%s/%s", TEST_DIR, name);
    size_t len = 0;
    uint8_t *blob = read_file(path, &len);
    if (!blob) {
        return;
    }

    result_t r;
    if (len >= 4 && blob[0] == 0x89 && blob[1] == 'E' && blob[2] == 'A' && blob[3] == 'F') {
        bench_eaf(blob, len, &r);
    } else if (len >= 4 && memcmp(blob, "EFC1", 4) == 0) {
        bench_efc(blob, len, &r);
    } else {
        ESP_LOGW(TAG, "  %s: unknown magic, skipping", name);
        free(blob);
        return;
    }

    if (r.ok) {
        double mpix_s = r.us_per_frame > 0 ? (double)r.W * r.H / r.us_per_frame : 0.0;
        ESP_LOGI(TAG, "  %-24s %-3s %7.1f KB  %dx%d x%d  %8.2f ms/play  %7.1f us/frame  %.2f Mpix/s",
                 name, r.codec, len / 1024.0, r.W, r.H, r.frames,
                 r.play_us / 1000.0, r.us_per_frame, mpix_s);
        /* machine-readable line for pc/analyze_bench.py */
        printf("RESULT,%s,%s,%u,%d,%d,%d,%.3f,%.3f,0x%08x\n",
               name, r.codec, (unsigned)len, r.W, r.H, r.frames,
               r.play_us / 1000.0, r.us_per_frame, (unsigned)r.csum);
    } else {
        ESP_LOGW(TAG, "  %-24s decode FAILED", name);
    }
    free(blob);
}

void app_main(void)
{
    int ret = esp_board_device_init(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_board_device_init failed");
        return;
    }
    ESP_LOGI(TAG, "===== EAF vs EFC decode benchmark =====");
    ESP_LOGI(TAG, "dir=%s  iters=%d  free heap=%u B  free PSRAM=%u B",
             TEST_DIR, ITERS, (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    do {
        DIR *dir = opendir(TEST_DIR);
        if (!dir) {
            ESP_LOGE(TAG, "cannot open %s (is the SD card mounted?) - retrying in 3s", TEST_DIR);
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        /* collect + sort names so variants of a clip print together */
        char(*names)[256] = malloc(sizeof(*names) * 256);
        int n = 0;
        struct dirent *de;
        while (n < 256 && (de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') {
                continue;
            }
            snprintf(names[n], sizeof(names[n]), "%s", de->d_name);
            n++;
        }
        closedir(dir);
        for (int i = 1; i < n; i++) {  /* insertion sort */
            char key[256];
            strcpy(key, names[i]);
            int j = i - 1;
            while (j >= 0 && strcmp(names[j], key) > 0) {
                strcpy(names[j + 1], names[j]);
                j--;
            }
            strcpy(names[j + 1], key);
        }

        ESP_LOGI(TAG, "found %d files", n);
        printf("===BENCH BEGIN===\n");
        for (int i = 0; i < n; i++) {
            run_one(names[i]);
        }
        printf("===BENCH END===\n");
        free(names);

        ESP_LOGI(TAG, "done; re-running in 10s");
        vTaskDelay(pdMS_TO_TICKS(10000));
    } while (0);
}
