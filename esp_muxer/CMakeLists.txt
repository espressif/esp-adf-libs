set(COMPONENT_SRC "src/muxer_reg.c")
set(COMPONENT_INCLUDE "include" "include/impl")

idf_component_register(
    INCLUDE_DIRS ${COMPONENT_INCLUDE}
    SRCS ${COMPONENT_SRC}
)

get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_prebuilt_library(esp_muxer "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}/libesp_muxer.a"
                     PRIV_REQUIRES ${BASE_DIR})
set(TARGET_LIB_NAME esp_muxer)

target_link_libraries(${COMPONENT_LIB}  PRIVATE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CONFIG_IDF_TARGET}")
target_link_libraries(${COMPONENT_LIB}  PRIVATE ${TARGET_LIB_NAME})
