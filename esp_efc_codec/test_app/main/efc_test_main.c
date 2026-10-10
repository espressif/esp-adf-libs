/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_efc_codec.h"

static const char *TAG = "efc_test";

/* Plain and DEFLATE-compressed (`z`) builds of the SAME clip, so we can measure
 * exactly what the optional compression costs at decode time on real hardware. */
extern const uint8_t giphy_efc_start[] asm("_binary_giphy_efc_start");
extern const uint8_t giphy_efc_end[] asm("_binary_giphy_efc_end");
extern const uint8_t giphy_z_efc_start[] asm("_binary_giphy_z_efc_start");
extern const uint8_t giphy_z_efc_end[] asm("_binary_giphy_z_efc_end");

typedef struct {
    int       frames, W, H;
    uint32_t  csum;
    size_t    blob_sz;
    int64_t   open_us;       /* dec_open: header parse + (for z) streaming setup */
    double    decode_us_pf;  /* per-frame decode; for z this INCLUDES gradual inflate */
    size_t    ctx_heap;      /* heap held by the live decoder (cur/prev + z buffers) */
} bench_t;

typedef struct efc_inflate efc_inflate_t;
efc_inflate_t *efc_inflate_begin(const uint8_t *src, size_t src_len);
void efc_inflate_restart(efc_inflate_t *s);
int efc_inflate_take(efc_inflate_t *s, uint8_t *out, size_t n);
void efc_inflate_end(efc_inflate_t *s);
size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len,
                                   const void *pSrc_buf, size_t src_buf_len, int flags);

static uint32_t csum_frame(uint32_t acc, const uint16_t *px, int n)
{
    for (int i = 0; i < n; i++) {
        acc = acc * 2654435761u + px[i];
    }
    return acc;
}

static int bench_efc(const uint8_t *blob, size_t len, int iters, bench_t *r)
{
    uint8_t *buf = malloc(len);  /* copy to RAM for fair timing (vs flash) */
    if (!buf) {
        return -1;
    }
    memcpy(buf, blob, len);

    /* (1) open cost, averaged */
    int64_t open_acc = 0;
    for (int it = 0; it < iters; it++) {
        int64_t t0 = esp_timer_get_time();
        esp_efc_dec_handle_t dd = NULL;
        esp_efc_err_t open_result = esp_efc_dec_open(buf, len, &dd);
        open_acc += esp_timer_get_time() - t0;
        if (open_result != ESP_EFC_OK) {
            free(buf);
            return -1;
        }
        esp_efc_dec_close(dd);
    }

    /* (2) per-frame decode + live-decoder heap footprint */
    size_t heap_before = esp_get_free_heap_size();
    esp_efc_dec_handle_t d = NULL;
    esp_efc_err_t open_result = esp_efc_dec_open(buf, len, &d);
    size_t heap_after = esp_get_free_heap_size();
    if (open_result != ESP_EFC_OK) {
        free(buf);
        return -1;
    }
    esp_efc_dec_info_t info;
    if (esp_efc_dec_get_info(d, &info) != ESP_EFC_OK) {
        esp_efc_dec_close(d);
        free(buf);
        return -1;
    }
    int W = info.width, H = info.height, FC = info.frame_count;
    uint16_t *fbuf = malloc((size_t)W * H * 2);  /* persistent across frames */
    if (!fbuf) {
        esp_efc_dec_close(d);
        free(buf);
        return -1;
    }

    int64_t total = 0;
    int ok = 0;
    uint32_t cs = 0;
    for (int it = 0; it < iters; it++) {
        int64_t t0 = esp_timer_get_time();
        for (int i = 0; i < FC; i++) {
            if (esp_efc_dec_frame(d, i, fbuf) == ESP_EFC_OK && it == 0) {
                ok++;
            }
        }
        total += esp_timer_get_time() - t0;
        cs = 0;
        for (int i = 0; i < FC; i++) {  /* checksum pass (not timed) */
            if (esp_efc_dec_frame(d, i, fbuf) == ESP_EFC_OK) {
                cs = csum_frame(cs, fbuf, W * H);
            }
        }
    }

    free(fbuf);
    esp_efc_dec_close(d);
    free(buf);

    r->frames = ok;
    r->W = W;
    r->H = H;
    r->csum = cs;
    r->blob_sz = len;
    r->open_us = open_acc / (iters > 0 ? iters : 1);
    r->decode_us_pf = ok ? (double)total / (iters > 0 ? iters : 1) / ok : 0.0;
    r->ctx_heap = (heap_before > heap_after) ? (heap_before - heap_after) : 0;
    return 0;
}

static uint32_t z_raw_token_len(const uint8_t *blob)
{
    uint16_t FC = (uint16_t)(blob[16] | (blob[17] << 8));
    uint16_t palC = (uint16_t)(blob[18] | (blob[19] << 8));
    size_t off = 20u + (size_t)palC * 2u + (size_t)FC * 9u;
    return (uint32_t)blob[off] | ((uint32_t)blob[off + 1] << 8) |
           ((uint32_t)blob[off + 2] << 16) | ((uint32_t)blob[off + 3] << 24);
}

static void inflate_probe(const uint8_t *blob, size_t blob_sz)
{
    uint16_t FC = (uint16_t)(blob[16] | (blob[17] << 8));
    uint16_t palC = (uint16_t)(blob[18] | (blob[19] << 8));
    size_t diro = 20u + (size_t)palC * 2u;
    size_t fdata = diro + (size_t)FC * 9u;
    uint32_t raw_len = z_raw_token_len(blob);
    const uint8_t *comp = blob + fdata + 4;
    size_t comp_len = blob_sz - (fdata + 4);

    uint32_t maxlen = 0;
    for (int i = 0; i < FC; i++) {
        const uint8_t *de = blob + diro + (size_t)i * 9;
        uint32_t l = (uint32_t)de[5] | ((uint32_t)de[6] << 8) | ((uint32_t)de[7] << 16) | ((uint32_t)de[8] << 24);
        if (l > maxlen) {
            maxlen = l;
        }
    }

    uint8_t *whole = malloc(raw_len);
    uint8_t *scratch = malloc(maxlen ? maxlen : 1);
    if (!whole || !scratch) {
        free(whole);
        free(scratch);
        return;
    }

    /* (A) one-shot bulk inflate of the entire stream */
    int64_t t0 = esp_timer_get_time();
    size_t got = tinfl_decompress_mem_to_mem(whole, raw_len, comp, comp_len, 0);
    int64_t t_oneshot = esp_timer_get_time() - t0;

    /* (B) gradual streaming inflate, frame by frame (no decode) */
    efc_inflate_t *s = efc_inflate_begin(comp, comp_len);
    int64_t t_stream = -1;
    if (s) {
        t0 = esp_timer_get_time();
        int okk = 1;
        for (int i = 0; i < FC && okk; i++) {
            const uint8_t *de = blob + diro + (size_t)i * 9;
            uint32_t l = (uint32_t)de[5] | ((uint32_t)de[6] << 8) | ((uint32_t)de[7] << 16) | ((uint32_t)de[8] << 24);
            okk = (efc_inflate_take(s, scratch, l) == 0);
        }
        t_stream = okk ? (esp_timer_get_time() - t0) : -1;
        efc_inflate_end(s);
    }

    ESP_LOGI(TAG, "inflate probe (%u B -> %u B, %d frames):", (unsigned)comp_len, (unsigned)got, (int)FC);
    ESP_LOGI(TAG, "  one-shot bulk   : %lld us   (%.1f us/frame amortized)",
             (long long)t_oneshot, (double)t_oneshot / FC);
    ESP_LOGI(TAG, "  gradual stream  : %lld us   (%.1f us/frame)   overhead %+.1f%%",
             (long long)t_stream, (double)t_stream / FC,
             t_oneshot ? ((double)t_stream / (double)t_oneshot - 1.0) * 100.0 : 0.0);

    free(whole);
    free(scratch);
}

void app_main(void)
{
    const int iters = 4;
    size_t plain_sz = (size_t)(giphy_efc_end - giphy_efc_start);
    size_t z_sz = (size_t)(giphy_z_efc_end - giphy_z_efc_start);

    ESP_LOGI(TAG, "===== EFC codec perf test (plain vs DEFLATE `z`) =====");
#if defined(CONFIG_ESP_EFC_USE_SIMD)
    ESP_LOGI(TAG, "config: 4x4_only=%d  SIMD=on (target PIE)", (int)CONFIG_ESP_EFC_DECODE_4X4_ONLY);
#else
    ESP_LOGI(TAG, "config: 4x4_only=%d  SIMD=off (portable C)", (int)CONFIG_ESP_EFC_DECODE_4X4_ONLY);
#endif  /* defined(CONFIG_ESP_EFC_USE_SIMD) */
    ESP_LOGI(TAG, "assets: plain=%u B  z=%u B (%.2fx smaller)  free heap %u B",
             (unsigned)plain_sz, (unsigned)z_sz,
             z_sz ? (double)plain_sz / z_sz : 0.0, (unsigned)esp_get_free_heap_size());

    inflate_probe(giphy_z_efc_start, z_sz);

    for (int round = 0;; round++) {
        bench_t p = {0}, z = {0};
        int rc = bench_efc(giphy_efc_start, plain_sz, iters, &p);
        rc |= bench_efc(giphy_z_efc_start, z_sz, iters, &z);
        if (rc != 0 || p.frames == 0 || z.frames == 0) {
            ESP_LOGE(TAG, "decode failed (plain.fr=%d z.fr=%d)", p.frames, z.frames);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        uint32_t raw_len = z_raw_token_len(giphy_z_efc_start);
        size_t z_extra = (z.ctx_heap > p.ctx_heap) ? (z.ctx_heap - p.ctx_heap) : 0;

        ESP_LOGI(TAG, "--- round %d: %d frames %dx%d ---", round, p.frames, p.W, p.H);
        ESP_LOGI(TAG, "plain: open %lld us   decode %.1f us/frame   ctx heap %u B",
                 (long long)p.open_us, p.decode_us_pf, (unsigned)p.ctx_heap);
        ESP_LOGI(TAG, "  z  : open %lld us   decode %.1f us/frame   ctx heap %u B   (gradual inflate per frame)",
                 (long long)z.open_us, z.decode_us_pf, (unsigned)z.ctx_heap);
        ESP_LOGI(TAG, "drop : decode %+.1f%% per frame (every playthrough)   (size -%.1f%%)",
                 (z.decode_us_pf / p.decode_us_pf - 1.0) * 100.0,
                 (1.0 - (double)z_sz / plain_sz) * 100.0);
        ESP_LOGI(TAG, "mem  : z streaming work-set %u B vs inflate-all %u B  -> saves %d B",
                 (unsigned)z_extra, (unsigned)raw_len, (int)((long)raw_len - (long)z_extra));
        ESP_LOGI(TAG, "verify: plain csum=0x%08x  z csum=0x%08x -> %s",
                 p.csum, z.csum, (p.csum == z.csum) ? "MATCH (identical output)" : "DIFF !!");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
