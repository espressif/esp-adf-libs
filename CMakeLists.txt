set(COMPONENT_ADD_INCLUDEDIRS ./esp_audio/include
                            ./esp_codec/include/codec
                            ./esp_codec/include/processing
                            ./esp_dlna/include
                            ./esp_upnp/include
                            ./esp_ssdp/include
                            ./recorder_engine/include
                            ./esp_sip/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal)

set(COMPONENT_SRCS ./esp_codec/audio_alc.c
                    ./esp_codec/audio_sonic.c
                    ./esp_codec/audio_forge.c
                    ./esp_codec/downmix.c
                    ./esp_codec/equalizer.c
                    ./esp_codec/filter_resample.c
                    ./esp_codec/wav_decoder.c
                    ./esp_codec/wav_encoder.c)

register_component()

set(COMPONENT_PRIV_REQUIRES audio_pipeline audio_sal)

target_link_libraries(${COMPONENT_NAME} "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_audio/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_codec/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_dlna/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_ssdp/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_upnp/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/esp_sip/lib"
                                "-L ${CMAKE_CURRENT_SOURCE_DIR}/recorder_engine/lib")
target_link_libraries(${COMPONENT_NAME}
                        esp_processing esp-aac esp-amr esp-amrwbenc esp-flac esp-ogg-container esp-opus esp-tremor
                        esp_audio esp_dlna esp_ssdp esp_upnp esp_sip esp-mp3
                        recorder_engine)
