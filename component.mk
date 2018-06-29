#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


COMPONENT_ADD_INCLUDEDIRS :=    esp_audio/include \
                                esp_codec/include \
                                esp_codec/wav/include \
                                esp_codec/resample/include \
                                recorder_engine/include \

COMPONENT_SRCDIRS := . esp_codec

LIBS := esp_codec esp_audio esp-aac recorder_engine vad # wakeup

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/esp_audio/lib \
                          -L$(COMPONENT_PATH)/esp_codec/lib \
                          -L$(COMPONENT_PATH)/recorder_engine/lib \
                           $(addprefix -l,$(LIBS)) \

ALL_LIB_FILES += $(patsubst %,$(COMPONENT_PATH)/%/lib/lib%.a,$(LIBS))