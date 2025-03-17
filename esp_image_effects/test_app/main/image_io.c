// Copyright 2024 Espressif Systems (Shanghai) CO., LTD.
// All rights reserved.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "image_io.h"
#include "sdkconfig.h"
#if CONFIG_IDF_TARGET_ESP32P4
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */

#define MOUNT_POINT "/sdcard"

static sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
static sdmmc_host_t         host            = SDSPI_HOST_DEFAULT();
static sdmmc_card_t        *card;
static const char           mount_point[] = MOUNT_POINT;
#if CONFIG_IDF_TARGET_ESP32P4
#define PIN_CLK 43
#define PIN_CMD 44
#define PIN_D0  39
#define PIN_D1  40
#define PIN_D2  41
#define PIN_D3  42
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
#if CONFIG_IDF_TARGET_ESP32S3
#define PIN_CLK 15
#define PIN_CMD 7
#define PIN_D0  4
#endif  /* CONFIG_IDF_TARGET_ESP32S3 */
void mount_sd(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 20,
        .allocation_unit_size = 16 * 1024};
    printf("Initializing SD card\n");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#if CONFIG_IDF_TARGET_ESP32P4
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = 4,
    };

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("SDCARD", "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#if CONFIG_IDF_TARGET_ESP32P4
    slot_config.clk = PIN_CLK;
    slot_config.cmd = PIN_CMD;
    slot_config.d0 = PIN_D0;
    slot_config.d1 = PIN_D1;
    slot_config.d2 = PIN_D2;
    slot_config.d3 = PIN_D3;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_config.width = 4;
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */

#if CONFIG_IDF_TARGET_ESP32S3
    slot_config.clk = PIN_CLK;
    slot_config.cmd = PIN_CMD;
    slot_config.d0 = PIN_D0;

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_config.width = 1;
#endif  /* CONFIG_IDF_TARGET_ESP32S3 */
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("SDCARD", "Failed to mount filesystem. "
                               "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE("SDCARD", "Failed to initialize the card (%s). "
                               "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI("SDCARD", "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}

void unmount_sd(void)
{
    esp_err_t ret;
    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    printf("Card unmounted\n");
#if CONFIG_IDF_TARGET_ESP32P4
    ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("SDCARD", "Failed to delete the on-chip LDO power control driver");
        return;
    }
#endif  /* CONFIG_IDF_TARGET_ESP32P4 */
}
