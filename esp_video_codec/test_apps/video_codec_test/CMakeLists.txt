# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
set(COMPONENTS main)

if(EXISTS "$ENV{IDF_PATH}/tools/test_apps/components")
    set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/test_apps/components")
else()
    set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/unit-test-app/components")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(video_codec_test)

