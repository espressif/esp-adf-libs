set(COMPONENT_SRC "src/audio_decoder_reg.c" "src/audio_encoder_reg.c" "src/simple_decoder_reg.c")
set(COMPONENT_INCLUDE "include" 
    "include/decoder"
    "include/decoder/impl"
    "include/encoder"
    "include/encoder/impl"
    "include/simple_dec"
)

idf_component_register(
    INCLUDE_DIRS ${COMPONENT_INCLUDE}
    SRCS ${COMPONENT_SRC}
)

# v2.6+ enables P4 assembly optimizations that require chip revision >= 3.0.
# Fail at build time (not configure) so that `idf.py menuconfig` can still open
# and allow selecting ESP32P4_REV_MIN >= 3.0.
if(CONFIG_IDF_TARGET_ESP32P4)
    idf_build_get_property(p4_rev CONFIG_ESP32P4_REV_MIN_FULL)
    if(NOT p4_rev OR p4_rev LESS 300)
        string(CONCAT _esp_ac_p4_rev_msg
            "esp_audio_codec (v2.6+) requires ESP32-P4 chip version >= 3.0. "
            "Please use ESP32-P4 chip version >= 3.0, or use audio codec version < v2.6.")
        add_custom_target(esp_audio_codec_p4_rev_check ALL
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow "${_esp_ac_p4_rev_msg}"
            COMMAND ${CMAKE_COMMAND} -E false
            COMMENT "Checking ESP32-P4 chip revision for esp_audio_codec"
            VERBATIM
        )
    endif()
endif()

get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(AUDIO_CODEC_LIB_TARGET "${CONFIG_IDF_TARGET}")
if(CONFIG_IDF_TARGET_ESP32S31 AND NOT CONFIG_ESP_AUDIO_CODEC_S31_USE_ASM)
    set(AUDIO_CODEC_LIB_TARGET "${CONFIG_IDF_TARGET}_esp_c")
endif()

add_prebuilt_library(esp_acodec "${CMAKE_CURRENT_SOURCE_DIR}/lib/${AUDIO_CODEC_LIB_TARGET}/libesp_audio_codec.a"
                     PRIV_REQUIRES ${BASE_DIR})
add_prebuilt_library(esp_sdec "${CMAKE_CURRENT_SOURCE_DIR}/lib/${AUDIO_CODEC_LIB_TARGET}/libesp_audio_simple_dec.a"
                     PRIV_REQUIRES ${BASE_DIR})
set(TARGET_LIB_NAME esp_sdec esp_acodec)

target_link_libraries(${COMPONENT_LIB}  PRIVATE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${AUDIO_CODEC_LIB_TARGET}")
target_link_libraries(${COMPONENT_LIB}  PRIVATE ${TARGET_LIB_NAME})
