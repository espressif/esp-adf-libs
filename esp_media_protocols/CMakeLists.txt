idf_component_register(INCLUDE_DIRS "include")

idf_build_get_property(build_components BUILD_COMPONENTS)
if("espressif__esp_libsrtp" IN_LIST build_components)
    set(libsrtp_comp espressif__esp_libsrtp)
else()
    set(libsrtp_comp esp_libsrtp)
endif()

add_prebuilt_library(esp_media_protocols_prebuilt
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_media_protocols.a"
    PRIV_REQUIRES ${libsrtp_comp})
idf_component_get_property(libsrtp_lib ${libsrtp_comp} COMPONENT_LIB)
target_link_libraries(${COMPONENT_LIB} INTERFACE
    esp_media_protocols_prebuilt
    "$<TARGET_FILE:${libsrtp_lib}>")
