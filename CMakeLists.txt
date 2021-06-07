IF (CONFIG_IDF_TARGET MATCHES "esp32s2")
set(COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./esp_upnp/include
                            ./esp_ssdp/include
                            ./recorder_engine/include
                            ./audio_misc/include
                            ./esp_sip/include)
ELSE ()
set(COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./esp_upnp/include
                            ./esp_ssdp/include
                            ./recorder_engine/include
                            ./audio_misc/include
                            ./esp_sip/include
                            ./mrm_client/include)
ENDIF (CONFIG_IDF_TARGET MATCHES "esp32s2")

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES audio_pipeline audio_sal esp-sr nghttp tcp_transport)

set(COMPONENT_SRCS ./esp_codec/audio_alc.c
                    ./esp_codec/audio_sonic.c
                    ./esp_codec/audio_forge.c
                    ./esp_codec/downmix.c
                    ./esp_codec/equalizer.c
                    ./esp_codec/filter_resample.c
                    ./esp_codec/wav_encoder.c
                    ./audio_misc/rec_eng_helper.c)

register_component()

IF (CONFIG_IDF_TARGET STREQUAL "esp32s2")
set(adf_libs_target_dir "esp32s2")
set(esp_sip_name "esp_sip")
set(esp_ssdp_name "esp_ssdp")

ELSEIF (CONFIG_IDF_TARGET STREQUAL "esp32")
set(adf_libs_target_dir "esp32")
set(esp_sip_name "esp_sip-v4x")
set(esp_ssdp_name "esp_ssdp-v4x")

ELSEIF (CONFIG_IDF_TARGET STREQUAL "esp32s3")
set(adf_libs_target_dir "esp32s3")
set(esp_sip_name "esp_sip")
set(esp_ssdp_name "esp_ssdp")

ENDIF (CONFIG_IDF_TARGET STREQUAL "esp32s2")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}, Chip is ${adf_libs_target_dir}")

IF (CONFIG_IDF_TARGET MATCHES "esp32s2")
target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_ssdp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_upnp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${adf_libs_target_dir}")
ELSE()
target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_ssdp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_upnp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/mrm_client/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${adf_libs_target_dir}")
ENDIF (CONFIG_IDF_TARGET MATCHES "esp32s2")


if (CONFIG_REC_ENG_ENABLE_VAD_ONLY)
    set(target_recorder_engine "recorder_engine_vad")
elseif (CONFIG_REC_ENG_ENABLE_VAD_WWE)
    set(target_recorder_engine "recorder_engine_vad_wwe")
else()
    set(target_recorder_engine "recorder_engine")
endif()

IF (IDF_VER MATCHES "v4.")
    set(name_of_recorder_engine_lib "lib${target_recorder_engine}.a")
    set(name_of_esp_sip_lib "lib${esp_sip_name}.a")

    add_prebuilt_library(esp_audio "${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${adf_libs_target_dir}/libesp_audio.a"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(recorder_engine "${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${adf_libs_target_dir}/${name_of_recorder_engine_lib}"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(esp_codec "${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${adf_libs_target_dir}/libesp_codec.a"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(esp_processing "${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${adf_libs_target_dir}/libesp_processing.a"
                 PRIV_REQUIRES esp-adf-libs)
    add_prebuilt_library(${esp_sip_name} "${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib/${adf_libs_target_dir}/${name_of_esp_sip_lib}"
                 PRIV_REQUIRES tcp_transport)
    IF (CONFIG_IDF_TARGET MATCHES "esp32s2")
    target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                            esp_processing esp_audio esp_codec ${esp_ssdp_name} ${esp_sip_name} esp_upnp 
                            recorder_engine  "-Wl,--end-group")
    ELSE()
    target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                            esp_processing esp_audio esp_codec ${esp_ssdp_name} ${esp_sip_name} esp_upnp mrm_client
                            recorder_engine  "-Wl,--end-group")
    ENDIF (CONFIG_IDF_TARGET MATCHES "esp32s2")
ELSE ()
target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                        esp_processing esp_audio esp_codec esp_ssdp esp_upnp esp_sip
                        ${target_recorder_engine}  "-Wl,--end-group")
ENDIF (IDF_VER MATCHES "v4.")
