idf_component_register(INCLUDE_DIRS "include")

add_prebuilt_library(esp_audio_effects_prebuilt
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_audio_effects.a"
    PRIV_REQUIRES espressif__gmf_fft
)

target_link_libraries(${COMPONENT_LIB} INTERFACE esp_audio_effects_prebuilt)
