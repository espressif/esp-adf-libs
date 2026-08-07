idf_component_register(INCLUDE_DIRS "include")

# v1.4+ enables P4 assembly optimizations that require chip revision >= 3.0.
# Fail at build time (not configure) so that `idf.py menuconfig` can still open
# and allow selecting ESP32P4_REV_MIN >= 3.0.
if(CONFIG_IDF_TARGET_ESP32P4)
    idf_build_get_property(p4_rev CONFIG_ESP32P4_REV_MIN_FULL)
    if(NOT p4_rev OR p4_rev LESS 300)
        string(CONCAT _esp_ae_p4_rev_msg
            "esp_audio_effects (v1.4+) requires ESP32-P4 chip version >= 3.0. "
            "Please use ESP32-P4 chip version >= 3.0, or use audio effects version < v1.4.")
        add_custom_target(esp_audio_effects_p4_rev_check ALL
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow "${_esp_ae_p4_rev_msg}"
            COMMAND ${CMAKE_COMMAND} -E false
            COMMENT "Checking ESP32-P4 chip revision for esp_audio_effects"
            VERBATIM
        )
    endif()
endif()

set(AUDIO_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}")
if(CONFIG_IDF_TARGET_ESP32S31 AND NOT CONFIG_ESP_AUDIO_EFFECTS_S31_USE_ASM)
    set(AUDIO_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}_esp_c")
endif()

add_prebuilt_library(esp_audio_effects_prebuilt
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${AUDIO_EFFECTS_LIB_TARGET}/libesp_audio_effects.a"
    PRIV_REQUIRES espressif__gmf_fft
)

target_link_libraries(${COMPONENT_LIB} INTERFACE esp_audio_effects_prebuilt)
