/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Folder player for EFC clips.
 *
 * It mounts the SD card, lists files whose extensionstarts with ".efc" (so .efc and .efc_z both show),
 * and plays the one you tap. Play restarts from frame 0, Pause holds the current frame, Resume continues.
 * The label is the FPS the player actually reaches.
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"

#include "esp_board_manager_includes.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"
#include "lv_efc.h"

static const char *TAG = "EFC_PLAYER";

#define EFC_DIR             "/sdcard/totest"
#define MAX_CLIPS           256
#define MAX_FILENAME_LEN    64
#define MAX_EFC_FILE_SIZE   (5 * 1024 * 1024)
#define EFC_FRAME_DELAY_MS  10
#define STATUS_ROW_HEIGHT   20
#define CONTROL_ROW_HEIGHT  40

static lv_obj_t *s_player;
static lv_obj_t *s_status;
static lv_obj_t *s_list;
static char s_filename[MAX_CLIPS][MAX_FILENAME_LEN];
static int s_nclips;
static int s_lcd_w = 320;
static int s_lcd_h = 240;
static char s_name[64];

static bool ext_is_efc(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot) {
        return false;
    }
    return strncasecmp(dot, ".efc", 4) == 0;
}

static int cmp_filename(const void *a, const void *b)
{
    return strcmp(a, b);
}

static int scan_clips(void)
{
    DIR *dir = opendir(EFC_DIR);
    if (!dir) {
        ESP_LOGE(TAG, "cannot open %s", EFC_DIR);
        return -1;
    }
    s_nclips = 0;
    struct dirent *de;
    while (s_nclips < MAX_CLIPS && (de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.' || de->d_type == DT_DIR || !ext_is_efc(de->d_name)) {
            continue;
        }
        int nw = snprintf(s_filename[s_nclips], sizeof(s_filename[s_nclips]), "%s", de->d_name);
        if (nw < 0 || nw >= (int)sizeof(s_filename[s_nclips])) {
            continue;
        }
        s_nclips++;
    }
    closedir(dir);
    qsort(s_filename, (size_t)s_nclips, sizeof(s_filename[0]), cmp_filename);
    ESP_LOGI(TAG, "%d .efc* clips in %s", s_nclips, EFC_DIR);
    return s_nclips;
}

static void set_status(const char *text)
{
    lv_label_set_text(s_status, text);
}

static void play_file(const char *filename)
{
    char path[sizeof(EFC_DIR) + MAX_FILENAME_LEN];
    int nw = snprintf(path, sizeof(path), "%s/%s", EFC_DIR, filename);
    if (nw < 0 || nw >= (int)sizeof(path)) {
        set_status("file name too long");
        return;
    }
    snprintf(s_name, sizeof(s_name), "%s", filename);

    struct stat st;
    if (stat(path, &st) != 0 || st.st_size <= 0 || st.st_size > MAX_EFC_FILE_SIZE) {
        set_status("file too large or missing");
        return;
    }
    lv_efc_set_src(s_player, path);
    if (!lv_efc_is_loaded(s_player)) {
        set_status("open failed");
        return;
    }
    char line[96];
    snprintf(line, sizeof(line), "%s  %lu fps", s_name, (unsigned long)lv_efc_get_fps(s_player));
    set_status(line);
}

static void on_clip(lv_event_t *e)
{
    play_file(lv_event_get_user_data(e));
}

static void on_play(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_efc_play(s_player);
}

static void on_pause(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_efc_pause(s_player);
}

static void on_resume(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_efc_resume(s_player);
}

static void status_timer(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (!lv_efc_is_loaded(s_player) || s_name[0] == '\0') {
        return;
    }
    char line[96];
    snprintf(line, sizeof(line), "%s  %lu fps", s_name, (unsigned long)lv_efc_get_fps(s_player));
    set_status(line);
}

static void initial_play_timer(lv_timer_t *t)
{
    lv_timer_delete(t);
    if (s_nclips > 0) {
        play_file(s_filename[0]);
    }
}

static lv_obj_t *add_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, LV_PCT(33));
    lv_obj_set_height(btn, 32);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, text);
    lv_obj_center(lab);
    return btn;
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_pad_all(scr, 0, 0);

    int list_w = s_lcd_w / 3;
    if (list_w < 100) {
        list_w = 100;
    }
    int stage_w = s_lcd_w - list_w;

    s_list = lv_obj_create(scr);
    lv_obj_set_size(s_list, list_w, s_lcd_h);
    lv_obj_set_pos(s_list, 0, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_list, 4, 0);
    lv_obj_set_style_pad_all(s_list, 4, 0);

    s_player = lv_efc_create(scr);
    lv_obj_set_pos(s_player, list_w, 0);
    lv_obj_set_size(s_player, stage_w, s_lcd_h - STATUS_ROW_HEIGHT - CONTROL_ROW_HEIGHT);
    /* This demo requests high-FPS playback. Its LVGL refresh period is also
     * configured to 10 ms so short animations do not skip the same frames. */
    lv_efc_set_frame_delay(s_player, EFC_FRAME_DELAY_MS);
    lv_image_set_inner_align(s_player, LV_IMAGE_ALIGN_CONTAIN_DOWNSCALE);
    lv_obj_set_style_bg_color(s_player, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_player, LV_OPA_COVER, 0);

    s_status = lv_label_create(scr);
    lv_obj_set_pos(s_status, list_w + 4, s_lcd_h - STATUS_ROW_HEIGHT - CONTROL_ROW_HEIGHT + 2);
    lv_obj_set_width(s_status, stage_w - 8);
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_MODE_DOTS);
    lv_label_set_text(s_status, "no clip");

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_pos(row, list_w, s_lcd_h - CONTROL_ROW_HEIGHT);
    lv_obj_set_size(row, stage_w, CONTROL_ROW_HEIGHT);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    add_button(row, "Play", on_play);
    add_button(row, "Pause", on_pause);
    add_button(row, "Resume", on_resume);

    for (int i = 0; i < s_nclips; i++) {
        lv_obj_t *btn = lv_button_create(s_list);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_add_event_cb(btn, on_clip, LV_EVENT_CLICKED, s_filename[i]);
        lv_obj_t *name = lv_label_create(btn);
        lv_label_set_text(name, s_filename[i]);
        lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(name, list_w - 24);
    }
    if (s_nclips == 0) {
        lv_obj_t *empty = lv_label_create(s_list);
        lv_label_set_text(empty, "no .efc* files");
        set_status("folder empty");
    }

    lv_timer_create(status_timer, 500, NULL);
    if (s_nclips > 0) {
        /* Defer playback until LVGL has finished laying out the newly-created
         * object tree and the player has its final dimensions. */
        lv_timer_create(initial_play_timer, 20, NULL);
    }
}

static void player_start(lv_timer_t *t)
{
    lv_timer_delete(t);
    scan_clips();
    build_ui();
}

static esp_err_t lcd_lvgl_adapter_init(void)
{
    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.task_priority = 4;
    /* Frame decode runs on this task. */
    adapter_config.task_stack_size = 20 * 1024;
    adapter_config.task_core_id = 1;
    adapter_config.tick_period_ms = 1;
    adapter_config.task_max_delay_ms = 15;

    esp_err_t ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL adapter: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}

#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
static esp_lv_adapter_rotation_t lcd_get_rotation(const dev_display_lcd_config_t *lcd_cfg)
{
    if (lcd_cfg->swap_xy) {
        return lcd_cfg->mirror_x ? ESP_LV_ADAPTER_ROTATE_90 : ESP_LV_ADAPTER_ROTATE_270;
    }
    return (lcd_cfg->mirror_x || lcd_cfg->mirror_y) ? ESP_LV_ADAPTER_ROTATE_180 : ESP_LV_ADAPTER_ROTATE_0;
}

static esp_lv_adapter_tear_avoid_mode_t lcd_get_tear_mode(uint8_t num_fbs)
{
    if (num_fbs >= 3) {
        return ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    }
    if (num_fbs == 2) {
        return ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_PARTIAL;
    }
    return ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;
}
#endif

static lv_display_t *lcd_lvgl_adapter_register_display(const dev_display_lcd_config_t *lcd_cfg,
                                                       const dev_display_lcd_handles_t *lcd_handles)
{
    esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_0;
    esp_lv_adapter_display_config_t disp_cfg;

    if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_DSI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT
        rotation = lcd_get_rotation(lcd_cfg);
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                              lcd_handles->io_handle,
                                                              lcd_cfg->lcd_width,
                                                              lcd_cfg->lcd_height,
                                                              rotation);
        disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.dsi.dpi_config.num_fbs);
#else
        ESP_LOGE(TAG, "DSI support not enabled");
        return NULL;
#endif
    } else if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB_3WIRE_SPI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT || CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
        rotation = lcd_get_rotation(lcd_cfg);
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                             lcd_handles->io_handle,
                                                             lcd_cfg->lcd_width,
                                                             lcd_cfg->lcd_height,
                                                             rotation);
        if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB_3WIRE_SPI) == 0) {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_3WIRE_SPI_SUPPORT
            disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.rgb_3wire_spi.rgb_panel_config.num_fbs);
#endif
        } else {
#if CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_RGB_SUPPORT
            disp_cfg.tear_avoid_mode = lcd_get_tear_mode(lcd_cfg->sub_cfg.rgb.panel_config.num_fbs);
#endif
        }
#else
        ESP_LOGE(TAG, "RGB support not enabled");
        return NULL;
#endif
    } else if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_SPI) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_I80) == 0 ||
               strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_PARLIO) == 0) {
#ifdef CONFIG_SPIRAM
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                                        lcd_handles->io_handle,
                                                                        lcd_cfg->lcd_width,
                                                                        lcd_cfg->lcd_height,
                                                                        rotation);
#else
        disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(lcd_handles->panel_handle,
                                                                           lcd_handles->io_handle,
                                                                           lcd_cfg->lcd_width,
                                                                           lcd_cfg->lcd_height,
                                                                           rotation);
#endif
    } else {
        ESP_LOGE(TAG, "Unknown LCD sub_type: %s", lcd_cfg->sub_type);
        return NULL;
    }

    /* 1:1 RGB565 blends use the P4 PPA. lv_efc scales the clip before this. */
    disp_cfg.profile.enable_ppa_accel = true;

    ESP_LOGI(TAG, "Register LCD to LVGL adapter: sub_type=%s, size=%dx%d, tear_mode=%d",
             lcd_cfg->sub_type, lcd_cfg->lcd_width, lcd_cfg->lcd_height,
             (int)disp_cfg.tear_avoid_mode);
    return esp_lv_adapter_register_display(&disp_cfg);
}

void app_main(void)
{
    if (esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD) != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed");
        return;
    }

    esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display_lcd: %s", esp_err_to_name(ret));
        return;
    }
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize lcd_touch: %s, continue", esp_err_to_name(ret));
    }
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize lcd_brightness: %s, continue", esp_err_to_name(ret));
    }

    if (lcd_lvgl_adapter_init() != ESP_OK) {
        return;
    }

    void *lcd_handle = NULL;
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &lcd_handle);
    if (ret != ESP_OK || !lcd_handle) {
        ESP_LOGE(TAG, "Failed to get LCD device handle");
        return;
    }
    dev_display_lcd_config_t *lcd_cfg = NULL;
    ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_cfg);
    if (ret != ESP_OK || !lcd_cfg) {
        ESP_LOGE(TAG, "Failed to get LCD config");
        return;
    }
    s_lcd_w = lcd_cfg->lcd_width;
    s_lcd_h = lcd_cfg->lcd_height;

    lv_display_t *disp = lcd_lvgl_adapter_register_display(lcd_cfg, lcd_handle);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to register display to LVGL adapter");
        return;
    }

#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
    void *touch_handle = NULL;
    if (esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, &touch_handle) == ESP_OK && touch_handle) {
        dev_lcd_touch_handles_t *touch_handles = (dev_lcd_touch_handles_t *)touch_handle;
        const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handles->touch_handle);
        if (esp_lv_adapter_register_touch(&touch_cfg) == NULL) {
            ESP_LOGW(TAG, "Failed to add touch input");
        }
    }
#endif

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL adapter: %s", esp_err_to_name(ret));
        return;
    }

    /* Folder scan and the first frame run on the adapter task. */
    esp_lv_adapter_lock(-1);
    lv_timer_create(player_start, 1, NULL);
    esp_lv_adapter_unlock();
}
