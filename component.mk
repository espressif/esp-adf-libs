# "main" pseudo-component makefile.

# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS :=    esp_audio/include \
                                esp_codec/include/codec \
                                esp_codec/include/processing \
                                recorder_engine/include \
                                media_lib_sal/include \
                                media_lib_sal/include/port \
                                audio_misc/include \
                                esp_muxer/include
                                
ifdef CONFIG_MEDIA_PROTOCOL_LIB_ENABLE
    COMPONENT_ADD_INCLUDEDIRS += esp_media_protocols/include
endif

COMPONENT_SRCDIRS := . esp_codec audio_misc media_lib_sal media_lib_sal/port

LIBS := esp_processing esp_audio esp_codec esp_muxer

ifdef CONFIG_MEDIA_PROTOCOL_LIB_ENABLE
    LIBS += esp_media_protocols
    COMPONENT_SRCDIRS += esp_media_protocols
endif

ifdef CONFIG_REC_ENG_ENABLE_VAD_ONLY
 LIBS += recorder_engine_vad
endif

ifdef CONFIG_REC_ENG_ENABLE_VAD_WWE
 LIBS += recorder_engine_vad_wwe
endif

ifdef CONFIG_REC_ENG_ENABLE_VAD_WWE_AMR
 LIBS += recorder_engine
endif

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/esp_audio/lib/esp32 \
                          -L$(COMPONENT_PATH)/esp_codec/lib/esp32 \
                          -L$(COMPONENT_PATH)/recorder_engine/lib/esp32 \
                          -L$(COMPONENT_PATH)/esp_media_protocols/lib/esp32 \
                          -L$(COMPONENT_PATH)/esp_muxer/lib/esp32 \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/%/lib/esp32/lib%.a,$(LIBS))
