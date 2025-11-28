set(EMBED_FILES_LIST "")

if(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER)
    list(APPEND EMBED_FILES_LIST "tone.pcm" "voice_with_music.pcm")
elseif(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC)
    if(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO)
        list(APPEND EMBED_FILES_LIST "voice_flick_up_and_down.pcm")
    elseif(CONFIG_ESP_AUDIO_EFFECTS_DEMO_REMOVE_HIT_DEMO)
        list(APPEND EMBED_FILES_LIST "voice_with_hit.pcm")
    endif()
elseif(CONFIG_ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT)
    list(APPEND EMBED_FILES_LIST "manen_48000_2_24_10.pcm")
else()
    list(APPEND EMBED_FILES_LIST "voice_with_music.pcm")
endif()

idf_component_register(
    SRCS "esp_audio_effects_demo.c"
    PRIV_REQUIRES esp_board_manager
    EMBED_FILES ${EMBED_FILES_LIST}
)
