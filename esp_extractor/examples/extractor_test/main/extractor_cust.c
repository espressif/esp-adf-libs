/* Customized extractor Demo code

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_extractor_reg.h"
#include "extractor_helper.h"

/**
 * @brief   Suppose your file is consisted by:
 *          File extension: myext
 *          File Header:
 *              MyCodec\n
 *              A: pcm 16000hz 2ch 16bit\n
 *              V: mjpeg 1280x720\n\n
 *          File body:
 *              A: pts[4bytes] size[4bytes] audiocontent
 *              V: pts[4bytes] size[4bytes] video content
 */
#define BUFFER_SIZE           1024
#define MY_EXTRACTOR_ID       "MyCodec\n"
#define AUDIO_FRAME_DURATION  (20)
#define VIDEO_FPS             (20)
#define MY_EXTRACTOR_TYPE     (esp_extractor_type_t) EXTRACTOR_4CC('m', 'y', 'e', 'x')

#define BREAK_ON_ERR(ret)  if (ret < 0) {  \
    break;                                 \
}
// #define USR_MEM_CONFIG

typedef struct {
    esp_extractor_stream_info_t  video_stream;
    esp_extractor_stream_info_t  audio_stream;
    bool                         audio_enable;
    bool                         video_enable;
} my_extract_t;

static void write_const_data(FILE *fp, uint8_t *data, int size, uint8_t pattern)
{
    memset(data, pattern, size > BUFFER_SIZE ? BUFFER_SIZE : size);
    while (size > 0) {
        int s = size > BUFFER_SIZE ? BUFFER_SIZE : size;
        fwrite(data, s, 1, fp);
        size -= s;
    }
}

static uint8_t get_audio_frame_pattern_by_pts(uint32_t pts)
{
    int audio_frame_idx = pts / AUDIO_FRAME_DURATION;
    return (uint8_t)(audio_frame_idx + 0xAA);
}

static uint8_t get_video_frame_pattern_by_pts(uint32_t pts)
{
    int video_frame_idx = pts / (1000 / VIDEO_FPS);
    return (uint8_t)(video_frame_idx + 0x55);
}

int my_extractor_gen_test(const char *url)
{
    FILE *fp = fopen(url, "wb");
    if (fp == NULL) {
        return -1;
    }
    uint8_t *write_buffer = (uint8_t *)malloc(BUFFER_SIZE);
    if (write_buffer == NULL) {
        fclose(fp);
        return -1;
    }
    int fps = 10;
    int sample_rate = 16000;
    // write headers
    const char *header = {MY_EXTRACTOR_ID "A:pcm %dhz 2ch 16bit\n"
                                          "V:mjpeg 1280x720 %dfps\n"
                                          "\n"};
    snprintf((char *)write_buffer, BUFFER_SIZE, header, sample_rate, fps);
    fwrite(write_buffer, strlen((char *)write_buffer), 1, fp);
    int file_duration = 10000;
    int audio_pts = 0;
    int video_pts = 0;
    int video_frame_duration = 1000 / VIDEO_FPS;
    int audio_sample_size = AUDIO_FRAME_DURATION * 4 * sample_rate / 1000;
    while (audio_pts < file_duration && video_pts < file_duration) {
        if (audio_pts < video_pts) {
            fwrite("A:", 2, 1, fp);
            fwrite(&audio_pts, 4, 1, fp);
            fwrite(&audio_sample_size, 4, 1, fp);
            write_const_data(fp, write_buffer, audio_sample_size, get_audio_frame_pattern_by_pts(audio_pts));
            audio_pts += AUDIO_FRAME_DURATION;
        } else {
            fwrite("V:", 2, 1, fp);
            fwrite(&video_pts, 4, 1, fp);
            int video_sample_size = 4096 + rand() % 1024;
            fwrite(&video_sample_size, 4, 1, fp);
            write_const_data(fp, write_buffer, video_sample_size, get_video_frame_pattern_by_pts(video_pts));
            video_pts += video_frame_duration;
        }
    }
    free(write_buffer);
    fclose(fp);
    return 0;
}

static int read_line(data_cache_t *cache)
{
    uint8_t d = 0;
    int n = 0;
    while (data_cache_read(cache, &d, 1) > 0) {
        n++;
        if (d == '\n') {
            return n;
        }
    }
    return 0;
}

static esp_extractor_err_t my_extractor_open(extractor_t *extractor, extractor_ctrl_list_t *ctrls)
{
    my_extract_t *my = calloc(1, sizeof(my_extract_t));
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_NO_MEM;
    }
    data_cache_t *cache = extractor->cache;
    data_cache_skip(cache, strlen(MY_EXTRACTOR_ID));
    int ret = -1;
    while (1) {
        char fmt[6];
        char *header = (char *)data_cache_get_cache_buffer(cache);
        int n = read_line(cache);
        if (n == 1) {
            ret = 0;
            break;
        }
        if (memcmp(header, "A:", 2) == 0) {
            int sample_rate = 0, channel = 0, bits = 0;
            n = sscanf(header + 2, "%s %dhz %dch %dbits", fmt, &sample_rate, &channel, &bits);
            if (n != 4) {
                break;
            }
            my->audio_stream.audio_info.sample_rate = sample_rate;
            my->audio_stream.audio_info.channel = (uint8_t)channel;
            my->audio_stream.audio_info.bits_per_sample = (uint8_t)bits;
            my->audio_stream.stream_type = ESP_EXTRACTOR_STREAM_TYPE_AUDIO;
            if (strcmp(fmt, "pcm") == 0) {
                my->audio_stream.audio_info.format = ESP_EXTRACTOR_AUDIO_FORMAT_PCM;
            }
        } else if (memcmp(header, "V:", 2) == 0) {
            int width = 0, height = 0, fps = 0;
            n = sscanf(header + 2, "%s %dx%d %dfps", fmt, &width, &height, &fps);
            if (n != 4) {
                ret = -1;
                break;
            }
            my->video_stream.video_info.width = (uint16_t)width;
            my->video_stream.video_info.height = (uint16_t)height;
            my->video_stream.video_info.fps = (uint8_t)fps;
            my->video_stream.stream_type = ESP_EXTRACTOR_STREAM_TYPE_VIDEO;
            if (strcmp(fmt, "mjpeg") == 0) {
                my->video_stream.video_info.format = ESP_EXTRACTOR_VIDEO_FORMAT_MJPEG;
            }
        } else {
            break;
        }
    }
    if (ret != 0) {
        free(my);
    }
    extractor->extractor_inst = my;
    return ret;
}

static esp_extractor_err_t my_extractor_get_stream_num(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                       uint16_t *stream_num)
{
    my_extract_t *my = (my_extract_t *)extractor->extractor_inst;
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO && my->video_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
        *stream_num = 1;
        return ESP_EXTRACTOR_ERR_OK;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO && my->audio_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
        *stream_num = 1;
        return ESP_EXTRACTOR_ERR_OK;
    }
    return ESP_EXTRACTOR_ERR_NOT_FOUND;
}

static esp_extractor_err_t my_extractor_close(extractor_t *extractor)
{
    my_extract_t *my = (my_extract_t *)extractor->extractor_inst;
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    free(my);
    return ESP_EXTRACTOR_ERR_OK;
}

static esp_extractor_err_t my_extractor_get_stream_info(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                        uint16_t stream_index, esp_extractor_stream_info_t *stream_info)
{
    my_extract_t *my = (my_extract_t *)extractor->extractor_inst;
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO && my->video_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
        memcpy(stream_info, &my->video_stream, sizeof(esp_extractor_stream_info_t));
        return ESP_EXTRACTOR_ERR_OK;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO && my->audio_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
        memcpy(stream_info, &my->audio_stream, sizeof(esp_extractor_stream_info_t));
        return ESP_EXTRACTOR_ERR_OK;
    }
    return ESP_EXTRACTOR_ERR_NOT_FOUND;
}

static esp_extractor_err_t my_extractor_enable_stream(extractor_t *extractor, esp_extractor_stream_type_t stream_type,
                                                      uint16_t stream_index, bool enable)
{
    my_extract_t *my = (my_extract_t *)extractor->extractor_inst;
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO && my->video_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
        my->video_enable = enable;
        return ESP_EXTRACTOR_ERR_OK;
    }
    if (stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO && my->audio_stream.stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
        my->audio_enable = enable;
        return ESP_EXTRACTOR_ERR_OK;
    }
    return ESP_EXTRACTOR_ERR_NOT_FOUND;
}

static esp_extractor_err_t my_extractor_read_frame(extractor_t *extractor, esp_extractor_frame_info_t *frame_info)
{
    my_extract_t *my = (my_extract_t *)extractor->extractor_inst;
    if (my == NULL) {
        return ESP_EXTRACTOR_ERR_INV_ARG;
    }
    char header[2];
    uint32_t pts;
    uint32_t frame_size;
    memset(frame_info, 0, sizeof(esp_extractor_frame_info_t));
    int ret = ESP_EXTRACTOR_ERR_OK;
    uint32_t last_pos = data_cache_get_position(extractor->cache);
    while (1) {
        ret = data_cache_read(extractor->cache, header, 2);
        if (ret != 2) {
            frame_info->frame_flag |= EXTRACTOR_FRAME_FLAG_EOS;
            return ESP_EXTRACTOR_ERR_EOS;
        }
        data_cache_read(extractor->cache, &pts, 4);
        data_cache_read(extractor->cache, &frame_size, 4);
        if (header[1] != ':') {
            return ESP_EXTRACTOR_ERR_READ;
        }
        bool skip = false;
        if (header[0] == 'A') {
            skip = (my->audio_enable == false);
            frame_info->stream_type = ESP_EXTRACTOR_STREAM_TYPE_AUDIO;
        } else if (header[0] == 'V') {
            skip = (my->video_enable == false);
            frame_info->stream_type = ESP_EXTRACTOR_STREAM_TYPE_VIDEO;
        }
        if (skip == false) {
            frame_info->pts = pts;
            bool over_size = false;
            frame_info->frame_buffer = extractor_malloc_output_pool(extractor, frame_size, &over_size);
            if (frame_info->frame_buffer) {
                frame_info->frame_size = frame_size;
                data_cache_read(extractor->cache, frame_info->frame_buffer, frame_size);
            } else {
                if (over_size) {
                    skip = true;
                    ret = ESP_EXTRACTOR_ERR_SKIPPED;
                } else {
                    data_cache_seek(extractor->cache, last_pos);
                    ret = ESP_EXTRACTOR_ERR_WAITING_OUTPUT;
                    break;
                }
            }
        }
        if (skip) {
            data_cache_skip(extractor->cache, frame_size);
            break;
        }
        break;
    }
    return ESP_EXTRACTOR_ERR_OK;
}

static esp_extractor_err_t my_extractor_probe(uint8_t *buffer, uint32_t size, uint32_t *sub_type)
{
    if (size >= 10 && memcmp(buffer, MY_EXTRACTOR_ID, strlen(MY_EXTRACTOR_ID)) == 0) {
        return ESP_EXTRACTOR_ERR_OK;
    }
    return ESP_EXTRACTOR_ERR_FAIL;
}

esp_extractor_err_t my_extractor_register(void)
{
    static const extractor_reg_info_t table = {
        .probe = my_extractor_probe,
        .open = my_extractor_open,
        .get_stream_num = my_extractor_get_stream_num,
        .get_stream_info = my_extractor_get_stream_info,
        .read_frame = my_extractor_read_frame,
        .enable_stream = my_extractor_enable_stream,
        .close = my_extractor_close,
    };
    return esp_extractor_register(MY_EXTRACTOR_TYPE, &table);
}

static bool all_same(esp_extractor_frame_info_t *frame, uint8_t pattern)
{
    for (int i = 0; i < frame->frame_size; i++) {
        if (frame->frame_buffer[i] != pattern) {
            return false;
        }
    }
    return true;
}

bool my_extractor_frame_verify(esp_extractor_frame_info_t *frame)
{
    if (frame->frame_buffer == NULL || frame->frame_size == 0) {
        return false;
    }
    if (frame->stream_type == ESP_EXTRACTOR_STREAM_TYPE_AUDIO) {
        return all_same(frame, get_audio_frame_pattern_by_pts(frame->pts));
    }
    if (frame->stream_type == ESP_EXTRACTOR_STREAM_TYPE_VIDEO) {
        return all_same(frame, get_video_frame_pattern_by_pts(frame->pts));
    }
    return false;
}
