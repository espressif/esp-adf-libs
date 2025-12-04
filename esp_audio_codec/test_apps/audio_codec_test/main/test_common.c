/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "test_common.h"

#define SDCARD_FILE_PREV_NAME           "file:/"
#define SDCARD_SCAN_URL_MAX_LENGTH      (1024 * 2)
#define SDCARD_LIST_URL_MAX_LENGTH      (1024 * 2)
#define SDCARD_DEFAULT_DIR_NAME         "/sdcard/__playlist"
#define SDCARD_DEFAULT_URL_FILE_NAME    "/sdcard/__playlist/_playlist_url"
#define SDCARD_DEFAULT_OFFSET_FILE_NAME "/sdcard/__playlist/_offset"
#define SDCARD_URL_FILE_NAME_LENGTH     (strlen(SDCARD_DEFAULT_URL_FILE_NAME) + 10)
#define SDCARD_OFFSET_FILE_NAME_LENGTH  (strlen(SDCARD_DEFAULT_OFFSET_FILE_NAME) + 10)
#define CHECK_ERROR(TAG, para, action) {                                      \
    if ((para) == false) {                                                    \
        ESP_LOGE(TAG, "unexpect action in sdcard list, line: %d", __LINE__);  \
        action;                                                               \
    }                                                                         \
}

static const char *TAG = "UT_COMMON";

typedef struct sdcard_list {
    char    *save_file_name;         /*!< Name of file to save URLs */
    char    *offset_file_name;       /*!< Name of file to save offset */
    FILE    *save_file;              /*!< File to save urls */
    FILE    *offset_file;            /*!< File to save offset of urls */
    char    *cur_url;                /*!< Point to current URL */
    uint16_t url_num;                /*!< Number of URLs */
    uint16_t cur_url_id;             /*!< Current url ID */
    uint32_t total_size_save_file;   /*!< Size of file to save URLs */
    uint32_t total_size_offset_file; /*!< Size of file to save offset */
} sdcard_list_t;

static void scan_dir(sdcard_scan_cb_t save_url_cb, const char *path, int cur_depth, int depth, const char *file_extension[], int filter_num, void *user_data)
{
    if (cur_depth > depth) {
        ESP_LOGD(TAG, "scan depth = %d, exit", cur_depth);
        return;
    }
    char *file_url = calloc(1, SDCARD_SCAN_URL_MAX_LENGTH);
    if (file_url == NULL) {
        ESP_LOGE(TAG, "Failed to malloc memory for file_url");
        return;
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Open [%s] directory failed", path);
        free(file_url);
        return;
    }

    struct dirent *file_info = NULL;
    while (NULL != (file_info = readdir(dir))) {
        if ((strlen(file_info->d_name) + strlen(path)) > (SDCARD_SCAN_URL_MAX_LENGTH - strlen(SDCARD_FILE_PREV_NAME))) {
            ESP_LOGE(TAG, "The file name is too long, invalid url");
            continue;
        }
        if (file_info->d_name[0] == '.') {
            continue;
        }

        if (file_info->d_type == DT_DIR) {
            if (file_info->d_name[0] == '_' && file_info->d_name[1] == '_') {
                continue;
            }
            memset(file_url, 0, SDCARD_SCAN_URL_MAX_LENGTH);
            sprintf(file_url, "%s/%s", path, file_info->d_name);
            scan_dir(save_url_cb, file_url, cur_depth + 1, depth, file_extension, filter_num, user_data);
        } else {
            memset(file_url, 0, SDCARD_SCAN_URL_MAX_LENGTH);
            sprintf(file_url, "%s%s/%s", SDCARD_FILE_PREV_NAME, path, file_info->d_name);
            if (NULL == file_extension) {
                save_url_cb(user_data, file_url);
                continue;
            }
            char *detect = strrchr(file_info->d_name, '.');
            if (NULL == detect) {
                continue;
            }
            detect++;
            for (int i = 0; i < filter_num; i++) {
                if (strcasecmp(detect, file_extension[i]) == 0) {
                    save_url_cb(user_data, file_url);
                    break;
                }
            }
        }
    }
    free(file_url);
    closedir(dir);
}

static esp_err_t sdcard_list_open(sdcard_list_t *playlist, uint8_t list_id)
{
    playlist->save_file_name = calloc(1, SDCARD_URL_FILE_NAME_LENGTH);
    if (NULL == playlist->save_file_name) {
        ESP_LOGE(TAG, "save_file_name calloc failed");
        return ESP_FAIL;
    }
    playlist->offset_file_name = calloc(1, SDCARD_OFFSET_FILE_NAME_LENGTH);
    if (NULL == playlist->offset_file_name) {
        ESP_LOGE(TAG, "offset_file_name calloc failed");
        free(playlist->save_file_name);
        return ESP_FAIL;
    }

    sprintf(playlist->save_file_name, "%s%d", SDCARD_DEFAULT_URL_FILE_NAME, list_id);
    sprintf(playlist->offset_file_name, "%s%d", SDCARD_DEFAULT_OFFSET_FILE_NAME, list_id);

    mkdir(SDCARD_DEFAULT_DIR_NAME, 0777);

    playlist->save_file = fopen(playlist->save_file_name, "w+");
    playlist->offset_file = fopen(playlist->offset_file_name, "w+");

    if (playlist->save_file == NULL || NULL == playlist->offset_file) {
        ESP_LOGE(TAG, "open file error, line: %d, have you mounted sdcard, set the long file name and UTF-8 encoding configuration ?", __LINE__);
        free(playlist->save_file_name);
        free(playlist->offset_file_name);
        if (playlist->save_file) {
            fclose(playlist->save_file);
        }
        if (playlist->offset_file) {
            fclose(playlist->offset_file);
        }
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t sdcard_list_choose_id(sdcard_list_t *playlist, int id, char **url_buff)
{
    uint32_t pos = 0;
    uint16_t size = 0;
    int offset = id * (sizeof(uint32_t) + sizeof(uint16_t));
    CHECK_ERROR(TAG, ((fseek(playlist->offset_file, offset, SEEK_SET)) == 0), return ESP_FAIL);
    CHECK_ERROR(TAG, ((fread(&pos, 1, sizeof(uint32_t), playlist->offset_file)) == sizeof(uint32_t)), return ESP_FAIL);
    CHECK_ERROR(TAG, ((fread(&size, 1, sizeof(uint16_t), playlist->offset_file)) == sizeof(uint16_t)), return ESP_FAIL);
    if (playlist->cur_url) {
        free(playlist->cur_url);
    }
    playlist->cur_url = (char *)calloc(1, size + 1);
    if (playlist->cur_url == NULL) {
        ESP_LOGE(TAG, "Fail to allocate memory for url");
        fclose(playlist->offset_file);
        fclose(playlist->save_file);
        return ESP_FAIL;
    }
    CHECK_ERROR(TAG, ((fseek(playlist->save_file, pos, SEEK_SET)) == 0), return ESP_FAIL);
    CHECK_ERROR(TAG, ((fread(playlist->cur_url, 1, size, playlist->save_file)) == size), return ESP_FAIL);
    playlist->cur_url[size] = 0;
    playlist->cur_url_id = id;
    *url_buff = playlist->cur_url;
    return ESP_OK;
}

static esp_err_t save_url_to_sdcard(sdcard_list_t *playlist, const char *path)
{
    if (playlist->save_file == NULL || playlist->offset_file == NULL) {
        ESP_LOGE(TAG, "The file to save playlist failed to open");
        return ESP_FAIL;
    }
    uint16_t len = strlen(path);
    CHECK_ERROR(TAG, ((fseek(playlist->offset_file, playlist->total_size_offset_file, SEEK_SET)) == 0), return ESP_FAIL);
    CHECK_ERROR(TAG, ((fseek(playlist->save_file, playlist->total_size_save_file, SEEK_SET)) == 0), return ESP_FAIL);
    CHECK_ERROR(TAG, (fwrite(path, 1, len, playlist->save_file) == len), return ESP_FAIL);
    CHECK_ERROR(TAG, (fwrite(&playlist->total_size_save_file, 1, sizeof(uint32_t), playlist->offset_file) == sizeof(uint32_t)), return ESP_FAIL);
    CHECK_ERROR(TAG, (fwrite(&len, 1, sizeof(uint16_t), playlist->offset_file)) == sizeof(uint16_t), return ESP_FAIL);
    CHECK_ERROR(TAG, (fsync(fileno(playlist->save_file)) == 0), return ESP_FAIL);
    CHECK_ERROR(TAG, (fsync(fileno(playlist->offset_file)) == 0), return ESP_FAIL);
    playlist->total_size_save_file += len;
    playlist->total_size_offset_file += (sizeof(uint16_t) + sizeof(uint32_t));
    playlist->url_num++;

    return ESP_OK;
}

static esp_err_t sdcard_list_close(sdcard_list_t *playlist)
{
    fclose(playlist->offset_file);
    fclose(playlist->save_file);
    playlist->offset_file = NULL;
    playlist->save_file = NULL;
    return ESP_OK;
}

esp_err_t sdcard_scan(sdcard_scan_cb_t cb, const char *path, int depth, const char *file_extension[], int filter_num, void *user_data)
{
    if (depth < 0 || filter_num < 0) {
        ESP_LOGE(TAG, "Invalid parameters, please check");
        return ESP_FAIL;
    }
    scan_dir(cb, path, 0, depth, file_extension, filter_num, user_data);
    return ESP_OK;
}

esp_err_t sdcard_list_create(void **handle)
{
    esp_err_t ret = ESP_OK;
    sdcard_list_t *sdcard_list = (sdcard_list_t *)calloc(1, sizeof(sdcard_list_t));
    if (sdcard_list == NULL) {
        ESP_LOGE(TAG, "sdcard_list malloc fail");
        return ESP_FAIL;
    };
    static int list_id;
    ret |= sdcard_list_open(sdcard_list, list_id++);
    if (ret != ESP_OK) {
        free(sdcard_list);
        return ESP_FAIL;
    }
    *handle = sdcard_list;
    return ESP_OK;
}

esp_err_t sdcard_list_save(void *handle, const char *url)
{
    sdcard_list_t *playlist = (sdcard_list_t *)handle;
    esp_err_t ret = ESP_OK;
    uint16_t len = strlen(url);
    if (len >= SDCARD_LIST_URL_MAX_LENGTH) {
        ESP_LOGE(TAG, "The url length is greater than MAX LENTGTH, you should change the SDCARD_LIST_URL_MAX_LENGTH value");
        return ESP_FAIL;
    }
    ret = save_url_to_sdcard(playlist, url);
    return ret;
}

esp_err_t sdcard_list_choose(void *handle, int url_id, char **url_buff)
{
    sdcard_list_t *playlist = (sdcard_list_t *)handle;
    if (playlist->url_num == 0) {
        ESP_LOGE(TAG, "No url, please save urls to playlist first");
        return ESP_FAIL;
    }
    if ((url_id < 0) || (url_id >= playlist->url_num)) {
        ESP_LOGE(TAG, "Invalid url id to be choosen");
        return ESP_FAIL;
    }

    return sdcard_list_choose_id(playlist, url_id, url_buff);
}

int sdcard_list_get_url_num(void *handle)
{
    sdcard_list_t *playlist = (sdcard_list_t *)handle;
    return playlist->url_num;
}

esp_err_t sdcard_list_destroy(void *handle)
{
    sdcard_list_t *playlist = (sdcard_list_t *)handle;
    sdcard_list_close(playlist);
    remove(playlist->save_file_name);
    remove(playlist->offset_file_name);
    free(playlist->save_file_name);
    free(playlist->offset_file_name);
    if (playlist->cur_url) {
        free(playlist->cur_url);
        playlist->cur_url = NULL;
    }
    free(playlist);
    return ESP_OK;
}
