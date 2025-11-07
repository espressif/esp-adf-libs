set(src_dirs                "./")

set(public_include_dirs     "./")

set(priv_requires unity
                  fatfs
                  media_lib_sal)

idf_component_register(SRC_DIRS "${src_dirs}"
                       INCLUDE_DIRS "${public_include_dirs}"
                       PRIV_REQUIRES "${priv_requires}"
                       WHOLE_ARCHIVE)
