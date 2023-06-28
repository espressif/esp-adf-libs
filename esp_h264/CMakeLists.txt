IF (${IDF_TARGET} STREQUAL "esp32s3")
idf_component_register(INCLUDE_DIRS "include")
get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_prebuilt_library(${BASE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_h264.a"
                PRIV_REQUIRES ${BASE_DIR})
target_link_libraries(${COMPONENT_LIB}  INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${idf_target}")
target_link_libraries(${COMPONENT_LIB}  INTERFACE esp_h264 )
ENDIF ()