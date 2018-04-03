// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "audio_element.h"
#include "mp3_decoder.h"
#include "pvmp3decoder_api.h"
#include "mp3reader.h"

static const char *TAG = "MP3_DECODER";

#ifdef FORCE_DEC_MONO_CHANNEL
#define STAGEFRIGHT_MP3_SAMPLE_SIZE (2304)
#else
#define STAGEFRIGHT_MP3_SAMPLE_SIZE (4608)
#endif

#define STAGEFRIGHT_MP3_BUFFER_SZ (2106)

typedef struct mp3_decoder {
    bool is_opened;
    int id3_checked;
    mp3_buffer mp3buf;
    mp3_callbacks *mp3data;
    uint32_t mFixedHeader;

    int consumed_bytes;
    void *decoderBuf;
    uint8_t *inputBuf;
    int16_t *outputBuf;

    tPVMP3DecoderExternal config;
    int framecnt;
    int pcmcnt;
} mp3_decoder_t;

static int mp3_buffer_advance(mp3_buffer *b, int bytes)
{
    b->consumed_bytes = bytes;
    b->data_num -= bytes;
    if (b->data_num < 0) {
        b->data_num = 0;
    }
    return 0;
}

static int mp3_fill_buffer(void *mp3_stream, mp3_buffer *b)
{
    int bread = 0;
    audio_element_handle_t el = (audio_element_handle_t)mp3_stream;
    if (b->consumed_bytes > 0) {
        if (b->data_num) {
            memmove((void *)b->buffer, (void *)(b->buffer + b->consumed_bytes),
                    b->data_num * sizeof(unsigned char));
        }
        if (!b->at_eof) {
            int wanted_sz = b->consumed_bytes;
            while (wanted_sz) {
                bread = audio_element_input(el, (char *)(b->buffer + b->data_num), wanted_sz);
                ESP_LOGD(TAG, "consumed_bytes:%d, out_size:%d,byteread:%d", b->consumed_bytes, bread, b->byteread  );
                if (bread < 0) {
                    bread = 0;
                    b->at_eof = 1;
                    break;
                }
                b->data_num += bread;
                b->byteread += bread;
                wanted_sz -= bread;
            }
        }
        b->consumed_bytes = 0;
    }
    return bread;
}

static void mp3_buf_init(mp3_buffer *b, uint8_t *buffer)
{
    memset(b, 0, sizeof(mp3_buffer));
    memset(buffer, 0, STAGEFRIGHT_MP3_BUFFER_SZ);
    b->buffer = buffer;
    b->at_eof = 0;
    b->byteread = 0;
    b->data_num = 0;
    b->buffer_size = STAGEFRIGHT_MP3_BUFFER_SZ;
    b->consumed_bytes = STAGEFRIGHT_MP3_BUFFER_SZ;
}

static int stagefright_mp3_open(audio_element_handle_t mp3_el)
{
    ESP_LOGI(TAG, "MP3 opened");
    mp3_decoder_t *codec = (mp3_decoder_t *)audio_element_getdata(mp3_el);
    configASSERT(codec);
    // Initialize the config.
    codec->config.equalizerType = 0;
    codec->config.crcEnabled = 0;
    codec->mp3data = audio_calloc(1, sizeof(mp3_callbacks));
    codec->mp3data->fill = mp3_fill_buffer;
    codec->mp3data->advance = mp3_buffer_advance;
    codec->mp3data->user_data = (void *)mp3_el;

    // Allocate the decoder memory.
    uint32_t memRequirements = pvmp3_decoderMemRequirements();
    codec->decoderBuf = audio_calloc(1, memRequirements);
    if (codec->decoderBuf == NULL) {
        return ESP_FAIL;
    }

    // Initialize the decoder.
    pvmp3_InitDecoder(&codec->config, codec->decoderBuf);
    codec->inputBuf = (uint8_t *)audio_calloc(1, STAGEFRIGHT_MP3_BUFFER_SZ);
    if (codec->inputBuf == NULL) {
        return ESP_FAIL;
    }
    mp3_buf_init(&codec->mp3buf, codec->inputBuf);

    // Allocate output buffer.
    codec->outputBuf = (int16_t *)audio_calloc(1, STAGEFRIGHT_MP3_SAMPLE_SIZE);
    if (codec->outputBuf == NULL) {
        return ESP_FAIL;
    }

    audio_element_info_t mp3_info = {0};
    audio_element_getinfo(mp3_el, &mp3_info);
    mp3_info.sample_rates = 0;
    mp3_info.channels = 0;
    mp3_info.byte_pos = 0;
    audio_element_setinfo(mp3_el, &mp3_info);

    codec->framecnt = 0;
    codec->pcmcnt = 0;

    codec->id3_checked = 0;
    codec->mFixedHeader = 0;
    codec->consumed_bytes = 0;

    int success;
    success = Mp3ReadInit(&codec->id3_checked, codec->mp3data, &codec->mp3buf,  &codec->mFixedHeader);
    if (!success) {
        fprintf(stderr, "Encountered error reading when MP3 init\n");
        return ESP_FAIL;
    }
    AUDIO_MEM_SHOW(TAG);
    return ESP_OK;
}


int stagefright_mp3_close(audio_element_handle_t mp3_el)
{
    mp3_decoder_t *codec = (mp3_decoder_t *)audio_element_getdata(mp3_el);
    ESP_LOGI(TAG, "Closed");
    if (codec == NULL) {
        return ESP_OK;
    }
    if (codec->mp3data) {
        free(codec->mp3data);
        codec->mp3data = NULL;
    }
    if (codec->inputBuf) {
        free(codec->inputBuf);
        codec->inputBuf = NULL;
    }
    if (codec->outputBuf) {
        free(codec->outputBuf);
        codec->outputBuf = NULL;
    }
    if (codec->decoderBuf) {
        free(codec->decoderBuf);
        codec->decoderBuf = NULL;
    }
    if (AEL_STATE_PAUSED != audio_element_get_state(mp3_el)) {
        memset(codec, 0, sizeof(mp3_decoder_t));
        audio_element_info_t mp3_info = {0};
        audio_element_setinfo(mp3_el, &mp3_info);
    }

    return ESP_OK;
}

static esp_err_t _mp3_destroy(audio_element_handle_t self)
{
    mp3_decoder_t *mp3 = (mp3_decoder_t *)audio_element_getdata(self);
    audio_free(mp3);
    return ESP_OK;
}

static esp_err_t _mp3_open(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "_mp3_open");
    mp3_decoder_t *mp3 = (mp3_decoder_t *)audio_element_getdata(self);
    mem_assert(mp3);
    if (ESP_OK == stagefright_mp3_open(self)) {
        mp3->is_opened = true;
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t _mp3_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "_mp3_close");
    stagefright_mp3_close(self);

    return ESP_OK;
}

static int _mp3_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    mp3_decoder_t *codec = (mp3_decoder_t *)audio_element_getdata(self);
    uint32_t frame_size = 0;
    int success = 0;
    success = Mp3GetFrame(codec->mp3data, &codec->mp3buf, &frame_size, codec->mFixedHeader, &codec->id3_checked, codec->consumed_bytes);

    if (!success) {
        ESP_LOGI(TAG, "MP3 decodig done");
        return AEL_IO_DONE;
    }
    audio_element_info_t mp3_info = {0};
    audio_element_getinfo(self, &mp3_info);
    mp3_info.byte_pos += frame_size;
    audio_element_setinfo(self, &mp3_info);

    codec->config.inputBufferCurrentLength = frame_size;
    codec->config.inputBufferMaxLength = 0;
    codec->config.inputBufferUsedLength = 0;
    codec->config.pInputBuffer = codec->inputBuf;
    codec->config.pOutputBuffer = codec->outputBuf;
    codec->config.outputFrameSize = STAGEFRIGHT_MP3_SAMPLE_SIZE / sizeof(int16_t);

    ERROR_CODE decoderErr;
    decoderErr = pvmp3_framedecoder(&codec->config, codec->decoderBuf);

    codec->consumed_bytes = codec->config.inputBufferUsedLength;
    if (decoderErr != NO_DECODING_ERROR) {
        ESP_LOGE(TAG, "Decoder encountered error: %d", decoderErr);
        if ((decoderErr == UNSUPPORTED_LAYER)
            || (decoderErr == UNSUPPORTED_FREE_BITRATE)
            || (decoderErr == FILE_OPEN_ERROR)
            || (decoderErr == READ_FILE_ERROR)
            || (decoderErr == MEMORY_ALLOCATION_ERROR)
            || (decoderErr == OUTPUT_BUFFER_TOO_SMALL)) {
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    //ESP_LOGI(TAG, "sample_rate=%d : %d  => _channels = %d : %d",codec->_sample_rate, codec->config.samplingRate, codec->_channels, codec->config.num_channels);
    if (mp3_info.sample_rates  != codec->config.samplingRate || mp3_info.channels != codec->config.num_channels) {
        ESP_LOGI(TAG, "I2S setup: sample_rate = %d : channels = %d", codec->config.samplingRate, codec->config.num_channels);
        audio_element_info_t mp3_info = {0};
        audio_element_getinfo(self, &mp3_info);
        mp3_info.sample_rates = codec->config.samplingRate;
        mp3_info.channels = codec->config.num_channels;
        mp3_info.bits = 16;
        // printf("sample_rates = %d : channels = %d\n", (int)mp3_info.sample_rates, (int)mp3_info.channels);
        audio_element_setinfo(self, &mp3_info);
        audio_element_report_info(self);
    }
    codec->pcmcnt += codec->config.outputFrameSize / codec->config.num_channels;
    codec->framecnt++;
    int w_size = audio_element_output(self, (char *)codec->outputBuf, codec->config.outputFrameSize * 2);
    return w_size;
}

audio_element_handle_t mp3_decoder_init(mp3_decoder_cfg_t *config)
{
    mp3_decoder_t *mp3 = audio_calloc(1, sizeof(mp3_decoder_t));
    mem_assert(mp3);
    mp3->is_opened = false;
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.destroy = _mp3_destroy;
    cfg.process = _mp3_process;
    cfg.open = _mp3_open;
    cfg.close = _mp3_close;
    cfg.task_stack = config->task_stack;
    if (cfg.task_stack == 0) {
        cfg.task_stack = MP3_DECODER_TASK_STACK;
    }
    cfg.tag = "mp3";

    audio_element_handle_t el = audio_element_init(&cfg);
    mem_assert(el);
    audio_element_setdata(el, mp3);
    ESP_LOGD(TAG, "mp3_decoder_init");
    return el;
}
