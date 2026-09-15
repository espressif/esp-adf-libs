idf_component_register(INCLUDE_DIRS "include")


set(IMAGE_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}")
if(CONFIG_IDF_TARGET_ESP32P4)
    if(CONFIG_ESP32P4_SELECTS_REV_LESS_V3)
        set(IMAGE_EFFECTS_LIB_TARGET "esp32p4_less_v3")
    endif()
    message(STATUS "esp_image_effects: ESP32-P4 prebuilt library profile: ${IMAGE_EFFECTS_LIB_TARGET}")
endif()

if(CONFIG_IDF_TARGET_ESP32S31 AND NOT CONFIG_ESP_IMAGE_EFFECTS_S31_USE_ASM)
    set(IMAGE_EFFECTS_LIB_TARGET "${CONFIG_IDF_TARGET}_esp_c")
endif()

add_prebuilt_library(esp_image_effects_prebuilt
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${IMAGE_EFFECTS_LIB_TARGET}/libesp_image_effects.a"
)

target_link_libraries(${COMPONENT_LIB}  INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${IMAGE_EFFECTS_LIB_TARGET}")
target_link_libraries(${COMPONENT_LIB}  INTERFACE esp_image_effects_prebuilt)
