#This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/components"
                         "$ENV{IDF_PATH}/tools/unit-test-app/components"
                         "$ENV{ADF_PATH}/components/esp-adf-libs/esp_audio_effects")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_audio_effects_test)
