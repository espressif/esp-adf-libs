set(COMPONENT_INCLUDE ".")
set(COMPONENT_SRCDIRS ".")

idf_component_register(SRC_DIRS ${COMPONENT_SRCDIRS}
                       INCLUDE_DIRS ${COMPONENT_INCLUDE}
                       EMBED_TXTFILES "test.mp3" "test.flac"
                       PRIV_REQUIRES esp_audio_codec esp_ringbuf unity esp_timer esp_psram
                       WHOLE_ARCHIVE)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format-overflow)
