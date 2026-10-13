
if (CONFIG_EFC_DECODER_BENCH_WITH_FILE)
    set(SRCS "bench_main.c")
else()
    set(SRCS "efc_test_main.c")
endif()

idf_component_register(
    SRCS "${SRCS}"
    INCLUDE_DIRS "."
    EMBED_FILES "assets/giphy.efc" "assets/giphy_z.efc"
    REQUIRES esp_timer esp_efc_codec
)
