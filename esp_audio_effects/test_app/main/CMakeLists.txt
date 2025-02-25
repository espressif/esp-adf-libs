
idf_component_register( SRCS "test_alc.c"  "test_bit_cvt.c" "test_ch_cvt.c" "test_audio_effect_main.c"
                        "test_common.c" "test_data_weaver.c" "test_eq.c" "test_fade.c" "test_mixer.c"
                        "test_rate_cvt.c" "test_sonic.c" "test_ae_performance.c"
                        PRIV_INCLUDE_DIRS "."
                        PRIV_REQUIRES test_utils esp_audio_effects unity esp_timer sdmmc fatfs heap
                        WHOLE_ARCHIVE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=incompatible-pointer-types)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=int-conversion)
