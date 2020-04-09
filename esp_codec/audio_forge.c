/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD.>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * The module is multifunctional audio forge which contains  resample, downmix, ALC, equalizer and sonic. Choose a combination of several function by `component_select`.
 *
 *  resample: Change sample rate or number of channels for source stream.
 *  downmix: Down-mix different source streams or stereo source stream.The downmix supports less and equal than  `SOURCE_NUM_MAX` source files.
 *  ALC: Change volume of source file.
 *  equalizer: Modify a frequency response to compensate for distortion.
 *  sonic: Change the speed and pitch of source file.
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "audio_forge.h"

// #define AUDIO_FORGE_SPEED_ANALYSIS
// #define AUDIO_FORGE_MEMORY_ANALYSIS
// #define DEBUG_AUDIO_FORGE_ISSUE

#ifdef AUDIO_FORGE_SPEED_ANALYSIS
int audio_forge_start_time;
int audio_forge_stop_time;
int audio_forge_whole_time;
int audio_forge_decode_time;
int audio_forge_write_time;
long pcmcnt;
#endif

#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
int startmemory_inram;
int stopmemory_inram;
int startmemory_total;
int stopmemory_total;
long pcmcnt;
#endif

#ifdef DEBUG_AUDIO_FORGE_ISSUE
FILE *in_file[SOURCE_NUM_MAX];
FILE *out_file;
char in_name[SOURCE_NUM_MAX][100];
char out_name[100];
#endif

#define NUMBER_BAND (10)
#define USE_XMMS_ORIGINAL_FREQENT (0)
int audio_forge_set_gain_value[NUMBER_BAND * 2] = { -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13};

static const char *TAG = "AUDIO_FORGE";

typedef struct {
    void *downmix_handle;
    esp_downmix_info_t downmix;
    unsigned char **inbuf;
    unsigned char *outbuf;
    int tick_to_wait;
    void **rsp_handle;
    resample_info_t *rsp_info;
    unsigned char **rsp_in;
    unsigned char **rsp_out;
    int *in_offset;
    audio_forge_select_t component_select;
    void *volume_handle;
    void *eq_handle;
    int *equalizer_gain;
    void *sonic_handle;
    float sonic_pitch;
    float sonic_speed;

    int sample_rate;
    int channel;
    int volume;
    int reflag;
    int max_sample;
    int sonic_num;
} audio_forge_t;

static esp_err_t audio_forge_open(audio_element_handle_t self)
{
    ESP_LOGI(TAG, "audio_forge opened");
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
    audio_forge_start_time = periph_tick_get();
    audio_forge_stop_time = 0;
    audio_forge_whole_time = 0;
    audio_forge_decode_time = 0;
    audio_forge_write_time = 0;
    pcmcnt = 0;
#endif
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
    startmemory_inram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    startmemory_total = esp_get_free_heap_size();
#endif
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    audio_forge->sonic_num = 1;
    if ((audio_forge->component_select == AUDIO_FORGE_SELECT_RESAMPLE)
        && (audio_forge->channel != audio_forge->rsp_info[0].src_ch)) {
        audio_forge->downmix.source_num = 1;
        audio_forge->component_select = AUDIO_FORGE_SELECT_RESAMPLE | AUDIO_FORGE_SELECT_DOWNMIX;
    }
    if (!(audio_forge->component_select & (AUDIO_FORGE_SELECT_RESAMPLE | AUDIO_FORGE_SELECT_DOWNMIX))) {
        audio_forge->sample_rate = audio_forge->downmix.source_info[0].samplerate;
        audio_forge->channel = audio_forge->downmix.source_info[0].channel;
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX) {
        //downmix
        audio_forge->downmix.source_num = audio_forge->downmix.source_num > SOURCE_NUM_MAX ?
                                          SOURCE_NUM_MAX : audio_forge->downmix.source_num;
        for (int i = 0; i < audio_forge->downmix.source_num; i++) {
            audio_forge->in_offset[i] = 0;
            if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
                //resample
                audio_forge->rsp_info[i].dest_rate = audio_forge->sample_rate;
                audio_forge->rsp_info[i].dest_ch = audio_forge->rsp_info[i].src_ch;
                audio_forge->rsp_info[i].mode = RESAMPLE_ENCODE_MODE;
                audio_forge->rsp_info[i].max_indata_bytes = audio_forge->max_sample * sizeof(
                            short) * 6 * audio_forge->rsp_info[i].src_ch; //48000/8000 = 6, so need 6 times
                audio_forge->rsp_info[i].out_len_bytes = audio_forge->max_sample * sizeof(
                            short) * audio_forge->rsp_info[i].src_ch;
                audio_forge->rsp_info[i].complexity = 5;
                audio_forge->rsp_info[i].sample_bits = 16;
                audio_forge->rsp_info[i].type = -1;
                audio_forge->rsp_handle[i] = esp_resample_create((void *)&audio_forge->rsp_info[i],
                                             &audio_forge->rsp_in[i], &audio_forge->rsp_out[i]);
                if (audio_forge->rsp_handle[i] == NULL) {
                    ESP_LOGE(TAG, "audio_forge create the handle for resample failed, (line %d)", __LINE__);
                    return ESP_FAIL;
                }
                audio_forge->downmix.source_info[i].samplerate = audio_forge->sample_rate;
            }

            if ((audio_forge->sample_rate != audio_forge->downmix.source_info[i].samplerate)
                && (audio_forge->downmix.source_info[i].samplerate < SAMPLERATE_MIN
                    || audio_forge->downmix.source_info[i].samplerate >= SAMPLERATE_MAX)) {
                ESP_LOGE(TAG, "The stream sample rate is out of range. (line %d)", __LINE__);
                return ESP_ERR_INVALID_ARG;
            }

            if ((audio_forge->downmix.source_info[i].channel != 1)
                && (audio_forge->downmix.source_info[i].channel != 2)) {
                ESP_LOGE(TAG, "The number of channels should be either 1 or 2. (line %d)", __LINE__);
                return ESP_ERR_INVALID_ARG;
            }

            if (audio_forge->downmix.source_info[i].gain[0] < GAIN_MIN
                || audio_forge->downmix.source_info[i].gain[0] > GAIN_MAX
                || audio_forge->downmix.source_info[i].gain[1] < GAIN_MIN
                || audio_forge->downmix.source_info[i].gain[1] > GAIN_MAX) {
                ESP_LOGE(TAG, "The gain is out (%d, %d) range", GAIN_MIN, GAIN_MAX);
                return ESP_ERR_INVALID_ARG;
            }

            if (audio_forge->downmix.source_info[i].transit_time < 0) {
                ESP_LOGE(TAG, "The transit_time (%d) must be equal or gather than zero",
                         audio_forge->downmix.source_info[0].transit_time);
                return ESP_ERR_INVALID_ARG;
            }

            audio_forge->downmix.source_info[i].bits_num = 16;
            if (i > 0) {
                audio_forge->inbuf[i] = (unsigned char *)audio_calloc(1,
                                        audio_forge->max_sample * sizeof(short) * 2);
                if (audio_forge->inbuf[i] == NULL) {
                    ESP_LOGE(TAG, "Failed to audio_calloc of audio_forge->inbuf[%d]. (line %d)", i, __LINE__);
                    return ESP_ERR_NO_MEM;
                }
            }
#ifdef DEBUG_AUDIO_FORGE_ISSUE
            in_file[i] = fopen(in_name[i], "rb");
#endif
        }
        audio_forge->downmix.mode = ESP_DOWNMIX_WORK_MODE_SWITCH_ON;
        audio_forge->downmix.output_type = audio_forge->channel;
        audio_forge->downmix.out_ctx = ESP_DOWNMIX_OUT_CTX_LEFT_RIGHT;
        audio_forge->downmix_handle = esp_downmix_open(&audio_forge->downmix);
        if (audio_forge->downmix_handle == NULL) {
            ESP_LOGE(TAG, "audio_forge create the handle for down-mix failed, (line %d)", __LINE__);
            return ESP_FAIL;
        }
    } else {
        audio_forge->in_offset[0] = 0;
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
            //resample
            audio_forge->rsp_info[0].dest_rate = audio_forge->sample_rate;
            audio_forge->rsp_info[0].dest_ch = audio_forge->channel;
            audio_forge->rsp_info[0].mode = RESAMPLE_ENCODE_MODE;
            audio_forge->rsp_info[0].max_indata_bytes = audio_forge->max_sample * sizeof(short) * 6 * audio_forge->rsp_info[0].src_ch;
            audio_forge->rsp_info[0].out_len_bytes = audio_forge->max_sample * sizeof(short) * audio_forge->rsp_info[0].src_ch;
            audio_forge->rsp_info[0].complexity = 5;
            audio_forge->rsp_info[0].sample_bits = 16;
            audio_forge->rsp_info[0].type = -1;
            audio_forge->rsp_handle[0] = esp_resample_create((void *)&audio_forge->rsp_info[0],
                                         &audio_forge->rsp_in[0], &audio_forge->rsp_out[0]);
            if (audio_forge->rsp_handle[0] == NULL) {
                ESP_LOGE(TAG, "audio_forge create the handle for resample failed, (line %d)", __LINE__);
                return ESP_FAIL;
            }
        }
#ifdef DEBUG_AUDIO_FORGE_ISSUE
        in_file[0] = fopen(in_name[0], "rb");
#endif
        if ((audio_forge->sample_rate < SAMPLERATE_MIN) || (audio_forge->sample_rate >= SAMPLERATE_MAX)) {
            ESP_LOGE(TAG, "The samplerates of stream are error. (line %d)", __LINE__);
            return ESP_ERR_INVALID_ARG;
        }

        if ((audio_forge->channel != 1) && (audio_forge->channel != 2)) {
            ESP_LOGE(TAG, "The number of channels should be either 1 or 2. (line %d)", __LINE__);
            return ESP_ERR_INVALID_ARG;
        }

        audio_forge->downmix.source_num = 1;
    }
#ifdef DEBUG_AUDIO_FORGE_ISSUE
    out_file = fopen(out_name, "wb");
#endif
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_ALC) {
        //alc
        audio_forge->volume_handle = alc_volume_setup_open();
        if (audio_forge->volume_handle == NULL) {
            ESP_LOGE(TAG, "audio_forge create the handle for setting volume failed, (line %d)", __LINE__);
            return ESP_FAIL;
        }
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_EQUALIZER) {
        //equalizer
        audio_forge->eq_handle = esp_equalizer_init(audio_forge->channel, audio_forge->sample_rate, NUMBER_BAND, USE_XMMS_ORIGINAL_FREQENT);
        if (audio_forge->eq_handle == NULL) {
            ESP_LOGE(TAG, "audio_forge create the handle for setting equalizer failed, (line %d)", __LINE__);
            return ESP_FAIL;
        }
        for (int i = 0; i < audio_forge->channel; i++) {
            for (int j = 0; j < NUMBER_BAND; j++) {
                esp_equalizer_set_band_value(audio_forge->eq_handle, audio_forge->equalizer_gain[NUMBER_BAND * i + j], j, i);
            }
        }
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC) {
        //sonic
        audio_forge->sonic_handle = esp_sonic_create_stream(audio_forge->sample_rate, audio_forge->channel);
        esp_sonic_set_resample_mode(audio_forge->sonic_handle, 0);
        esp_sonic_set_speed(audio_forge->sonic_handle, audio_forge->sonic_speed);
        esp_sonic_set_pitch(audio_forge->sonic_handle, audio_forge->sonic_pitch);
        if (audio_forge->sonic_handle == NULL) {
            ESP_LOGE(TAG, "audio_forge create the handle for setting sonic failed, (line %d)", __LINE__);
            return ESP_FAIL;
        }
        audio_forge->sonic_num = audio_forge->sample_rate * sizeof(short) * 2 / SONIC_MIN_PITCH;
        audio_forge->sonic_num = audio_forge->sonic_num / (audio_forge->max_sample * sizeof(short)) + 1;
        audio_forge->inbuf[0] = (unsigned char *)audio_calloc(1, audio_forge->sonic_num * audio_forge->max_sample * sizeof(short) * 2);
        audio_forge->outbuf = (unsigned char *)audio_calloc(1, audio_forge->sonic_num * audio_forge->max_sample * sizeof(short) * 2);
    } else {
        audio_forge->inbuf[0] = (unsigned char *)audio_calloc(1, audio_forge->max_sample * sizeof(short) * 2);
        audio_forge->outbuf = (unsigned char *)audio_calloc(1, audio_forge->max_sample * sizeof(short) * 2);
    }
    if (audio_forge->inbuf[0] == NULL) {
        ESP_LOGE(TAG, "Failed to audio_calloc of audio_forge->inbuf[%d]. (line %d)", 0, __LINE__);
        return ESP_ERR_NO_MEM;
    }
    if (audio_forge->outbuf == NULL) {
        ESP_LOGE(TAG, "Failed to audio_calloc of audio_forge->outbuf. (line %d)", __LINE__);
        return ESP_ERR_NO_MEM;
    }
    audio_forge->reflag = 0;
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
    audio_forge_start_time = periph_tick_get();
    audio_forge_stop_time = 0;
    audio_forge_whole_time = 0;
    audio_forge_decode_time = 0;
    audio_forge_write_time = 0;
#endif
    return ESP_OK;
}

static esp_err_t audio_forge_destroy(audio_element_handle_t self)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (audio_forge->inbuf) {
        audio_free(audio_forge->inbuf);
        audio_forge->inbuf = NULL;
    }
    if (audio_forge->rsp_handle) {
        audio_free(audio_forge->rsp_handle);
        audio_forge->rsp_handle = NULL;
    }
    if (audio_forge->rsp_info) {
        audio_free(audio_forge->rsp_info);
        audio_forge->rsp_info = NULL;
    }
    if (audio_forge->rsp_in) {
        audio_free(audio_forge->rsp_in);
        audio_forge->rsp_in = NULL;
    }
    if (audio_forge->rsp_out) {
        audio_free(audio_forge->rsp_out);
        audio_forge->rsp_out = NULL;
    }
    if (audio_forge->in_offset) {
        audio_free(audio_forge->in_offset);
        audio_forge->in_offset = NULL;
    }
    audio_free(audio_forge);
    return ESP_OK;
}

static esp_err_t audio_forge_close(audio_element_handle_t self)
{
    ESP_LOGI(TAG, "Closed");
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
    audio_forge_stop_time = periph_tick_get();
    audio_forge_whole_time = audio_forge_stop_time - audio_forge_start_time;
    int i2stime = (double)audio_forge->max_sample * pcmcnt / audio_forge->sample_rate * 1000.0;
    printf("i2stime = %d : whole_time = %d write_time = %d : decode_time = %d\n", i2stime,
           audio_forge_whole_time, audio_forge_write_time, audio_forge_decode_time);
    if (pcmcnt > 0) {
        double audio_forge_whole_ratio = (double)audio_forge_whole_time / i2stime;
        double audio_forge_write_ratio = (double)audio_forge_write_time / i2stime;
        double audio_forge_decode_ratio = (double)audio_forge_decode_time / i2stime;
        printf("TIMEINFO: samplerate = %d Hz channel = %d whole_ratio = %.4f write_ratio = %.4f decode_ratio = %.4f\n",
               audio_forge->sample_rate, audio_forge->channel, audio_forge_whole_ratio, audio_forge_write_ratio,
               audio_forge_decode_ratio);
    }
#endif
    for (int i = 0; i < audio_forge->downmix.source_num; i++) {
        if (audio_forge->inbuf[i] != NULL) {
            audio_free(audio_forge->inbuf[i]);
            audio_forge->inbuf[i] = NULL;
        }
#ifdef DEBUG_AUDIO_FORGE_ISSUE
        if (in_file[i] != NULL) {
            fclose(in_file[i]);
        }
#endif
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
            //resample
            if (audio_forge->rsp_handle[i] != NULL) {
                esp_resample_destroy(audio_forge->rsp_handle[i]);
            }
        }
    }
#ifdef DEBUG_AUDIO_FORGE_ISSUE
    if (out_file != NULL) {
        fclose(out_file);
    }
#endif
    if (audio_forge->outbuf != NULL) {
        audio_free(audio_forge->outbuf);
        audio_forge->outbuf = NULL;
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX) {
        //downmix
        if (audio_forge->downmix_handle != NULL) {
            esp_downmix_close(audio_forge->downmix_handle);
        }
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_ALC) {
        //alc
        if (audio_forge->volume_handle != NULL) {
            alc_volume_setup_close(audio_forge->volume_handle);
        }
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_EQUALIZER) {
        //equalizer
        if (audio_forge->eq_handle != NULL) {
            esp_equalizer_uninit(audio_forge->eq_handle);
        }
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC) {
        //sonic
        if (audio_forge->sonic_handle != NULL) {
            esp_sonic_destroy_stream(audio_forge->sonic_handle);
        }
    }
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    return ESP_OK;
}

static int audio_forge_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    if (pcmcnt == 0) {
        AUDIO_MEM_SHOW(TAG);
    }
#endif
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
    pcmcnt += audio_forge->sonic_num;
    int starttime = 0;
    int stoptime = 0;
#endif
    int ret = 0;
    if (audio_forge->reflag == 1) {
        ret = audio_forge_close(self);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "close failed, line ( %d )", __LINE__);
            return ESP_FAIL;
        }
        ret = audio_forge_open(self);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "open failed, line ( %d )", __LINE__);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "audio_forge reopen");
    }
    int status = 0;

    int w_size = 0;
    int max_byte = 0;
    int byte = 0;
    int j = 0;
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC) {
        memset(audio_forge->outbuf, 0, audio_forge->max_sample * sizeof(short) * 2 * audio_forge->channel);
    } else {
        memset(audio_forge->outbuf, 0, audio_forge->max_sample * sizeof(short) * 2);
    }

    for (j = 0; j < audio_forge->sonic_num; j++) {
        status = 0;
        for (int i = 0, in_bytes_consumed = 0; i < audio_forge->downmix.source_num; i++) {
            if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
                //resample
                if ((audio_forge->in_offset[i] < audio_forge->rsp_info[i].max_indata_bytes)
                    && (audio_forge->rsp_info[i].max_indata_bytes > 0)) {
                    if (audio_forge->in_offset[i] > 0) {
                        memmove(audio_forge->rsp_in[i],
                                &audio_forge->rsp_in[i][audio_forge->rsp_info[i].max_indata_bytes - audio_forge->in_offset[i]],
                                audio_forge->in_offset[i]);
                    }
#ifndef DEBUG_AUDIO_FORGE_ISSUE
                    ret = audio_element_multi_input(self, (char *)&audio_forge->rsp_in[i][audio_forge->in_offset[i]],
                                                    audio_forge->rsp_info[i].max_indata_bytes - audio_forge->in_offset[i], i,
                                                    audio_forge->tick_to_wait);
#else
                    ret = fread(&audio_forge->rsp_in[i][audio_forge->in_offset[i]], 1,
                                audio_forge->rsp_info[i].max_indata_bytes - audio_forge->in_offset[i], in_file[i]);
#endif
                    if (ret > 0) {
                        audio_forge->in_offset[i] += ret;
                        if (audio_forge->rsp_info[i].max_indata_bytes - audio_forge->in_offset[i]) {
                            max_byte = audio_forge->in_offset[i] / audio_forge->rsp_info[i].src_ch;
                            byte = audio_forge->rsp_info[i].out_len_bytes * audio_forge->rsp_info[i].src_rate /
                                   (audio_forge->rsp_info[i].dest_rate * audio_forge->rsp_info[i].dest_ch) + 128;
                            if (max_byte > byte) {
                                audio_forge->rsp_info[i].max_indata_bytes = audio_forge->in_offset[i];
                            }
                        }
                    } else {
                        max_byte = audio_forge->in_offset[i] / audio_forge->rsp_info[i].src_ch;
                        byte = audio_forge->rsp_info[i].out_len_bytes * audio_forge->rsp_info[i].src_rate /
                               (audio_forge->rsp_info[i].dest_rate * audio_forge->rsp_info[i].dest_ch) + 128;
                        if (max_byte > byte) {
                            audio_forge->rsp_info[i].max_indata_bytes = audio_forge->in_offset[i];
                        }
                    }
                }
                if (audio_forge->in_offset[i] > 0) {
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
                    starttime = periph_tick_get();
#endif
                    in_bytes_consumed = esp_resample_run(audio_forge->rsp_handle[i],
                                                         (void *)&audio_forge->rsp_info[i], audio_forge->rsp_in[i], audio_forge->rsp_out[i],
                                                         audio_forge->in_offset[i],
                                                         &audio_forge->rsp_info[i].out_len_bytes);
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
                    stoptime = periph_tick_get();
                    audio_forge_decode_time += stoptime - starttime;
#endif
                    if (in_bytes_consumed <= 0) {
                        audio_forge->in_offset[i] = 0;
                        audio_forge->rsp_info[i].max_indata_bytes = 0;
                        memset(audio_forge->rsp_out[i], 0, audio_forge->rsp_info[i].out_len_bytes);
                    } else {
                        audio_forge->in_offset[i] -= in_bytes_consumed;
                        memset(audio_forge->inbuf[i], 0, audio_forge->max_sample * sizeof(short) * 2);
                        memcpy(audio_forge->inbuf[i], audio_forge->rsp_out[i], audio_forge->rsp_info[i].out_len_bytes);
                    }
                } else {
                    if (ret <= 0) {
                        memset(audio_forge->inbuf[i], 0, audio_forge->max_sample * sizeof(short) * 2);
                        if (ret != AEL_IO_TIMEOUT) {
                            status++;
                        }
                    }
                }
                ret = audio_forge->rsp_info[i].out_len_bytes;
            } else {
                memset(audio_forge->inbuf[i], 0, audio_forge->max_sample * sizeof(short) * 2);
#ifndef DEBUG_AUDIO_FORGE_ISSUE
                audio_forge->in_offset[i] = audio_element_multi_input(self, (char *)audio_forge->inbuf[i],
                                            audio_forge->max_sample * sizeof(short) * audio_forge->downmix.source_info[i].channel, i,
                                            audio_forge->tick_to_wait);
#else
                audio_forge->in_offset[i] = fread((void *)&audio_forge->inbuf[i][0], 1, audio_forge->max_sample * sizeof(short) * audio_forge->downmix.source_info[i].channel,
                                                  in_file[i]);
#endif
                if (audio_forge->in_offset[i] <= 0) {
                    memset(audio_forge->inbuf[i], 0, audio_forge->max_sample * sizeof(short) * 2);
                    if ((audio_forge->in_offset[i] != AEL_IO_TIMEOUT)) {
                        status++;
                    }
                } else if (audio_forge->in_offset[i] != audio_forge->max_sample * sizeof(
                               short) * audio_forge->downmix.source_info[i].channel) {
                    memset(audio_forge->inbuf[i] + audio_forge->in_offset[i], 0,
                           audio_forge->max_sample * sizeof(short) * 2 - audio_forge->in_offset[i]);
                }
                ret = audio_forge->in_offset[i];
            }
        }
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        starttime = periph_tick_get();
#endif
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX) {
            //downmix
            ret = esp_downmix_process(audio_forge->downmix_handle, audio_forge->inbuf,
                                      &audio_forge->outbuf[j * ret * audio_forge->channel], audio_forge->max_sample,
                                      audio_forge->downmix.mode);
        } else {
            memcpy(&audio_forge->outbuf[j * ret], audio_forge->inbuf[0], ret);
        }
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_ALC) {
            //alc
            alc_volume_setup_process(&audio_forge->outbuf[j * ret], ret, audio_forge->channel,
                                     audio_forge->volume_handle, audio_forge->volume);
        }
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_EQUALIZER) {
            //equalizer
            esp_equalizer_process(audio_forge->eq_handle,
                                  (unsigned char *)&audio_forge->outbuf[j * ret], ret, audio_forge->sample_rate,
                                  audio_forge->channel);
        }
        //finished
        if (status == audio_forge->downmix.source_num
            || (status == 1 && audio_forge->downmix.mode == ESP_DOWNMIX_WORK_MODE_BYPASS)) {
            break;
        }
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        stoptime = periph_tick_get();
        audio_forge_decode_time += stoptime - starttime;
#endif
    }
    int write_ret = 0;
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC) {
        //sonic
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        starttime = periph_tick_get();
#endif
        esp_sonic_write_to_stream(audio_forge->sonic_handle, (short *)audio_forge->outbuf,
                                  audio_forge->sonic_num * audio_forge->max_sample);
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        stoptime = periph_tick_get();
        audio_forge_decode_time += stoptime - starttime;
#endif
        do {
            write_ret = esp_sonic_read_from_stream(audio_forge->sonic_handle,
                                                   (short *)audio_forge->inbuf[0], audio_forge->sonic_num * audio_forge->max_sample);
            if (write_ret > 0) {
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
                starttime = periph_tick_get();
#endif
#ifndef DEBUG_AUDIO_FORGE_ISSUE
                w_size = audio_element_output(self, (char *)audio_forge->inbuf[0],
                                              write_ret * audio_forge->channel * sizeof(short) * j / audio_forge->sonic_num);
#else
                w_size = fwrite(audio_forge->inbuf[0], 1,
                                write_ret * audio_forge->channel * sizeof(short) * j / audio_forge->sonic_num, out_file);
#endif
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
                stoptime = periph_tick_get();
                audio_forge_write_time += stoptime - starttime;
#endif
            }

        } while (write_ret > 0);
    } else {
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        starttime = periph_tick_get();
#endif
#ifndef DEBUG_AUDIO_FORGE_ISSUE
        w_size = audio_element_output(self, (char *)audio_forge->outbuf, ret);
#else
        w_size = fwrite(audio_forge->outbuf, 1, ret, out_file);
#endif
#ifdef AUDIO_FORGE_SPEED_ANALYSIS
        stoptime = periph_tick_get();
        audio_forge_write_time += stoptime - starttime;
#endif
    }
    if (j < audio_forge->sonic_num) {
        return ESP_OK;
    }
#ifdef AUDIO_FORGE_MEMORY_ANALYSIS
    pcmcnt += audio_forge->sonic_num;
    if (pcmcnt == audio_forge->sonic_num) {
        AUDIO_MEM_SHOW(TAG);
        stopmemory_inram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        stopmemory_total = esp_get_free_heap_size();
        int inram_mem = startmemory_inram - stopmemory_inram;
        int total_mem = startmemory_total - stopmemory_total;
        int flag = (total_mem >= inram_mem) ? 1 : 0;
        printf("MEMORYINFO: samplerate = %dHz channel = %d\n", total_mem, inram_mem, flag);
    }
#endif
    return w_size;
}

esp_err_t audio_forge_alc_set_volume(audio_element_handle_t self, int volume)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_ALC) {
        audio_forge->volume = volume;
        return ESP_OK;
    }
    ESP_LOGE(TAG, "The ALC don't be used");
    return ESP_FAIL;
}

esp_err_t audio_forge_alc_get_volume(audio_element_handle_t self, int *volume)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_ALC) {
        *volume = audio_forge->volume;
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, " The ALC has not been used.");
        return ESP_FAIL;
    }
}

esp_err_t audio_forge_downmix_set_gain(audio_element_handle_t self, float *gain, int index)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX)) {
        return ESP_OK;
    }
    if (index > audio_forge->downmix.source_num) {
        ESP_LOGE(TAG, "The index is out of source_num range");
        return ESP_ERR_INVALID_ARG;
    }
    if (gain[0] < GAIN_MIN || gain[0] > GAIN_MAX || gain[1] < GAIN_MIN || gain[1] > GAIN_MAX) {
        ESP_LOGE(TAG, "The gain is out (%d, %d) range", GAIN_MIN, GAIN_MAX);
        return ESP_ERR_INVALID_ARG;
    }
    if ((int)(abs((gain[0] - audio_forge->downmix.source_info[index].gain[0]) * 100)) <= 5 //100 and 5 is to determine if two double numbers are equal.
        && (int)(abs((gain[1] - audio_forge->downmix.source_info[index].gain[0]) * 100)) <= 5) {
        return ESP_OK;
    }
    audio_forge->reflag = 1;
    audio_forge->downmix.source_info[index].gain[0] = gain[0];
    audio_forge->downmix.source_info[index].gain[1] = gain[1];
    return ESP_OK;
}

esp_err_t audio_forge_downmix_set_transit_time(audio_element_handle_t self, int transit_time,
        int index)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX)) {
        return ESP_OK;
    }
    ESP_LOGE(TAG, "The transit_time (%d) must be equal or gather than zero", transit_time);
    if (index > audio_forge->downmix.source_num) {
        ESP_LOGE(TAG, "The index is out of source_num range");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGE(TAG, "The transit_time (%d) must be equal or gather than zero", transit_time);
    if (transit_time < 0) {
        ESP_LOGE(TAG, "The transit_time (%d) must be equal or gather than zero", transit_time);
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGE(TAG, "The transit_time (%d) must be equal or gather than zero", transit_time);
    if (audio_forge->downmix.source_info[index].transit_time == transit_time) {
        return ESP_OK;
    }
    audio_forge->reflag = 1;
    audio_forge->downmix.source_info[index].transit_time = transit_time;
    return ESP_OK;
}

void audio_forge_downmix_set_input_rb_timeout(audio_element_handle_t self, int tick_to_wait)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    audio_forge->tick_to_wait = tick_to_wait;
}

void audio_forge_downmix_set_input_rb(audio_element_handle_t self, ringbuf_handle_t rb, int index)
{
    audio_element_set_multi_input_ringbuf(self, rb, index);
}

esp_err_t audio_forge_eq_set_gain(audio_element_handle_t self, int eq_gain, int band_index)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & AUDIO_FORGE_SELECT_EQUALIZER)) {
        return ESP_OK;
    }
    if ((band_index < 0) || (band_index > NUMBER_BAND)) {
        ESP_LOGE(TAG, "The range of index for audio gain of equalizer should be [0 9]. Here is %d. (line %d)", band_index, __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    if (audio_forge->channel == 2) {
        if ((audio_forge->equalizer_gain[band_index] == eq_gain)
            && (audio_forge->equalizer_gain[NUMBER_BAND * 1 + band_index] == eq_gain)) {
            return ESP_OK;
        }
        audio_forge->reflag = 1;
        audio_forge->equalizer_gain[NUMBER_BAND * 0 + band_index] = eq_gain;
        audio_forge->equalizer_gain[NUMBER_BAND * 1 + band_index] = eq_gain;
        return ESP_OK;
    }
    if (audio_forge->equalizer_gain[band_index] == eq_gain) {
        return ESP_OK;
    }
    audio_forge->reflag = 1;
    audio_forge->equalizer_gain[band_index] = eq_gain;
    return ESP_OK;
}

esp_err_t audio_forge_sonic_set_speed(audio_element_handle_t self, float sonic_speed)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC)) {
        return ESP_OK;
    }
    if ((int)(abs(sonic_speed - audio_forge->sonic_speed) * 100) <= 5) {
        return ESP_OK;
    }
    audio_forge->reflag = 1;
    audio_forge->sonic_speed = sonic_speed;
    return ESP_OK;
}

esp_err_t audio_forge_sonic_set_pitch(audio_element_handle_t self, float sonic_pitch)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & AUDIO_FORGE_SELECT_SONIC)) {
        return ESP_OK;
    }
    if ((int)(abs((sonic_pitch - audio_forge->sonic_pitch) * 100)) <= 5) {
        //100 and 5 is to determine if two double numbers are equal.
        return ESP_OK;
    }
    audio_forge->reflag = 1;
    audio_forge->sonic_pitch = sonic_pitch;
    return ESP_OK;
}

esp_err_t audio_forge_set_src_info(audio_element_handle_t self, int samplerate, int channel,
                                   int index)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    if (!(audio_forge->component_select & (AUDIO_FORGE_SELECT_RESAMPLE | AUDIO_FORGE_SELECT_DOWNMIX))) {
        return ESP_OK;
    }
    if (index > audio_forge->downmix.source_num) {
        ESP_LOGE(TAG, "The index is out of source_num range");
        return ESP_ERR_INVALID_ARG;
    }
    if (samplerate < SAMPLERATE_MIN || samplerate > SAMPLERATE_MAX) {
        ESP_LOGE(TAG, "Samplerate is out of (%d %d) range.", SAMPLERATE_MIN, SAMPLERATE_MAX);
        return ESP_ERR_INVALID_ARG;
    }
    if (channel != 1 && channel != 2) {
        ESP_LOGE(TAG, "The number of channel must be 1 or 2 ");
        return ESP_ERR_INVALID_ARG;
    }
    if ((audio_forge->component_select & AUDIO_FORGE_SELECT_EQUALIZER) && samplerate != 11025
        && samplerate != 22050 && samplerate != 44100 && samplerate != 48000) {
        ESP_LOGE(TAG, "Unsupported sample rate %d. Currently supported rates are 11025 Hz, 22050 Hz, 44100 Hz and 48000 Hz",
                 samplerate);
        return ESP_ERR_INVALID_ARG;
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
        if ((samplerate == audio_forge->rsp_info[index].src_rate)
            && (channel == audio_forge->rsp_info[index].src_ch)) {
            return ESP_OK;
        }
        audio_forge->reflag = 1;
        audio_forge->rsp_info[index].src_rate = samplerate;
        audio_forge->rsp_info[index].src_ch = channel;
        return ESP_OK;
    }
    if (audio_forge->component_select & AUDIO_FORGE_SELECT_DOWNMIX) {
        if ((samplerate == audio_forge->downmix.source_info[index].samplerate)
            && (channel == audio_forge->downmix.source_info[index].channel)) {
            return ESP_OK;
        }
        audio_forge->reflag = 1;
        audio_forge->downmix.source_info[index].samplerate = samplerate;
        audio_forge->downmix.source_info[index].channel = channel;
        return ESP_OK;
    }
    if (!(audio_forge->component_select & (AUDIO_FORGE_SELECT_RESAMPLE | AUDIO_FORGE_SELECT_DOWNMIX))) {
        if (samplerate == audio_forge->downmix.source_info[0].samplerate
            && channel > audio_forge->downmix.source_info[0].channel) {
            return ESP_OK;
        }
        audio_forge->reflag = 1;
        audio_forge->downmix.source_info[0].samplerate = samplerate;
        audio_forge->downmix.source_info[0].channel = channel;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}

//if user only uses `downmix`, source streams has same sample rate.
esp_err_t audio_forge_source_info_init(audio_element_handle_t self, audio_forge_src_info_t *source_num, audio_forge_downmix_t *downmix_info)
{
    audio_forge_t *audio_forge = (audio_forge_t *)audio_element_getdata(self);
    for (int i = 0; i < audio_forge->downmix.source_num; i++) {
        audio_forge->downmix.source_info[i].samplerate = source_num[i].samplerate;
        audio_forge->downmix.source_info[i].channel = source_num[i].channel;
        audio_forge->downmix.source_info[i].bits_num = source_num[i].bit_num;
        audio_forge->downmix.source_info[i].gain[0] = downmix_info[i].gain[0];
        audio_forge->downmix.source_info[i].gain[1] = downmix_info[i].gain[1];
        audio_forge->downmix.source_info[i].transit_time = downmix_info[i].transit_time;
        if (audio_forge->component_select & AUDIO_FORGE_SELECT_RESAMPLE) {
            audio_forge->rsp_info[i].src_rate = audio_forge->downmix.source_info[i].samplerate;
            audio_forge->rsp_info[i].src_ch = audio_forge->downmix.source_info[i].channel;
        }
    }
    return ESP_OK;
}

audio_element_handle_t audio_forge_init(audio_forge_cfg_t *config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "config is NULL. (line %d)", __LINE__);
        return NULL;
    }
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_element_handle_t el;
    cfg.open = audio_forge_open;
    cfg.close = audio_forge_close;
    cfg.process = audio_forge_process;
    cfg.destroy = audio_forge_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    cfg.stack_in_ext = config->stack_in_ext;
    config->audio_forge.source_num = config->audio_forge.source_num >= 1 ? config->audio_forge.source_num : 1;
    cfg.multi_in_rb_num = config->audio_forge.source_num;
    cfg.tag = "audio_forge";
    audio_forge_t *audio_forge = audio_calloc(1, sizeof(audio_forge_t));
    AUDIO_MEM_CHECK(TAG, audio_forge, return NULL);

    audio_forge->sample_rate = config->audio_forge.dest_samplerate;
    audio_forge->channel = config->audio_forge.dest_channel;
    audio_forge->volume = config->audio_forge.alc_volume;
    audio_forge->sonic_pitch = config->audio_forge.sonic_pitch;
    audio_forge->sonic_speed = config->audio_forge.sonic_speed;
    audio_forge->equalizer_gain = config->audio_forge.equalizer_gain;
    audio_forge->component_select = config->audio_forge.component_select;
    audio_forge->max_sample = config->audio_forge.max_sample;
    audio_forge->tick_to_wait = 0;
    audio_forge->downmix.source_num = config->audio_forge.source_num;

    if (audio_forge->downmix.source_num > SOURCE_NUM_MAX) {
        audio_free(audio_forge);
        ESP_LOGE(TAG, "the array size of source number is out of range");
        return NULL;
    }

    audio_forge->inbuf = audio_calloc(audio_forge->downmix.source_num, sizeof(unsigned char *));
    AUDIO_MEM_CHECK(TAG, audio_forge->inbuf, {audio_free(audio_forge); return NULL;});

    audio_forge->rsp_handle = audio_calloc(audio_forge->downmix.source_num, sizeof(void *));
    AUDIO_MEM_CHECK(TAG, audio_forge->rsp_handle, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        return NULL;
    });
    audio_forge->rsp_info = audio_calloc(audio_forge->downmix.source_num, sizeof(resample_info_t));
    AUDIO_MEM_CHECK(TAG, audio_forge->rsp_info, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        audio_free(audio_forge->rsp_handle);
        return NULL;
    });
    audio_forge->rsp_in = audio_calloc(audio_forge->downmix.source_num, sizeof(unsigned char *));
    AUDIO_MEM_CHECK(TAG, audio_forge->rsp_in, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        audio_free(audio_forge->rsp_handle);
        audio_free(audio_forge->rsp_info);
        return NULL;
    });
    audio_forge->rsp_out = audio_calloc(audio_forge->downmix.source_num, sizeof(unsigned char *));
    AUDIO_MEM_CHECK(TAG, audio_forge->rsp_out, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        audio_free(audio_forge->rsp_handle);
        audio_free(audio_forge->rsp_info);
        audio_free(audio_forge->rsp_in);
        return NULL;
    });
    audio_forge->in_offset = audio_calloc(audio_forge->downmix.source_num, sizeof(int));
    AUDIO_MEM_CHECK(TAG, audio_forge->in_offset, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        audio_free(audio_forge->rsp_handle);
        audio_free(audio_forge->rsp_info);
        audio_free(audio_forge->rsp_in);
        audio_free(audio_forge->in_offset);
        return NULL;
    });

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, {
        audio_free(audio_forge);
        audio_free(audio_forge->inbuf);
        audio_free(audio_forge->rsp_handle);
        audio_free(audio_forge->rsp_info);
        audio_free(audio_forge->rsp_in);
        audio_free(audio_forge->in_offset);
        return NULL;
    });
    audio_element_setdata(el, audio_forge);
    return el;
}
