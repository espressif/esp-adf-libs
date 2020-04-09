set(COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./esp_upnp/include
                            ./esp_ssdp/include
                            ./recorder_engine/include
                            ./esp_sip/include
                            ./audio_misc/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal esp-sr)

set(COMPONENT_SRCS ./esp_codec/audio_alc.c
                    ./esp_codec/audio_sonic.c
                    ./esp_codec/audio_forge.c
                    ./esp_codec/downmix.c
                    ./esp_codec/equalizer.c
                    ./esp_codec/filter_resample.c
                    ./esp_codec/wav_decoder.c
                    ./esp_codec/wav_encoder.c
                    ./audio_misc/rec_eng_helper.c)

register_component()

set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal)

target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_ssdp/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_upnp/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib")
IF (IDF_VER MATCHES "v4.")
add_prebuilt_library(esp_audio "${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib/libesp_audio.a"
             PRIV_REQUIRES esp-adf-libs)
target_link_libraries(${COMPONENT_TARGET} INTERFACE esp_sip-v4x)
ELSE ()
target_link_libraries(${COMPONENT_TARGET} INTERFACE esp_sip)
ENDIF (IDF_VER MATCHES "v4.")

target_link_libraries(${COMPONENT_TARGET}  "-Wl,--start-group"
                        esp_processing esp_audio esp-aac esp-amr esp-amrwbenc esp-flac esp-ogg-container esp-opus esp-tremor
                        esp_ssdp esp_upnp esp-mp3 codec-utils
                        recorder_engine  "-Wl,--end-group")
