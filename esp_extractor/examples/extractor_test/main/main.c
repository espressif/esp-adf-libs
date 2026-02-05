/* esp_extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#ifndef __linux__
#include "sdkconfig.h"
#include "settings.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_idf_version.h"
#if CONFIG_IDF_TARGET_ESP32P4
#include "esp_ldo_regulator.h"
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
#endif  /* __linux__ */
#include "esp_extractor_defaults.h"
#include "extractor_helper.h"
#include "extractor_cust.h"
#include "raw_extractor_test.h"
#include "esp_log.h"

#define TAG                 "EXTRACTOR_DEMO"
#define MAX_PATH_LEN        (128)
#define OUTPUT_POOL_SIZE    (100 * 1024)  // Should more than one frame size
#define SUPPORTED_FILE_EXT  "avi;aac;mp3;mp4;flv;ts;caf;amr;ogg;flac;wav;opus;"

#ifndef __linux__
#define TEST_FOLDER  "/sdcard"
#else
#define TEST_FOLDER  (argc > 1 ? argv[1] : NULL)
#endif  /* __linux__ */

void print_stream_info(esp_extractor_handle_t extractor)
{
    uint16_t audio_num = 0;
    uint16_t video_num = 0;
    esp_extractor_stream_info_t stream_info;
    esp_extractor_err_t ret = esp_extractor_get_stream_num(extractor, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, &audio_num);
    for (uint16_t i = 0; i < audio_num; i++) {
        ret = esp_extractor_get_stream_info(extractor, ESP_EXTRACTOR_STREAM_TYPE_AUDIO, i, &stream_info);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to get audio stream info ret %d", ret);
            continue;
        }
        esp_extractor_audio_stream_info_t *audio_info = &stream_info.audio_info;
        ESP_LOGI(TAG, "Audio format:%s sample_rate:%d channel:%d bits:%d duration:%d",
                 esp_extractor_get_format_name(audio_info->format),
                 (int)audio_info->sample_rate, audio_info->channel, audio_info->bits_per_sample,
                 (int)stream_info.duration);
    }
    ret = esp_extractor_get_stream_num(extractor, ESP_EXTRACTOR_STREAM_TYPE_VIDEO, &video_num);
    for (uint16_t i = 0; i < video_num; i++) {
        ret = esp_extractor_get_stream_info(extractor, ESP_EXTRACTOR_STREAM_TYPE_VIDEO, i, &stream_info);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to get video stream info ret %d", ret);
            continue;
        }
        esp_extractor_video_stream_info_t *video_info = &stream_info.video_info;
        ESP_LOGI(TAG, "Video format:%s resolution:%dx%d fps:%d dur:%d",
                 esp_extractor_get_format_name(video_info->format),
                 video_info->width, video_info->height, video_info->fps,
                 (int)stream_info.duration);
    }
}

int extractor_use_helper(const char *url, frame_verify_func verify)
{
    esp_extractor_err_t ret = ESP_EXTRACTOR_ERR_OK;
    esp_extractor_config_t *cfg = NULL;
    esp_extractor_handle_t extractor = NULL;
    do {
        cfg = esp_extractor_alloc_file_config(url, ESP_EXTRACT_MASK_AV, OUTPUT_POOL_SIZE);
        if (cfg == NULL) {
            ESP_LOGE(TAG, "Failed to alloc file config");
            break;
        }
        ret = esp_extractor_open(cfg, &extractor);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to open extractor ret %d", ret);
            break;
        }
        ret = esp_extractor_parse_stream(extractor);
        if (ret != ESP_EXTRACTOR_ERR_OK) {
            ESP_LOGE(TAG, "Failed to parse stream ret %d", ret);
            break;
        }
        print_stream_info(extractor);
        esp_extractor_frame_info_t frame = {};
        // Now read frames and consume it instantly
        int audio_frame_count = 0;
        int audio_frame_size = 0;
        int audio_last_pts = 0;
        int video_frame_count = 0;
        int video_frame_size = 0;
        int video_last_pts = 0;

        while (1) {
            frame.frame_buffer = NULL;
            ret = esp_extractor_read_frame(extractor, &frame);
            if (ret != ESP_EXTRACTOR_ERR_OK) {
                if (ret < 0) {
                    ESP_LOGE(TAG, "Failed to read frames ret %d", ret);
                }
                if (ret != ESP_EXTRACTOR_ERR_SKIPPED) {
                    break;
                }
            }
            if (frame.frame_buffer) {
                if (frame.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
                    audio_frame_count++;
                    audio_frame_size += frame.frame_size;
                    audio_last_pts = frame.pts;
                } else if (frame.stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
                    video_frame_count++;
                    video_frame_size += frame.frame_size;
                    video_last_pts = frame.pts;
                }
                bool verify_ok = true;
                if (verify) {
                    verify_ok = verify(&frame);
                }
                esp_extractor_release_frame(extractor, &frame);
                if (verify_ok == false) {
                    int frame_idx = (frame.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) ? audio_frame_count : video_frame_count;
                    ESP_LOGE(TAG, "Fail to verify stream %d frame_idx:%d", frame.stream_type, frame_idx - 1);
                }
            }
        }
        if (audio_frame_count) {
            ESP_LOGI(TAG, "Audio: frame_count:%d size:%d last_pts:%d", audio_frame_count, audio_frame_size, audio_last_pts);
        }
        if (video_frame_count) {
            ESP_LOGI(TAG, "Video: frame_count:%d size:%d last_pts:%d", video_frame_count, video_frame_size, video_last_pts);
        }
    } while (0);
    if (extractor) {
        esp_extractor_close(extractor);
    }
    esp_extractor_free_config(cfg);
    return ret;
}

static bool is_matched(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return false;
    }
    char ext[16];
    int len = strlen(++dot);
    if (len + 1 > sizeof(ext)) {
        return false;
    }
    int i = 0;
    while (*dot) {
        ext[i++] = tolower(*dot);
        dot++;
    }
    ext[len] = 0;
    char *find = strstr(SUPPORTED_FILE_EXT, ext);
    return find && find[len] == ';';
}

static bool concat_path(char *dst, int dst_len, const char *src_a, const char *src_b)
{
    int len_a = strlen(src_a);
    int len_b = strlen(src_b);
    if (len_a + len_b + 2 > dst_len) {
        return false;
    }
    memcpy(dst, src_a, len_a);
    dst += len_a;
    *dst = '/';
    dst++;
    memcpy(dst, src_b, len_b);
    dst[len_b] = 0;
    return true;
}

static void extractor_all_files(char *folder)
{
    if (folder == NULL) {
        return;
    }
    DIR *dir = opendir(folder);
    if (dir == NULL) {
        FILE *fp = fopen(folder, "rb");
        if (fp) {
            fclose(fp);
            ESP_LOGI(TAG, "Start extractor for %s", folder);
            extractor_use_helper(folder, NULL);
            return;
        }
        ESP_LOGE(TAG, "%s not existed", folder);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip directories
        if (entry->d_type == DT_DIR) {
            continue;
        }
        char path[MAX_PATH_LEN];
        if (is_matched(entry->d_name) && concat_path(path, sizeof(path), folder, entry->d_name)) {
            ESP_LOGI(TAG, "Start extractor for %s", path);
            extractor_use_helper(path, NULL);
            printf("\n");
        }
    }
    closedir(dir);
}

#ifndef __linux__
static void enable_mmc_phy_power(void)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#if CONFIG_IDF_TARGET_ESP32P4
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    esp_ldo_channel_handle_t ldo_phy_chan;
    esp_ldo_acquire_channel(&ldo_cfg, &ldo_phy_chan);
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
#endif  /* ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0) */
}

static int mount_sdcard(void)
{
    enable_mmc_phy_power();
#if defined CONFIG_IDF_TARGET_ESP32
    gpio_config_t sdcard_pwr_pin_cfg = {
        .pin_bit_mask = 1UL << GPIO_NUM_13,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sdcard_pwr_pin_cfg);
    gpio_set_level(GPIO_NUM_13, 0);
#endif  /* defined CONFIG_IDF_TARGET_ESP32 */

#if SOC_SDMMC_HOST_SUPPORTED
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
#if CONFIG_IDF_TARGET_ESP32P4
    host.slot = 0;
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#endif  /* CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4 */
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = (SDCARD_D3 != -1) ? 4 : 1;
#if SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.width = (SDCARD_D3 != -1) ? 4 : 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_config.d4 = -1;
    slot_config.d5 = -1;
    slot_config.d6 = -1;
    slot_config.d7 = -1;
    slot_config.cd = -1;
    slot_config.wp = -1;
    slot_config.clk = SDCARD_CLK;
    slot_config.cmd = SDCARD_CMD;
    slot_config.d0 = SDCARD_D0;
    slot_config.d1 = SDCARD_D1;
    slot_config.d2 = SDCARD_D2;
    slot_config.d3 = SDCARD_D3;
#endif  /* SOC_SDMMC_USE_GPIO_MATRIX */
#if CONFIG_IDF_TARGET_ESP32P4
    memset(&slot_config, 0, sizeof(sdmmc_slot_config_t));
    slot_config.width = 4;
    slot_config.cd = SDMMC_SLOT_NO_CD;
    slot_config.wp = SDMMC_SLOT_NO_WP;
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
    sdmmc_card_t *card = NULL;
    return esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
#else
    return -1;
#endif  /* SOC_SDMMC_HOST_SUPPORTED */
}

void app_main()
#else
int main(int argc, char *argv[])
#endif  /* __linux__ */
{
#ifndef __linux__
    // Mount SDcard firstly
    mount_sdcard();
#endif  /* __linux__ */
    // Register default extractors
    esp_extractor_register_default();

    // Register customized extractor
    my_extractor_register();

    if (my_extractor_gen_test(MY_EXTRACTOR_TEST_URL) == 0) {
        extractor_use_helper(MY_EXTRACTOR_TEST_URL, my_extractor_frame_verify);
    }

    // Test raw extractor
    if (raw_extractor_test() == 0) {
        ESP_LOGI(TAG, "Raw extractor test passed");
    }

    // Extractor all files under test folder
    extractor_all_files(TEST_FOLDER);

    // Unregister default extractors
    esp_extractor_unregister_default();
#ifdef __linux__
    return 0;
#endif  /* __linux__ */
}
