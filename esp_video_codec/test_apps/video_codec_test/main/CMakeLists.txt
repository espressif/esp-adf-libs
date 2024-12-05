
set(srcs  "video_codec_test.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES esp_video_codec unity test_utils esp_timer esp_psram esp_gdbstub
                       WHOLE_ARCHIVE)
