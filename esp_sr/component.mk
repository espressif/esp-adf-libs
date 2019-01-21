#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS := include

ifdef CONFIG_WAKEUP_WORD_HI_LEXIN
    LIBS := vad esp_wakenet nn_model_hilexin_wn3
endif
ifdef CONFIG_WAKEUP_WORD_ALEXA
    LIBS := vad esp_wakenet nn_model_alexa_wn3
endif
ifdef CONFIG_WAKEUP_WORD_LIGHT_CONTROL
    LIBS := vad esp_wakenet nn_model_light_control_ch_wn4
endif
ifdef CONFIG_WAKEUP_WORD_SPEECH_CMD_CH
    LIBS := vad esp_wakenet nn_model_speech_cmd_ch_wn4
endif


COMPONENT_ADD_LDFLAGS += -L$(COMPONENT_PATH)/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/lib/lib%.a,$(LIBS))