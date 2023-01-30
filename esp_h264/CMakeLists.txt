IF (CONFIG_IDF_TARGET STREQUAL "esp32s3") 
idf_component_register(INCLUDE_DIRS "include")

target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${idf_target}")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib/${idf_target}")
ENDIF ()