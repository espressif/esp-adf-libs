# ESP-EFC codec performance test app. Uses the esp_efc_codec component, which
# lives in the parent directory.
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/..")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(efc_test_app)
