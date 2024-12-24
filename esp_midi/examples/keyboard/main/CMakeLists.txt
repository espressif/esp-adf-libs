set(src_dirs                "./"
                            "./app"
                            "./load_sound_lib/src")

set(public_include_dirs     "./"
                            "./app"
                            "./load_sound_lib/include"
                            "./midi_sound_files")

set(priv_requires           )

idf_component_register(SRC_DIRS "${src_dirs}"
                       INCLUDE_DIRS "${public_include_dirs}"
                       PRIV_REQUIRES "${priv_requires}")
