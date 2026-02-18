set(COMPONENT_INCLUDE include include/impl)

if (CONFIG_EXTRACTOR_CUSTOM_SUPPORT)
    list (APPEND COMPONENT_INCLUDE include/reg)
endif()

set(COMPONENT_SRC "src/esp_extractor_reg.c")

idf_component_register(
    INCLUDE_DIRS ${COMPONENT_INCLUDE}
    SRCS ${COMPONENT_SRC}
)

get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_prebuilt_library(esp_extractor "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_extractor.a"
                     PRIV_REQUIRES ${BASE_DIR})
set(TARGET_LIB_NAME esp_extractor)

target_link_libraries(${COMPONENT_LIB}  PRIVATE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}")
target_link_libraries(${COMPONENT_LIB}  PRIVATE ${TARGET_LIB_NAME})

