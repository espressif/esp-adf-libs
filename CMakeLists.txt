set(COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./esp_upnp/include
                            ./esp_ssdp/include
                            ./recorder_engine/include
                            ./audio_misc/include
                            ./esp_sip/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal esp-sr)

set(COMPONENT_SRCS ./esp_codec/audio_alc.c
                    ./esp_codec/audio_sonic.c
                    ./esp_codec/audio_forge.c
                    ./esp_codec/downmix.c
                    ./esp_codec/equalizer.c
                    ./esp_codec/filter_resample.c
                    ./esp_codec/wav_encoder.c
                    ./audio_misc/rec_eng_helper.c)

register_component()

set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal)

IF (CONFIG_IDF_TARGET MATCHES "esp32s2")
set(adf_libs_target_dir "esp32s2")
set(esp_sip_name "esp_sip")
set(esp_ssdp_name "esp_ssdp")

ELSEIF (CONFIG_IDF_TARGET MATCHES "esp32")
set(adf_libs_target_dir "esp32")
set(esp_sip_name "esp_sip-v4x")
set(esp_ssdp_name "esp_ssdp-v4x")

ENDIF (CONFIG_IDF_TARGET MATCHES "esp32s2")

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}, Chip is ${adf_libs_target_dir}")

target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_ssdp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_upnp/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib/${adf_libs_target_dir}"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${adf_libs_target_dir}")

if (CONFIG_REC_ENG_ENABLE_VAD_ONLY)
    set(target_recorder_engine "recorder_engine_vad")
elseif (CONFIG_REC_ENG_ENABLE_VAD_WWE)
    set(target_recorder_engine "recorder_engine_vad_wwe")
else()
    set(target_recorder_engine "recorder_engine")
endif()

IF (IDF_VER MATCHES "v4.")
set(name_of_recorder_engine_lib "lib${target_recorder_engine}.a")
add_prebuilt_library(esp_audio "${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/${adf_libs_target_dir}/libesp_audio.a"
             PRIV_REQUIRES esp-adf-libs)
add_prebuilt_library(recorder_engine "${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib/${adf_libs_target_dir}/${name_of_recorder_engine_lib}"
             PRIV_REQUIRES esp-adf-libs)

target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                        esp_processing esp_audio esp-aac  esp-amr esp-amrwbenc esp-flac ${esp_ssdp_name} esp-ogg-container esp-opus esp-tremor
                         ${esp_sip_name} esp_upnp esp-mp3 codec-utils esp-g711
                        recorder_engine  "-Wl,--end-group")
ELSE ()
target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                        esp_processing esp_audio esp-aac esp-amr esp-amrwbenc esp-flac esp_ssdp esp-ogg-container esp-opus esp-tremor
                         esp_upnp esp_sip esp-mp3 codec-utils esp-g711
                        ${target_recorder_engine}  "-Wl,--end-group")
ENDIF (IDF_VER MATCHES "v4.")
