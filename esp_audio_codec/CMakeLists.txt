
# Set the supported library variable for different chips
# Add the source code and header file
idf_component_register(INCLUDE_DIRS "include")
add_prebuilt_library(esp_audio_codec "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_audio_codec.a"
                     PRIV_REQUIRES esp_audio_codec)
target_link_libraries(${COMPONENT_LIB}  INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}")
target_link_libraries(${COMPONENT_LIB}  INTERFACE esp_audio_codec)