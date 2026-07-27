idf_component_register(INCLUDE_DIRS "include")

set(AUDIO_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}")
if(CONFIG_IDF_TARGET_ESP32S31 AND NOT CONFIG_ESP_AUDIO_EFFECTS_S31_USE_ASM)
    set(AUDIO_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}_esp_c")
endif()

add_prebuilt_library(esp_audio_effects_prebuilt
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${AUDIO_EFFECTS_LIB_TARGET}/libesp_audio_effects.a"
    PRIV_REQUIRES espressif__gmf_fft
)

target_link_libraries(${COMPONENT_LIB} INTERFACE esp_audio_effects_prebuilt)
