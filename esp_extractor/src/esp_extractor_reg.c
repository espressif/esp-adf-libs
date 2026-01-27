/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Proprietary
 *
 * See LICENSE file for details.
 */

#include <sdkconfig.h>
#include "esp_extractor_defaults.h"

esp_extractor_err_t esp_extractor_register_default(void)
{
    esp_extractor_err_t ret = ESP_EXTRACTOR_ERR_OK;
#ifdef CONFIG_WAV_EXTRACTOR_SUPPORT
    ret |= esp_wav_extractor_register();
#endif  /* CONFIG_WAV_EXTRACTOR_SUPPORT */

#ifdef CONFIG_MP4_EXTRACTOR_SUPPORT
    ret |= esp_mp4_extractor_register();
#endif  /* CONFIG_MP4_EXTRACTOR_SUPPORT */

#ifdef CONFIG_TS_EXTRACTOR_SUPPORT
    ret |= esp_ts_extractor_register();
#endif  /* CONFIG_TS_EXTRACTOR_SUPPORT */

#ifdef CONFIG_OGG_EXTRACTOR_SUPPORT
    ret |= esp_ogg_extractor_register();
#endif  /* CONFIG_OGG_EXTRACTOR_SUPPORT */

#ifdef CONFIG_AVI_EXTRACTOR_SUPPORT
    ret |= esp_avi_extractor_register();
#endif  /* CONFIG_AVI_EXTRACTOR_SUPPORT */

#ifdef CONFIG_AUDIO_ES_EXTRACTOR_SUPPORT
    ret |= esp_audio_es_extractor_register();
#endif  /* CONFIG_AUDIO_ES_EXTRACTOR_SUPPORT */

#ifdef CONFIG_CAF_EXTRACTOR_SUPPORT
    ret |= esp_caf_extractor_register();
#endif  /* CONFIG_CAF_EXTRACTOR_SUPPORT */

#ifdef CONFIG_FLV_EXTRACTOR_SUPPORT
    ret |= esp_flv_extractor_register();
#endif  /* CONFIG_FLV_EXTRACTOR_SUPPORT */
    return ret;
}

void esp_extractor_unregister_default(void)
{
#ifdef CONFIG_WAV_EXTRACTOR_SUPPORT
    esp_wav_extractor_unregister();
#endif  /* CONFIG_WAV_EXTRACTOR_SUPPORT */

#ifdef CONFIG_MP4_EXTRACTOR_SUPPORT
    esp_mp4_extractor_unregister();
#endif  /* CONFIG_MP4_EXTRACTOR_SUPPORT */

#ifdef CONFIG_TS_EXTRACTOR_SUPPORT
    esp_ts_extractor_unregister();
#endif  /* CONFIG_TS_EXTRACTOR_SUPPORT */

#ifdef CONFIG_OGG_EXTRACTOR_SUPPORT
    esp_ogg_extractor_unregister();
#endif  /* CONFIG_OGG_EXTRACTOR_SUPPORT */

#ifdef CONFIG_AVI_EXTRACTOR_SUPPORT
    esp_avi_extractor_unregister();
#endif  /* CONFIG_AVI_EXTRACTOR_SUPPORT */

#ifdef CONFIG_AUDIO_ES_EXTRACTOR_SUPPORT
    esp_audio_es_extractor_unregister();
#endif  /* CONFIG_AUDIO_ES_EXTRACTOR_SUPPORT */

#ifdef CONFIG_CAF_EXTRACTOR_SUPPORT
    esp_caf_extractor_unregister();
#endif  /* CONFIG_CAF_EXTRACTOR_SUPPORT */

#ifdef CONFIG_FLV_EXTRACTOR_SUPPORT
    esp_flv_extractor_unregister();
#endif  /* CONFIG_FLV_EXTRACTOR_SUPPORT */
}
