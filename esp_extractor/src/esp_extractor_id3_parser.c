/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <string.h>
#include "esp_extractor_ctrl.h"
#include "esp_extractor_id3_parser.h"
#include "esp_log.h"

#define TAG                   "ID3"
#define ID3_V1_TAG_SIZE       (128)
#define ID3_V1_FIELD_TITLE    (30)
#define ID3_V1_FIELD_AUTHOR   (30)
#define ID3_V1_FIELD_ALBUM    (30)
#define ID3_V1_FIELD_DATE     (4)
#define ID3_V1_FIELD_GENRE    (1)
#define ID3_V2_HEADER_SIZE    (10)
#define ID3_V2_FRAME_ID_SIZE  (4)
#define ID3_V2_MAX_EXTRA      (8)

typedef struct {
    esp_extractor_id3_info_t  info;
    bool                      parsed;
} esp_extractor_id3_parser_t;

void *media_lib_module_malloc(const char *module, size_t size);
void media_lib_free(void *ptr);
#define id3_malloc(size)  media_lib_module_malloc("ID3", size)

static uint32_t id3_read_be32(uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
}

static uint32_t id3_read_syncsafe(uint8_t *data)
{
    return ((uint32_t)(data[0] & 0x7F) << 21) | ((uint32_t)(data[1] & 0x7F) << 14) |
           ((uint32_t)(data[2] & 0x7F) << 7) | (data[3] & 0x7F);
}

static uint32_t id3_read_be24(uint8_t *data)
{
    return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

static void id3_free_info(esp_extractor_id3_info_t *info)
{
    if (info == NULL) {
        return;
    }
    media_lib_free(info->title);
    media_lib_free(info->author);
    media_lib_free(info->album);
    media_lib_free(info->date);
    media_lib_free(info->genre);
    media_lib_free(info->cover_mime);
    media_lib_free(info->cover);
    if (info->extra) {
        for (int i = 0; i < info->extra_num; i++) {
            media_lib_free(info->extra[i].key);
            media_lib_free(info->extra[i].value);
        }
        media_lib_free(info->extra);
    }
    memset(info, 0, sizeof(esp_extractor_id3_info_t));
}

static char *id3_copy_trim(const uint8_t *data, uint32_t size)
{
    while (size && (data[size - 1] == '\0' || data[size - 1] == ' ')) {
        size--;
    }
    while (size && (*data == '\0' || *data == ' ')) {
        data++;
        size--;
    }
    char *str = (char *)id3_malloc(size + 1);
    if (str == NULL) {
        return NULL;
    }
    uint32_t out = 0;
    for (uint32_t i = 0; i < size; i++) {
        if (data[i]) {
            str[out++] = (char)data[i];
        }
    }
    str[out] = '\0';
    return str;
}

static char *id3_copy_text(const uint8_t *data, uint32_t size)
{
    if (size == 0) {
        return NULL;
    }
    // First byte is text encoding. Keep bytes as-is, only drop separators.
    return id3_copy_trim(data + 1, size - 1);
}

static uint8_t id3_get_text_encoding(const uint8_t *data, uint32_t size)
{
    if (size == 0 || data[0] > ESP_EXTRACTOR_ID3_TEXT_ENCODING_UTF_8) {
        return ESP_EXTRACTOR_ID3_TEXT_ENCODING_NONE;
    }
    return data[0];
}

static void id3_replace_string(char **dst, char *value)
{
    if (value == NULL) {
        return;
    }
    media_lib_free(*dst);
    *dst = value;
}

static bool id3_is_same_key(const char *frame_id, const char *key)
{
    return memcmp(frame_id, key, strlen(key)) == 0;
}

static void id3_add_extra(esp_extractor_id3_info_t *info, const char *key, char *value, uint8_t encoding)
{
    if (value == NULL) {
        return;
    }
    if (info->extra_num >= ID3_V2_MAX_EXTRA) {
        media_lib_free(value);
        return;
    }
    if (info->extra == NULL) {
        info->extra = (esp_extractor_id3_kv_t *)id3_malloc(ID3_V2_MAX_EXTRA * sizeof(esp_extractor_id3_kv_t));
        if (info->extra == NULL) {
            media_lib_free(value);
            return;
        }
        memset(info->extra, 0, ID3_V2_MAX_EXTRA * sizeof(esp_extractor_id3_kv_t));
    }
    char *key_copy = id3_copy_trim((const uint8_t *)key, strlen(key));
    if (key_copy == NULL) {
        media_lib_free(value);
        return;
    }
    info->extra[info->extra_num].key = key_copy;
    info->extra[info->extra_num].value = value;
    info->extra[info->extra_num].encoding = encoding;
    info->extra_num++;
}

static void id3_keep_text_frame(esp_extractor_id3_info_t *info, const char *frame_id, const uint8_t *data, uint32_t size)
{
    uint8_t encoding = id3_get_text_encoding(data, size);
    char *value = id3_copy_text(data, size);
    if (value == NULL) {
        return;
    }
    info->encoding = encoding;
    if (id3_is_same_key(frame_id, "TIT2") || id3_is_same_key(frame_id, "TT2")) {
        id3_replace_string(&info->title, value);
    } else if (id3_is_same_key(frame_id, "TPE1") || id3_is_same_key(frame_id, "TP1")) {
        id3_replace_string(&info->author, value);
    } else if (id3_is_same_key(frame_id, "TALB") || id3_is_same_key(frame_id, "TAL")) {
        id3_replace_string(&info->album, value);
    } else if (id3_is_same_key(frame_id, "TDRC") || id3_is_same_key(frame_id, "TYER") || id3_is_same_key(frame_id, "TYE")) {
        id3_replace_string(&info->date, value);
    } else if (id3_is_same_key(frame_id, "TCON") || id3_is_same_key(frame_id, "TCO")) {
        id3_replace_string(&info->genre, value);
    } else {
        id3_add_extra(info, frame_id, value, encoding);
    }
}

static char *id3_copy_v1_genre(uint8_t genre)
{
    char *str = (char *)id3_malloc(4);
    if (str == NULL) {
        return NULL;
    }
    int pos = 0;
    if (genre >= 100) {
        str[pos++] = '0' + genre / 100;
    }
    if (genre >= 10) {
        str[pos++] = '0' + genre / 10 % 10;
    }
    str[pos++] = '0' + genre % 10;
    str[pos] = '\0';
    return str;
}

static void id3_keep_apic_frame(esp_extractor_id3_info_t *info, const uint8_t *data, uint32_t size)
{
    if (size < 5) {
        return;
    }
    uint32_t pos = 1;
    while (pos < size && data[pos]) {
        pos++;
    }
    if (pos >= size) {
        return;
    }
    char *mime = id3_copy_trim(data + 1, pos - 1);
    pos++;  // MIME terminator
    if (pos >= size) {
        media_lib_free(mime);
        return;
    }
    pos++;  // Picture type
    while (pos < size && data[pos]) {
        pos++;
    }
    if (pos >= size) {
        media_lib_free(mime);
        return;
    }
    pos++;  // Description terminator
    uint32_t cover_size = size - pos;
    uint8_t *cover = (uint8_t *)id3_malloc(cover_size);
    if (cover == NULL) {
        media_lib_free(mime);
        return;
    }
    memcpy(cover, data + pos, cover_size);
    media_lib_free(info->cover_mime);
    media_lib_free(info->cover);
    info->cover_mime = mime;
    info->cover = cover;
    info->cover_size = cover_size;
}

static int id3_read_full(id3_read_cb reader, void *read_ctx, uint8_t *data, uint32_t size)
{
    int ret = reader(data, size, false, read_ctx);
    if (ret < 0) {
        return ret;
    }
    return ret == (int)size ? 0 : -1;
}

static int id3_skip(id3_read_cb reader, void *read_ctx, uint32_t size)
{
    uint8_t dummy = 0;
    int ret = reader(&dummy, size, true, read_ctx);
    if (ret < 0) {
        return ret;
    }
    return (ret == 0 || ret == (int)size) ? 0 : -1;
}

static int id3_parse_v1(id3_read_cb reader, void *read_ctx, esp_extractor_id3_parser_t *parser, uint8_t *first)
{
    uint8_t tag[ID3_V1_TAG_SIZE];
    memcpy(tag, first, ID3_V2_HEADER_SIZE);
    if (id3_read_full(reader, read_ctx, tag + ID3_V2_HEADER_SIZE, ID3_V1_TAG_SIZE - ID3_V2_HEADER_SIZE) != 0) {
        return -1;
    }
    if (memcmp(tag, "TAG", 3) != 0) {
        return -1;
    }
    id3_replace_string(&parser->info.title, id3_copy_trim(tag + 3, ID3_V1_FIELD_TITLE));
    id3_replace_string(&parser->info.author, id3_copy_trim(tag + 33, ID3_V1_FIELD_AUTHOR));
    id3_replace_string(&parser->info.album, id3_copy_trim(tag + 63, ID3_V1_FIELD_ALBUM));
    id3_replace_string(&parser->info.date, id3_copy_trim(tag + 93, ID3_V1_FIELD_DATE));
    id3_replace_string(&parser->info.genre, id3_copy_v1_genre(tag[127]));
    parser->info.encoding = ESP_EXTRACTOR_ID3_TEXT_ENCODING_ISO_8859_1;
    parser->parsed = true;
    return 0;
}

static int id3_parse_v2(id3_read_cb reader, void *read_ctx, esp_extractor_id3_parser_t *parser, uint8_t *header)
{
    uint8_t version = header[3];
    uint8_t flags = header[5];
    uint32_t left = id3_read_syncsafe(header + 6);
    if (version < 2 || version > 4) {
        return -1;
    }
    if ((flags & 0x40) && left >= 4) {
        uint8_t ext_header[4];
        if (id3_read_full(reader, read_ctx, ext_header, sizeof(ext_header)) != 0) {
            return -1;
        }
        uint32_t ext_size = version == 4 ? id3_read_syncsafe(ext_header) : id3_read_be32(ext_header);
        if (version == 4) {
            if (ext_size < sizeof(ext_header) || ext_size > left) {
                return -1;
            }
            if (id3_skip(reader, read_ctx, ext_size - sizeof(ext_header)) != 0) {
                return -1;
            }
            left -= ext_size;
        } else {
            if (ext_size > left - sizeof(ext_header)) {
                return -1;
            }
            if (id3_skip(reader, read_ctx, ext_size) != 0) {
                return -1;
            }
            left -= ext_size + sizeof(ext_header);
        }
    }
    while (left >= (version == 2 ? 6 : 10)) {
        uint8_t frame_header[10] = {0};
        uint32_t header_size = version == 2 ? 6 : 10;
        if (id3_read_full(reader, read_ctx, frame_header, header_size) != 0) {
            return -1;
        }
        left -= header_size;
        if (frame_header[0] == 0) {
            break;
        }
        uint32_t frame_size = version == 2 ? id3_read_be24(frame_header + 3) : (version == 4 ? id3_read_syncsafe(frame_header + 4) : id3_read_be32(frame_header + 4));
        if (frame_size == 0 || frame_size > left) {
            break;
        }
        bool keep = (frame_header[0] == 'T') || (version == 2 ? memcmp(frame_header, "PIC", 3) == 0 : memcmp(frame_header, "APIC", 4) == 0);
        if (keep) {
            uint8_t *frame = (uint8_t *)id3_malloc(frame_size);
            if (frame == NULL) {
                return -1;
            }
            if (id3_read_full(reader, read_ctx, frame, frame_size) != 0) {
                media_lib_free(frame);
                return -1;
            }
            char frame_id[ID3_V2_FRAME_ID_SIZE + 1] = {0};
            memcpy(frame_id, frame_header, version == 2 ? 3 : 4);
            if (frame_header[0] == 'T') {
                id3_keep_text_frame(&parser->info, frame_id, frame, frame_size);
            } else {
                id3_keep_apic_frame(&parser->info, frame, frame_size);
            }
            media_lib_free(frame);
        } else if (id3_skip(reader, read_ctx, frame_size) != 0) {
            return -1;
        }
        left -= frame_size;
    }
    if (left) {
        id3_skip(reader, read_ctx, left);
    }
    parser->parsed = true;
    return 0;
}

static int id3_parse(id3_read_cb reader, void *read_ctx, void *parse_ctx)
{
    esp_extractor_id3_parser_t *parser = (esp_extractor_id3_parser_t *)parse_ctx;
    uint8_t header[ID3_V2_HEADER_SIZE];
    if (parser == NULL || reader == NULL) {
        return -1;
    }
    if (id3_read_full(reader, read_ctx, header, sizeof(header)) != 0) {
        return -1;
    }
    if (memcmp(header, "ID3", 3) == 0) {
        return id3_parse_v2(reader, read_ctx, parser, header);
    }
    if (memcmp(header, "TAG", 3) == 0) {
        return id3_parse_v1(reader, read_ctx, parser, header);
    }
    return -1;
}

esp_extractor_err_t esp_extractor_id3_parser_open(esp_extractor_handle_t extractor,
                                                  esp_extractor_id3_parser_hd_t *handle)
{
    if (extractor == NULL || handle == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    esp_extractor_id3_parser_t *parser = (esp_extractor_id3_parser_t *)id3_malloc(sizeof(esp_extractor_id3_parser_t));
    if (parser == NULL) {
        return ESP_EXTRACTOR_ERR_NO_MEM;
    }
    memset(parser, 0, sizeof(esp_extractor_id3_parser_t));
    parser->info.encoding = ESP_EXTRACTOR_ID3_TEXT_ENCODING_NONE;
    esp_extractor_id3_parse_cfg_t cfg = {
        .parse_cb = id3_parse,
        .parse_ctx = parser,
    };
    esp_extractor_err_t ret = esp_extractor_ctrl(extractor, ESP_EXTRACTOR_CTRL_TYPE_SET_ID3_PARSER, &cfg, sizeof(cfg));
    if (ret != ESP_EXTRACTOR_ERR_OK) {
        ESP_LOGE(TAG, "Failed to set ID3 parser ret %d", ret);
        media_lib_free(parser);
        return ret;
    }
    *handle = parser;
    return ESP_EXTRACTOR_ERR_OK;
}

esp_extractor_err_t esp_extractor_id3_parser_get_info(esp_extractor_id3_parser_hd_t handle,
                                                      const esp_extractor_id3_info_t **info)
{
    if (handle == NULL || info == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    esp_extractor_id3_parser_t *parser = (esp_extractor_id3_parser_t *)handle;
    if (parser->parsed == false) {
        ESP_LOGW(TAG, "ID3 not parsed yet");
        return ESP_EXTRACTOR_ERR_NOT_FOUND;
    }
    *info = &parser->info;
    return ESP_EXTRACTOR_ERR_OK;
}

esp_extractor_err_t esp_extractor_id3_parser_close(esp_extractor_id3_parser_hd_t handle)
{
    if (handle == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    esp_extractor_id3_parser_t *parser = (esp_extractor_id3_parser_t *)handle;
    id3_free_info(&parser->info);
    media_lib_free(parser);
    return ESP_EXTRACTOR_ERR_OK;
}
