# ESP-EFC codec ESP-IDF component.
#
# Pixel-move acceleration: on ESP32-P4 (RISC-V PIE) and ESP32-S3 (Xtensa PIE)
# the bulk copy uses a 128-bit-vector memcpy in assembly; every other target
# (and the PC build) uses portable C. Behaviour is controlled by Kconfig
# (CONFIG_ESP_EFC_USE_SIMD, CONFIG_ESP_EFC_DECODE_4X4_ONLY).

set(efc_srcs "src/esp_efc_codec.c")
set(efc_defs "")

if(CONFIG_ESP_EFC_USE_SIMD AND
   ("${IDF_TARGET}" STREQUAL "esp32p4" OR
    "${IDF_TARGET}" STREQUAL "esp32s3"))
    list(APPEND efc_defs "ESP_EFC_USE_SIMD")
endif()

if(CONFIG_ESP_EFC_DECODE_4X4_ONLY)
    list(APPEND efc_defs "ESP_EFC_DECODE_4X4_ONLY")
endif()

if(CONFIG_ESP_EFC_LVGL_WIDGET)
    list(APPEND efc_srcs "efc_player/lv_efc.c")
    if(CONFIG_ESP_EFC_USE_SIMD AND
       ("${IDF_TARGET}" STREQUAL "esp32p4" OR "${IDF_TARGET}" STREQUAL "esp32s3"))
        set_source_files_properties(
            "efc_player/lv_efc.c"
            PROPERTIES COMPILE_DEFINITIONS "ESP_EFC_USE_SIMD"
        )
    endif()
endif()

set(COMPONENT_REQUIRES esp_timer)
if(${IDF_TARGET} STREQUAL "esp32p4" OR ${IDF_TARGET} STREQUAL "esp32s31")
    list(APPEND COMPONENT_REQUIRES esp_driver_ppa esp_hw_support esp_mm)
    if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "6.0")
        list(APPEND COMPONENT_REQUIRES esp_driver_dma)
    endif()
endif()

idf_component_register(
    SRCS ${efc_srcs}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_inc" "src"
    PRIV_REQUIRES ${COMPONENT_REQUIRES}
)

# The decoder implementation and target-specific SIMD routines are distributed
# as one prebuilt archive. Feature selection remains in the public wrapper.
set(efc_lib
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/${IDF_TARGET}/libesp_efc_codec.a"
)
if(NOT EXISTS "${efc_lib}")
    message(FATAL_ERROR
        "No prebuilt EFC decoder library for target ${IDF_TARGET}: ${efc_lib}"
    )
endif()
add_prebuilt_library(esp_efc_codec_core "${efc_lib}")
target_link_libraries(${COMPONENT_LIB} PRIVATE esp_efc_codec_core)

if(CONFIG_ESP_EFC_LVGL_WIDGET)
    idf_build_get_property(build_components BUILD_COMPONENTS)
    if("lvgl" IN_LIST build_components)
        set(lvgl_name lvgl)
    else()
        set(lvgl_name lvgl__lvgl)
    endif()
    idf_component_get_property(lvgl_lib ${lvgl_name} COMPONENT_LIB)
    idf_component_get_property(lvgl_dir ${lvgl_name} COMPONENT_DIR)
    target_include_directories(${COMPONENT_LIB} PRIVATE "${lvgl_dir}")
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${lvgl_lib})
endif()

if(efc_defs)
    set_source_files_properties(
        "src/esp_efc_codec.c"
        PROPERTIES COMPILE_DEFINITIONS "${efc_defs}"
    )
endif()
