#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_ADD_INCLUDEDIRS :=    esp_audio/include \
                                esp_codec/include/codec \
                                esp_codec/include/processing \
                                recorder_engine/include \
                                esp_sr/include \
                                esp_ssdp/include \
                                esp_dlna/include \
                                esp_upnp/include \
                                esp_sip/include

COMPONENT_SRCDIRS := . esp_codec

LIBS := esp_processing esp_audio esp-amr esp-amrwbenc esp-aac esp-ogg-container esp-opus esp-tremor esp-flac esp_ssdp esp_upnp esp_dlna esp_sip

ifdef CONFIG_WAKEUP_WORD_HI_LEXIN
    LIBS += vad esp_wakenet nn_model_hilexin_wn5
endif
ifdef CONFIG_WAKEUP_WORD_ALEXA
    LIBS += vad esp_wakenet nn_model_alexa_wn3
endif
ifdef CONFIG_WAKEUP_WORD_LIGHT_CONTROL_CH
    LIBS += vad esp_wakenet nn_model_light_control_ch_wn4
endif
ifdef CONFIG_WAKEUP_WORD_SPEECH_CMD_CH
    LIBS += vad esp_wakenet nn_model_speech_cmd_ch_wn4
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

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/esp_audio/lib \
                          -L$(COMPONENT_PATH)/esp_codec/lib \
                          -L$(COMPONENT_PATH)/recorder_engine/lib \
                          -L$(COMPONENT_PATH)/esp_sr/lib \
                          -L$(COMPONENT_PATH)/esp_ssdp/lib \
                          -L$(COMPONENT_PATH)/esp_upnp/lib \
                          -L$(COMPONENT_PATH)/esp_dlna/lib \
                          -L$(COMPONENT_PATH)/esp_sip/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/%/lib/lib%.a,$(LIBS))
