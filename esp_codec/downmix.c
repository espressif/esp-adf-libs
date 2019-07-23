// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "esp_log.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "esp_downmix.h"
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
    int ticks_to_wait;
    int reset_flag;
} downmix_t;

#ifdef DEBUG_DOWNMIX_ISSUE
static FILE *inputone = NULL;
static FILE *inputtwo = NULL;
static FILE *output = NULL;
char *filein0 = NULL;
char *filein1 = NULL;
char *fileOut = NULL;
int loocpcm = 0;
#endif

static esp_err_t is_valid_downmix_samplerate(int *samplerate)
{
    if ((samplerate[0] < SAMPLERATE_MIN && samplerate[0] >= SAMPLERATE_MAX)
        || (samplerate[1] < SAMPLERATE_MIN && samplerate[1] >= SAMPLERATE_MAX)
        || samplerate[0] != samplerate[1]) {
        ESP_LOGE(TAG, "The samplerates of stream are error. (line %d)", __LINE__);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t is_valid_downmix_channel(int *channel)
{
    if ((channel[0] != 1 && channel[0] != 2) || (channel[1] != 1 && channel[1] != 2)) {
        ESP_LOGE(TAG, "The number of channels should be either 1 or 2. (line %d)", __LINE__);
        return ESP_ERR_INVALID_ARG;
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
#ifdef DOWNMIX_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
    ESP_LOGD(TAG, "downmix_open");
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);

    downmix->inbuf0 = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    if (downmix->inbuf0 == NULL) {
        ESP_LOGE(TAG, "Failed to audio_calloc of downmix->inbuf0. (line %d)", __LINE__);
        return ESP_ERR_NO_MEM;
    }
    downmix->inbuf1 = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    if (downmix->inbuf1 == NULL) {
        ESP_LOGE(TAG, "Failed to audio_calloc of downmix->inbuf1. (line %d)", __LINE__);
        return ESP_ERR_NO_MEM;
    }
    downmix->outbuf = (unsigned char *)audio_calloc(1, DM_BUF_SIZE);
    if (downmix->outbuf == NULL) {
        ESP_LOGE(TAG, "Failed to audio_calloc of downmix->outbuf. (line %d)", __LINE__);
        return ESP_ERR_NO_MEM;
    }

    downmix->reset_flag = 0;
    if (is_valid_downmix_samplerate(downmix->downmix_info.samplerate) != ESP_OK
        || is_valid_downmix_channel(downmix->downmix_info.channel) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((downmix->downmix_info.gain[0] < GAIN_MIN || downmix->downmix_info.gain[0] > GAIN_MAX)
        || (downmix->downmix_info.gain[1] < GAIN_MIN || downmix->downmix_info.gain[1] > GAIN_MAX)
        || (downmix->downmix_info.gain[2] < GAIN_MIN || downmix->downmix_info.gain[2] > GAIN_MAX)
        || (downmix->downmix_info.gain[3] < GAIN_MIN || downmix->downmix_info.gain[3] > GAIN_MAX)) {
        ESP_LOGE(TAG, "The gain is out (%d, %d) range", GAIN_MIN, GAIN_MAX);
        return ESP_ERR_INVALID_ARG;
    }
    if ((downmix->downmix_info.transform_time[0] < 0 || downmix->downmix_info.transform_time[1] < 0)) {
        ESP_LOGE(TAG, "The transform_time must be equal or gather than zero (%d, %d)",
                 downmix->downmix_info.transform_time[0], downmix->downmix_info.transform_time[1]);
        return ESP_ERR_INVALID_ARG;
    }
    if (downmix->downmix_info.play_status >= DOWNMIX_PLAY_STATUS_MAX) {
        ESP_LOGE(TAG, "The downmix play status is more than DOWNMIX_PLAY_STATUS_MAX, %d",
                 downmix->downmix_info.play_status);
        return ESP_ERR_INVALID_ARG;
    }
    if (downmix->downmix_info.output_status >= ESP_DOWNMIX_TYPE_OUTPUT_MAX) {
        ESP_LOGE(TAG, "The downmix play status is more than ESP_DOWNMIX_TYPE_OUTPUT_MAX, %d",
                 downmix->downmix_info.output_status);
        return ESP_ERR_INVALID_ARG;
    }
    if (downmix->downmix_info.dual_two_mono_select >= SELECT_CHANNEL_MAX) {
        ESP_LOGE(TAG, "The downmix dual_two_mono_select is more than SELECT_CHANNEL_MAX, %d",
                 downmix->downmix_info.dual_two_mono_select);
        return ESP_ERR_INVALID_ARG;
    }
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
    esp_downmix_info.output_status = downmix->downmix_info.output_status;

    downmix->downmix_handle = esp_downmix_open(&esp_downmix_info);
    if (downmix->downmix_handle == NULL) {
        ESP_LOGE(TAG, "Failed to do downmix initialization, (line %d)", __LINE__);
        return ESP_FAIL;
    }

#ifdef DEBUG_DOWNMIX_ISSUE
    loocpcm = 0;
    //char fNameIn0[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '1', '.', 'p', 'c', 'm', '\0'};
    inputone = fopen(filein0, "rb");
    if (!inputone) {
        perror(filein0);
        return ESP_FAIL;
    }
    if (filein1 != NULL) {
        //char fNameIn1[100] = {'//', 's', 'd', 'c', 'a', 'r', 'd', '//', 't', 'e', 's', 't', '2', '.', 'p', 'c', 'm', '\0'};
        inputtwo = fopen(filein1, "rb");
        if (!inputtwo) {
            perror(filein1);
            return ESP_FAIL;
        }
    }

    output = fopen(fileOut, "wb");
    if (!output) {
        perror(fileOut);
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
#ifdef DOWNMIX_MEMORY_ANALYSIS
    AUDIO_MEM_SHOW(TAG);
#endif
#ifdef DEBUG_DOWNMIX_ISSUE
    if (inputone != NULL) {
        fclose(inputone);
    }
    if (inputtwo != NULL) {
        fclose(inputtwo);
    }
    if (output != NULL) {
        fclose(output);
    }
#endif

    return ESP_OK;
}

static int downmix_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (downmix == NULL) {
        ESP_LOGE(TAG, "downmix is NULL(line %d)", __LINE__);
        return ESP_FAIL;
    }
#ifdef DEBUG_DOWNMIX_ISSUE
    loocpcm++;
    if (loocpcm == 5) {
        downmix->downmix_info.play_status = DOWNMIX_SWITCH_ON;
    }
    if (loocpcm == 1000) {
        downmix->downmix_info.play_status = DOWNMIX_SWITCH_OFF;
    }
#endif
    int r_size = DM_BUF_SIZE / 2;
    int ret = 0;
    int bytes_one = 0;
    int bytes_two = 0;
    if (downmix->reset_flag == 1) {
        ret = downmix_close(self);
        if (ret < 0) {
            return ret;
        }
        ret = downmix_open(self);
        if (ret < 0) {
            return ret;
        }
        ESP_LOGW(TAG, "Reopen downmix");
        return ESP_CODEC_ERR_CONTINUE;
    }
#ifdef DEBUG_DOWNMIX_ISSUE
    bytes_one = fread((char *)downmix->inbuf0, 1, r_size * downmix->downmix_info.channel[0], inputone);
    if (inputtwo != NULL) {
        bytes_two = fread((char *)downmix->inbuf1, 1, r_size * downmix->downmix_info.channel[1], inputtwo);
    }
#else
    bytes_one = audio_element_input(self, (char *)downmix->inbuf0, r_size * downmix->downmix_info.channel[0]);
    bytes_two = audio_element_multi_input(self, (char *)downmix->inbuf1, r_size * downmix->downmix_info.channel[1], 0, downmix->ticks_to_wait);

    ESP_LOGD("TAG", "two:%d, one:%d", bytes_two, bytes_one);
#endif
    // if buf0 done, until buf1 done or fail, downmix can be exit.
    if (bytes_one < 0) {
        if (bytes_two > 0) {
            memset(downmix->inbuf0, 0, DM_BUF_SIZE);
        } else {
            ESP_LOGI(TAG, "downmix finished");
            return ESP_OK;
        }
    }
    // If buf1 done, change the downmix status to `DOWNMIX_SWITCH_OFF`
    // And set timeout of buf1 ringbuffer is `0`
    if (bytes_two != r_size * downmix->downmix_info.channel[1]) {
        if ((bytes_two == AEL_IO_TIMEOUT) && (0 == downmix->ticks_to_wait)) {
            memset(downmix->inbuf1, 0, DM_BUF_SIZE);
        } else {
            downmix->downmix_info.play_status = DOWNMIX_SWITCH_OFF;
            downmix_set_second_input_rb_timeout(self, 0);
        }
    }
    if (bytes_one > 0 || bytes_two > 0) {
        ret = esp_downmix_process(downmix->downmix_handle, downmix->inbuf0, r_size,
                                  downmix->inbuf1, r_size, downmix->outbuf, downmix->downmix_info.play_status);
#ifdef DEBUG_DOWNMIX_ISSUE
        ret = fwrite((char *)downmix->outbuf, 1, ret, output);
        if (loocpcm == 10) {
            printf("inputpcm0:%d byte  inputpcm1: %d byte outputpcm: %d byte\n",
                   r_size * downmix->downmix_info.channel[0], r_size * downmix->downmix_info.channel[1], ret);
        }
#else
        ret = audio_element_output(self, (char *)downmix->outbuf, ret);
#endif
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

esp_err_t downmix_set_play_status(audio_element_handle_t self, downmix_play_status_t status_value)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (downmix->downmix_info.play_status == status_value) {
        return ESP_OK;
    }
    if (status_value >= DOWNMIX_PLAY_STATUS_MAX) {
        ESP_LOGE(TAG, "The set downmix play status is more than DOWNMIX_PLAY_STATUS_MAX, %d", status_value);
        return ESP_ERR_INVALID_ARG;
    }
    downmix->downmix_info.play_status = status_value;
    downmix->reset_flag = 1;
    return ESP_OK;
}

esp_err_t downmix_set_output_status(audio_element_handle_t self, downmix_source_info_t status_value)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (downmix->downmix_info.output_status == status_value) {
        return ESP_OK;
    }
    if (status_value >= ESP_DOWNMIX_TYPE_OUTPUT_MAX) {
        ESP_LOGE(TAG, "The set downmix output status is more than ESP_DOWNMIX_TYPE_OUTPUT_MAX, %d",
                 status_value);
        return ESP_ERR_INVALID_ARG;
    }
    downmix->downmix_info.output_status = status_value;
    downmix->reset_flag = 1;
    return ESP_OK;
}

esp_err_t downmix_set_dual_two_mono_select_info(audio_element_handle_t self,
        int dual_two_mono_select)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if (downmix->downmix_info.dual_two_mono_select == dual_two_mono_select) {
        return ESP_OK;
    }
    if (dual_two_mono_select >= SELECT_CHANNEL_MAX) {
        ESP_LOGE(TAG, "The set downmix dual_two_mono_select is more than SELECT_CHANNEL_MAX, %d",
                 dual_two_mono_select);
        return ESP_ERR_INVALID_ARG;
    }
    downmix->downmix_info.dual_two_mono_select = dual_two_mono_select;
    downmix->reset_flag = 1;
    return ESP_OK;
}

esp_err_t downmix_set_base_file_info(audio_element_handle_t self, int rate, int ch)
{
    int samplerate[2] = {rate, 44100};
    int channel[2] = {ch, 1};
    if (is_valid_downmix_samplerate(samplerate) != ESP_OK
        || is_valid_downmix_channel(channel) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->reset_flag = 1;
    downmix->downmix_info.samplerate[0] = rate;
    downmix->downmix_info.channel[0] = ch;
    return ESP_OK;
}

esp_err_t downmix_set_newcome_file_info(audio_element_handle_t self, int rate, int ch)
{
    int samplerate[2] = {44100, rate};
    int channel[2] = {1, ch};
    if (is_valid_downmix_samplerate(samplerate) != ESP_OK
        || is_valid_downmix_channel(channel) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    downmix->reset_flag = 1;
    downmix->downmix_info.samplerate[1] = rate;
    downmix->downmix_info.channel[1] = ch;
    return ESP_OK;
}

esp_err_t downmix_set_gain_info(audio_element_handle_t self, float *gain)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if ((downmix->downmix_info.gain[0] - gain[0] < 0.01)
        && (downmix->downmix_info.gain[0] - gain[0] > 0.01)
        && (downmix->downmix_info.gain[1] - gain[1] < 0.01)
        && (downmix->downmix_info.gain[1] - gain[1] > 0.01)
        && (downmix->downmix_info.gain[2] - gain[2] < 0.01)
        && (downmix->downmix_info.gain[2] - gain[2] > 0.01)
        && (downmix->downmix_info.gain[3] - gain[3] < 0.01)
        && (downmix->downmix_info.gain[3] - gain[3] > 0.01)) {
        return ESP_OK;
    }
    if ((gain[0] < GAIN_MIN || gain[0] > GAIN_MAX) || (gain[1] < GAIN_MIN || gain[1] > GAIN_MAX)
        || (gain[2] < GAIN_MIN || gain[2] > GAIN_MAX) || (gain[3] < GAIN_MIN || gain[3] > GAIN_MAX)) {
        ESP_LOGE(TAG, "The set gain set is more than gain range (%d, %d)", GAIN_MIN, GAIN_MAX);
        return ESP_ERR_INVALID_ARG;
    }
    downmix->reset_flag = 1;
    downmix->downmix_info.gain[0] = gain[0];
    downmix->downmix_info.gain[1] = gain[1];
    downmix->downmix_info.gain[2] = gain[2];
    downmix->downmix_info.gain[3] = gain[3];
    return ESP_OK;
}

esp_err_t downmix_set_transform_time_info(audio_element_handle_t self, int *transform_time)
{
    downmix_t *downmix = (downmix_t *)audio_element_getdata(self);
    if ((downmix->downmix_info.transform_time[0] == transform_time[0])
        && (downmix->downmix_info.transform_time[1] == transform_time[1])) {
        return ESP_OK;
    }

    if ((transform_time[0] < 0 || transform_time[1] < 0)) {
        ESP_LOGE(TAG, "The set transform_time must be equal or gather than zero (%d, %d)", transform_time[0],
                 transform_time[1]);
        return ESP_FAIL;
    }
    downmix->reset_flag = 1;
    downmix->downmix_info.transform_time[0] = transform_time[0];
    downmix->downmix_info.transform_time[1] = transform_time[1];
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
    downmix->reset_flag = 0;
    audio_element_setdata(el, downmix);
    downmix->ticks_to_wait = 0;
    ESP_LOGD(TAG, "downmix_init");
    return el;
}
