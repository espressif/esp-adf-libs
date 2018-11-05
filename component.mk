#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

EXTRA_COMPONENT_DIRS += $(COMPONENT_PATH)/esp_sr/

COMPONENT_ADD_INCLUDEDIRS :=    esp_audio/include \
                                esp_codec/include \
                                esp_codec/wav/include \
                                esp_codec/resample/include \
                                recorder_engine/include \
                                esp_sr/include

COMPONENT_SRCDIRS := . esp_codec

LIBS := esp_codec esp_audio esp-amr esp-amrwbenc esp-aac recorder_engine

ifdef CONFIG_WAKEUP_WORD_HI_LEXIN
    LIBS += vad esp_wakenet nn_model_hilexin_wn3
endif
ifdef CONFIG_WAKEUP_WORD_ALEXA
    LIBS += vad esp_wakenet nn_model_alexa_wn3
endif
ifdef CONFIG_WAKEUP_WORD_LIGHT_CONTROL
    LIBS += vad esp_wakenet nn_model_light_control_ch_wn4
endif
ifdef CONFIG_WAKEUP_WORD_SPEECH_CMD_CH
    LIBS += vad esp_wakenet nn_model_speech_cmd_ch_wn4
endif

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/esp_audio/lib \
                          -L$(COMPONENT_PATH)/esp_codec/lib \
                          -L$(COMPONENT_PATH)/recorder_engine/lib \
                          -L$(COMPONENT_PATH)/esp_sr/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/%/lib/lib%.a,$(LIBS))