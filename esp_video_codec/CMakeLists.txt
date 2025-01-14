
set(COMPONENT_SRC
    "src/esp_video_enc_default.c"
    "src/esp_video_dec_default.c"
    "src/esp_video_enc.c"
    "src/esp_video_dec.c"
    "src/esp_video_codec_version.c"
    "src/video_codec_utils.c"
    "src/video_codec_reg.c"
)

set(COMPONENT_INCLUDE "include"
    "include/decoder"
    "include/decoder/impl"
    "include/encoder"
    "include/encoder/impl"
)

set(COMPONENT_PRIV_INCLUDE  "./inc")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/inc/hw_caps/${IDF_TARGET}")
    list(APPEND COMPONENT_PRIV_INCLUDE  "./inc/hw_caps/${IDF_TARGET}")
endif()

list(APPEND COMPONENT_REQUIRES esp_mm)

if(${IDF_TARGET} STREQUAL "esp32p4")
    list(APPEND COMPONENT_REQUIRES esp_driver_jpeg)
endif()

if(CONFIG_VIDEO_DECODER_HW_MJPEG_SUPPORT)
    list(APPEND COMPONENT_SRC "src/decoder/esp_video_dec_mjpeg.c")
endif()

if(CONFIG_VIDEO_DECODER_SW_MJPEG_SUPPORT)
    list(APPEND COMPONENT_SRC "src/decoder/esp_video_dec_sw_mjpeg.c")
endif()

if(CONFIG_VIDEO_DECODER_SW_H264_SUPPORT)
    list(APPEND COMPONENT_SRC "src/decoder/esp_video_dec_sw_h264.c")
endif()

if (CONFIG_VIDEO_ENCODER_HW_MJPEG_SUPPORT)
    list(APPEND COMPONENT_SRC "src/encoder/esp_video_enc_mjpeg.c")
endif()

if (CONFIG_VIDEO_ENCODER_SW_MJPEG_SUPPORT)
    list(APPEND COMPONENT_SRC "src/encoder/esp_video_enc_sw_mjpeg.c")
endif()

if(CONFIG_VIDEO_ENCODER_HW_H264_SUPPORT)
    list(APPEND COMPONENT_SRC "src/encoder/esp_video_enc_h264.c")
endif()

if(CONFIG_VIDEO_ENCODER_SW_H264_SUPPORT)
    list(APPEND COMPONENT_SRC "src/encoder/esp_video_enc_sw_h264.c")
endif()

idf_component_register(
    INCLUDE_DIRS ${COMPONENT_INCLUDE}
    SRCS ${COMPONENT_SRC}
    PRIV_INCLUDE_DIRS ${COMPONENT_PRIV_INCLUDE}
    REQUIRES ${COMPONENT_REQUIRES}
)
