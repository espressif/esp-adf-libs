// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "esp_log.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "downmix.h"
#include "audio_type_def.h"
static const char *TAG = "DOWNMIX";

#define DM_BUF_SIZE (1024)
// #define DEBUG_DOWNMIX_ISSUE

typedef struct {
    downmix_info_t downmix_info;
    unsigned char *inbuf0;
    unsigned char *inbuf1;
    unsigned char *outbuf;
    void *downmix_handle;
    int at_eof;
    downmix_status_t status;
    int  ticks_to_wait;
} downmix_t;

#ifdef DEBUG_DOWNMIX_ISSUE
static FILE *outone = NULL;
static FILE *outtwo = NULL;
int loocpcm = 0;
#endif

static esp_err_t is_valid_downmix_samplerate(int *samplerate)
{
    if ((samplerate[0] < 0 && samplerate[0] >= 100000)
        || (samplerate[1] < 0 && samplerate[1] >= 100000)
        || samplerate[0] != samplerate[1]) {
        ESP_LOGE(TAG, "The samplerates are error");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t is_valid_downmix_channel(int *channel)
{
    if (channel[0] != 1
        && channel[0] != 2
        && channel[1] != 1
        && channel[1] != 2) {
        ESP_LOGE(TAG, "The number of channels should be either 1 or 2");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t downmix_destroy(audio_element_handle_t self)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    audio_free(downmix);
    return ESP_OK;
}

static esp_err_t downmix_open(audio_element_handle_t self)
{
#ifdef downmix_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    ESP_LOGD(TAG, "downmix_open");
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->inbuf0 = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    downmix->inbuf1 = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    downmix->outbuf = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    if (is_valid_downmix_samplerate(downmix->downmix_info.samplerate) != ESP_OK
        || is_valid_downmix_channel(downmix->downmix_info.channel) != ESP_OK) {
        return ESP_FAIL;
    }
    downmix->status = DOWNMIX_BYPASS;
    esp_downmix_info_t esp_downmix_info;
    esp_downmix_info.bits_num = 16;
    esp_downmix_info.channels[0] = downmix->downmix_info.channel[0];
    esp_downmix_info.channels[1] = downmix->downmix_info.channel[1];
    esp_downmix_info.sample_rate = downmix->downmix_info.samplerate[0];
    esp_downmix_info.downmix_gain[0].transit_ms = downmix->downmix_info.transform_time[0];
    esp_downmix_info.downmix_gain[1].transit_ms = downmix->downmix_info.transform_time[1];
    esp_downmix_info.downmix_gain[0].set_dbgain[0] = (float)downmix->downmix_info.gain[0];
    esp_downmix_info.downmix_gain[0].set_dbgain[1] = (float)downmix->downmix_info.gain[1];
    esp_downmix_info.downmix_gain[1].set_dbgain[0] = (float)downmix->downmix_info.gain[2];
    esp_downmix_info.downmix_gain[1].set_dbgain[1] = (float)downmix->downmix_info.gain[3];
    esp_downmix_info.dual_2_mono_select_ch = downmix->downmix_info.dual_two_mono_select;

    downmix->downmix_handle = esp_downmix_open(&esp_downmix_info);
    if (downmix->downmix_handle == NULL) {
        ESP_LOGE(TAG, "Failed to do downmix initialization");
        return ESP_FAIL;
    }

#ifdef DEBUG_DOWNMIX_ISSUE
    char fileName1[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '1', '.', 'p', 'c', 'm', '\0'};
    outone = fopen(fileName1, "rb");
    if (!outone) {
        perror(fileName1);
        return ESP_FAIL;
    }
    char fileName[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '2', '.', 'p', 'c', 'm', '\0'};
    outtwo = fopen(fileName, "rb");
    if (!outtwo) {
        perror(fileName);
        return ESP_FAIL;
    }
#endif
    return ESP_OK;
}

static esp_err_t downmix_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "downmix_close");
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (downmix->downmix_handle != NULL) {
        esp_downmix_close(downmix->downmix_handle);
    }
    if (downmix->inbuf0 != NULL) {
        audio_free(downmix->inbuf0);
    }
    if (downmix->inbuf1 != NULL) {
        audio_free(downmix->inbuf1);
    }
    if (downmix->outbuf != NULL) {
        audio_free(downmix->outbuf);
    }
#ifdef downmix_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
#ifdef DEBUG_DOWNMIX_ISSUE
    if (outone != NULL) {
        fclose(outone);
    }
    if (outtwo != NULL) {
        fclose(outtwo);
    }

#endif

    return ESP_OK;
}

static int downmix_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
#ifdef DEBUG_DOWNMIX_ISSUE
    loocpcm++;
    if (loocpcm == 500) {
        set_downmix_status(self, 1);
    }
    if (loocpcm == 10000) {
        set_downmix_status(self, 2);
    }
#endif
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (is_valid_downmix_samplerate(downmix->downmix_info.samplerate) != ESP_OK
        || is_valid_downmix_channel(downmix->downmix_info.channel) != ESP_OK) {
        return ESP_FAIL;
    }
    int r_size = DM_BUF_SIZE / 2;
    int ret = 0;
    int bytes_one = 0;
    int bytes_two = 0;
    if (downmix->at_eof == 1) {
        ESP_LOGI(TAG, "downmix finished");
        return ESP_OK;
    }
    if (downmix->at_eof == 0) {
#ifdef DEBUG_DOWNMIX_ISSUE
        bytes_one = fread((char *)downmix->inbuf0, 1, r_size * downmix->downmix_info.channel[0], outone);
        bytes_two = fread((char *)downmix->inbuf1, 1, r_size * downmix->downmix_info.channel[1], outtwo);
#else
        bytes_one = audio_element_input(self, (char *)downmix->inbuf0,
                                        r_size * downmix->downmix_info.channel[0]);
        bytes_two = audio_element_multi_input(self, (char *)downmix->inbuf1,
                                              r_size * downmix->downmix_info.channel[1], 0, downmix->ticks_to_wait);
        ESP_LOGD("TAG", "two:%d, one:%d", bytes_two, bytes_one);
#endif
        // if buf0 done, until buf1 done or fail, downmix can be exit.
        if (bytes_one < 0 ) {
            if (bytes_two > 0) {
                memset(downmix->inbuf0, 0, bytes_one);
            } else {
                downmix->at_eof = 1;
            }
        }
        // If buf1 done, change the downmix status to `DOWNMIX_SWITCH_OFF`
        // And set timeout of buf1 ringbuffer is `0`
        if (bytes_two != r_size * downmix->downmix_info.channel[1]) {
            if ((bytes_two == AEL_IO_TIMEOUT)
                && (0 == downmix->ticks_to_wait)) {
                memset(downmix->inbuf1, 0, DM_BUF_SIZE);
            } else {
                downmix_set_status(self, DOWNMIX_SWITCH_OFF);
                downmix_set_second_input_rb_timeout(self, 0);
            }
        }
    }
    if (bytes_one > 0 || bytes_two > 0) {
        ret = esp_downmix_process(downmix->downmix_handle, downmix->inbuf0, DM_BUF_SIZE,
                                  downmix->inbuf1, DM_BUF_SIZE, downmix->outbuf, downmix->status);
        if (ret < 0) {
            downmix_close(self);
            downmix_open(self);
            ESP_LOGW(TAG, "Reopen downmix");
            return ESP_CODEC_ERR_CONTINUE;
        }
        ret = audio_element_output(self, (char *)downmix->outbuf, ret);
    } else {
        ret = bytes_one > bytes_two ? bytes_two : bytes_one;
    }
    return ret;
}

void downmix_set_second_input_rb_timeout(audio_element_handle_t self, int ticks_to_wait)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->ticks_to_wait = ticks_to_wait;
}

void downmix_set_second_input_rb(audio_element_handle_t self, ringbuf_handle_t rb)
{
    audio_element_set_multi_input_ringbuf(self, rb, 0);
}

void downmix_set_status(audio_element_handle_t self, downmix_status_t status_value)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->status = status_value;
}

esp_err_t downmix_set_info(audio_element_handle_t self, int rate0, int ch0, int rate1, int ch1)
{
    int samplerate[2] = {rate0, rate1};
    int channel[2] = {ch0, ch1};
    if (is_valid_downmix_samplerate(samplerate) != ESP_OK
        || is_valid_downmix_channel(channel) != ESP_OK) {
        return ESP_FAIL;
    }

    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->downmix_info.samplerate[0] = rate0;
    downmix->downmix_info.samplerate[1] = rate1;
    downmix->downmix_info.channel[0] = ch0;
    downmix->downmix_info.channel[1] = ch1;

    return ESP_OK;
}

audio_element_handle_t downmix_init(downmix_cfg_t *config)
{
    downmix_t *downmix = audio_calloc(1, sizeof(downmix_t));
    mem_assert(downmix);
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = downmix_destroy;
    cfg.process = downmix_process;
    cfg.open = downmix_open;
    cfg.close = downmix_close;
    cfg.buffer_len = 0;
    cfg.tag = "downmix";
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_rb_size = config->out_rb_size;
    cfg.enable_multi_io = true;
    audio_element_handle_t el = audio_element_init(&cfg);
    mem_assert(el);
    memcpy(downmix, config, sizeof(downmix_info_t));
    audio_element_setdata(el, downmix);
    downmix->ticks_to_wait = 0;
    ESP_LOGD(TAG, "downmix_init");
    return el;
}
