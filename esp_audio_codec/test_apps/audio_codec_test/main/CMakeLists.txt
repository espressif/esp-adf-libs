set(srcs 
    "audio_codec_test.c" 
    "audio_decoder_test.c"
    "audio_encoder_test.c"
    "simple_decoder_test.c"
)

idf_component_register(SRCS ${srcs}
                       EMBED_TXTFILES "test.mp3" "test.flac"
                       PRIV_REQUIRES esp_audio_codec esp_ringbuf unity test_utils esp_timer esp_psram
                       WHOLE_ARCHIVE)
