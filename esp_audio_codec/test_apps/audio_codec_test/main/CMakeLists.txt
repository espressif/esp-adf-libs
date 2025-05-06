set(COMPONENT_INCLUDE ".")
set(COMPONENT_SRCDIRS ".")

idf_component_register(SRC_DIRS ${COMPONENT_SRCDIRS}
                       INCLUDE_DIRS ${COMPONENT_INCLUDE}
                       EMBED_TXTFILES "test.mp3" "test.flac"
                       PRIV_REQUIRES esp_audio_codec esp_ringbuf unity test_utils esp_timer esp_psram sdmmc fatfs
                       WHOLE_ARCHIVE)
