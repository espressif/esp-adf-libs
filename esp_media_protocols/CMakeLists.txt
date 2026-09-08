idf_component_register(INCLUDE_DIRS "include")
get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)

idf_build_get_property(build_components BUILD_COMPONENTS)
if("espressif__esp_libsrtp" IN_LIST build_components)
    set(libsrtp_comp espressif__esp_libsrtp)
else()
    set(libsrtp_comp esp_libsrtp)
endif()

add_prebuilt_library(${BASE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_media_protocols.a"
                     PRIV_REQUIRES ${BASE_DIR} ${libsrtp_comp})
target_link_libraries(${COMPONENT_LIB}  INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}")
target_link_libraries(${COMPONENT_LIB}  INTERFACE esp_media_protocols)
