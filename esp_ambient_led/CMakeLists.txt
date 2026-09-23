idf_component_register(INCLUDE_DIRS "include")

# During the early component_get_requirements pass, CONFIG_IDF_TARGET may be empty.
if(NOT CONFIG_IDF_TARGET)
    return()
endif()

set(ESP_AMBIENT_LED_LIB_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/idf_v${IDF_VERSION_MAJOR}/${CONFIG_IDF_TARGET}/libesp_ambient_led.a")

if(NOT EXISTS "${ESP_AMBIENT_LED_LIB_PATH}")
    message(FATAL_ERROR
        "esp_ambient_led: no prebuilt library for IDF ${IDF_VERSION_MAJOR} / ${CONFIG_IDF_TARGET}\n"
        "  Expected: lib/idf_v${IDF_VERSION_MAJOR}/${CONFIG_IDF_TARGET}/libesp_ambient_led.a")
endif()

add_prebuilt_library(esp_ambient_led_prebuilt "${ESP_AMBIENT_LED_LIB_PATH}")
target_link_libraries(${COMPONENT_LIB} INTERFACE esp_ambient_led_prebuilt)
