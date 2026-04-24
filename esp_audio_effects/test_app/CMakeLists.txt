#This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_audio_effects_test)
