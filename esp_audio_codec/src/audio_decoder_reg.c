/**
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "sdkconfig.h"
#include "esp_audio_dec_default.h"

esp_audio_err_t esp_audio_dec_register_default(void)
{
    esp_audio_err_t ret = ESP_AUDIO_ERR_OK;
#ifdef CONFIG_AUDIO_DECODER_MP3_SUPPORT
    ret |= esp_mp3_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_AAC_SUPPORT
    ret |= esp_aac_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_G711_SUPPORT
    ret |= esp_g711a_dec_register();
    ret |= esp_g711u_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_AMRNB_SUPPORT
    ret |= esp_amrnb_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_AMRWB_SUPPORT
    ret |= esp_amrwb_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_FLAC_SUPPORT
    ret |= esp_flac_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_VORBIS_SUPPORT
    ret |= esp_vorbis_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_OPUS_SUPPORT
    ret |= esp_opus_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_ADPCM_SUPPORT
    ret |= esp_adpcm_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_ALAC_SUPPORT
    ret |= esp_alac_dec_register();
#endif

#ifdef CONFIG_AUDIO_DECODER_PCM_SUPPORT
    ret |= esp_pcm_dec_register();
#endif
    return ret;
}

void esp_audio_dec_unregister_default(void)
{
#ifdef CONFIG_AUDIO_DECODER_MP3_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_MP3);
#endif

#ifdef CONFIG_AUDIO_DECODER_AAC_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_AAC);
#endif

#ifdef CONFIG_AUDIO_DECODER_G711_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_G711A);
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_G711U);
#endif

#ifdef CONFIG_AUDIO_DECODER_AMRNB_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_AMRNB);
#endif

#ifdef CONFIG_AUDIO_DECODER_AMRWB_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_AMRWB);
#endif

#ifdef CONFIG_AUDIO_DECODER_FLAC_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_FLAC);
#endif

#ifdef CONFIG_AUDIO_DECODER_VORBIS_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_VORBIS);
#endif

#ifdef CONFIG_AUDIO_DECODER_OPUS_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_OPUS);
#endif

#ifdef CONFIG_AUDIO_DECODER_ADPCM_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_ADPCM);
#endif

#ifdef CONFIG_AUDIO_DECODER_ALAC_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_ALAC);
#endif

#ifdef CONFIG_AUDIO_DECODER_PCM_SUPPORT
    esp_audio_dec_unregister(ESP_AUDIO_TYPE_PCM);
#endif
}
