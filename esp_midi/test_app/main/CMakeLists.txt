set(src_dirs                "./"
                            "./load_sound_lib/src")

set(public_include_dirs     "./"
                            "./load_sound_lib/include")

set(priv_requires           unity
                            fatfs)

idf_component_register(SRC_DIRS "${src_dirs}"
                       INCLUDE_DIRS "${public_include_dirs}"
                       PRIV_REQUIRES "${priv_requires}"
                       WHOLE_ARCHIVE)
