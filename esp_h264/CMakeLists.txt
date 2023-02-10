IF (${IDF_TARGET} STREQUAL "esp32s3") 
idf_component_register(INCLUDE_DIRS "include")
add_prebuilt_library(esp_h264 "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_h264.a"
                PRIV_REQUIRES esp_h264)
target_link_libraries(${COMPONENT_LIB}  INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${idf_target}")
target_link_libraries(${COMPONENT_LIB}  INTERFACE esp_h264 )
ENDIF ()