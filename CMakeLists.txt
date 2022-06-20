# Set the supported library variable for different chips

IF (CONFIG_IDF_TARGET STREQUAL "esp32")
set(recorder_engine_name "recorder_engine")

ELSEIF (CONFIG_IDF_TARGET STREQUAL "esp32s2")
set(recorder_engine_name "")

ELSEIF (CONFIG_IDF_TARGET STREQUAL "esp32s3")
set(recorder_engine_name "recorder_engine")

ELSEIF (CONFIG_IDF_TARGET STREQUAL "esp32c3")
set(recorder_engine_name "")

ENDIF ()


# Add the source code and header file
list(APPEND COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./media_lib_sal/include
                            ./media_lib_sal/include/port
                            ./recorder_engine/include
                            ./recorder_engine/include
                            ./audio_misc/include
                            ./esp_muxer/include)

if (CONFIG_MEDIA_PROTOCOL_LIB_ENABLE)
    list(APPEND COMPONENT_ADD_INCLUDEDIRS  ./esp_media_protocols/include)
endif()

list(APPEND COMPONENT_SRCS ./esp_codec/audio_alc.c
                    ./esp_codec/audio_sonic.c
                    ./esp_codec/audio_forge.c
                    ./esp_codec/downmix.c
                    ./esp_codec/equalizer.c
                    ./esp_codec/filter_resample.c
                    ./esp_codec/wav_encoder.c
                    ./media_lib_sal/media_lib_common.c
                    ./media_lib_sal/media_lib_os.c ./media_lib_sal/port/media_lib_os_freertos.c
                    ./media_lib_sal/media_lib_adapter.c)

if (CONFIG_MEDIA_PROTOCOL_LIB_ENABLE)
list(APPEND COMPONENT_SRCS ./media_lib_sal/media_lib_crypt.c  ./media_lib_sal/port/media_lib_crypt_default.c
                           ./media_lib_sal/media_lib_socket.c ./media_lib_sal/port/media_lib_socket_default.c
                           ./media_lib_sal/media_lib_tls.c  ./media_lib_sal/port/media_lib_tls_default.c)
endif()

IF (NOT (recorder_engine_name STREQUAL ""))
    list(APPEND COMPONENT_ADD_INCLUDEDIRS ./audio_misc/include)
    list(APPEND COMPONENT_SRCS  ./audio_misc/rec_eng_helper.c)

    if (CONFIG_REC_ENG_ENABLE_VAD_ONLY)
        set(target_recorder_engine "recorder_engine_vad")
    elseif (CONFIG_REC_ENG_ENABLE_VAD_WWE)
        set(target_recorder_engine "recorder_engine_vad_wwe")
    else()
        set(target_recorder_engine "recorder_engine")
    endif()
ELSE()
    set(target_recorder_engine "")
ENDIF ()


set(COMPONENT_REQUIRES audio_pipeline audio_sal esp-sr nghttp esp-tls)

register_component()


message(STATUS "Chip is ${CONFIG_IDF_TARGET}, ${CMAKE_CURRENT_SOURCE_DIR}")

set(TARGET_LIB_PATH "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${CONFIG_IDF_TARGET}"
                    "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${CONFIG_IDF_TARGET}"
                    "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${CONFIG_IDF_TARGET}")

if (CONFIG_MEDIA_PROTOCOL_LIB_ENABLE)
    list(APPEND TARGET_LIB_PATH  "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_media_protocols/lib/${CONFIG_IDF_TARGET}")
endif()

target_link_libraries(${COMPONENT_TARGET} "${TARGET_LIB_PATH}")


# GREATER_EQUAL is only available on CMake +3.7.
# To be totally compatible with older versions, let's use GREATER instead
IF (IDF_VERSION_MAJOR GREATER 3)
    set(name_of_recorder_engine_lib "lib${target_recorder_engine}.a")

    add_prebuilt_library(esp_audio "${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${CONFIG_IDF_TARGET}/libesp_audio.a"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(esp_codec "${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${CONFIG_IDF_TARGET}/libesp_codec.a"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(esp_processing "${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${CONFIG_IDF_TARGET}/libesp_processing.a"
                 PRIV_REQUIRES esp-adf-libs)

    add_prebuilt_library(recorder_engine "${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${CONFIG_IDF_TARGET}/${name_of_recorder_engine_lib}"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(esp_muxer "${CMAKE_CURRENT_SOURCE_DIR}/esp_muxer/lib/${CONFIG_IDF_TARGET}/libesp_muxer.a"
                 PRIV_REQUIRES esp-adf-libs)
ENDIF (IDF_VERSION_MAJOR GREATER 3)

set(TARGET_LIB_NAME esp_processing esp_audio esp_codec esp_muxer ${target_recorder_engine})
if (CONFIG_MEDIA_PROTOCOL_LIB_ENABLE)
    list(APPEND TARGET_LIB_NAME esp_media_protocols)
endif()

target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group" ${TARGET_LIB_NAME} "-Wl,--end-group")
