set(COMPONENT_INCLUDE ".")
set(COMPONENT_SRCDIRS ".")

idf_component_register(SRC_DIRS ${COMPONENT_SRCDIRS}
                       INCLUDE_DIRS ${COMPONENT_INCLUDE}
                       PRIV_REQUIRES esp_audio_codec esp_muxer unity test_utils esp_timer esp_psram esp_board_manager gen_bmgr_codes
                       WHOLE_ARCHIVE)
