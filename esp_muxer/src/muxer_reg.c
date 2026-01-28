/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "sdkconfig.h"
#include "esp_muxer_default.h"

esp_muxer_err_t esp_muxer_register_default(void)
{
    esp_muxer_err_t ret = ESP_MUXER_ERR_OK;

#ifdef CONFIG_ESP_MUXER_MP4_SUPPORT
    ret = mp4_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_MP4_SUPPORT */

#ifdef CONFIG_ESP_MUXER_TS_SUPPORT
    ret = ts_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_TS_SUPPORT */

#ifdef CONFIG_ESP_MUXER_OGG_SUPPORT
    ret = ogg_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_OGG_SUPPORT */

#ifdef CONFIG_ESP_MUXER_WAV_SUPPORT
    ret = wav_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_WAV_SUPPORT */

#ifdef CONFIG_ESP_MUXER_FLV_SUPPORT
    ret = flv_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_FLV_SUPPORT */

#ifdef CONFIG_ESP_MUXER_CAF_SUPPORT
    ret = caf_muxer_register();
    if (ret != ESP_MUXER_ERR_OK) {
        return ret;
    }
#endif  /* CONFIG_ESP_MUXER_CAF_SUPPORT */

    return ret;
}

void esp_muxer_unregister_default(void)
{
#ifdef CONFIG_ESP_MUXER_MP4_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_MP4);
#endif  /* CONFIG_ESP_MUXER_MP4_SUPPORT */

#ifdef CONFIG_ESP_MUXER_TS_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_TS);
#endif  /* CONFIG_ESP_MUXER_TS_SUPPORT */

#ifdef CONFIG_ESP_MUXER_OGG_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_OGG);
#endif  /* CONFIG_ESP_MUXER_OGG_SUPPORT */

#ifdef CONFIG_ESP_MUXER_WAV_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_WAV);
#endif  /* CONFIG_ESP_MUXER_WAV_SUPPORT */

#ifdef CONFIG_ESP_MUXER_FLV_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_FLV);
#endif  /* CONFIG_ESP_MUXER_FLV_SUPPORT */

#ifdef CONFIG_ESP_MUXER_CAF_SUPPORT
    esp_muxer_unreg(ESP_MUXER_TYPE_CAF);
#endif  /* CONFIG_ESP_MUXER_CAF_SUPPORT */
}
