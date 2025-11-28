idf_component_register( SRC_DIRS "."
                        PRIV_INCLUDE_DIRS "."
                        PRIV_REQUIRES test_utils unity esp_timer
                                      sdmmc fatfs heap esp-tls nvs_flash esp_event
                                      esp_netif esp_http_client
                        WHOLE_ARCHIVE)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=incompatible-pointer-types)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=int-conversion)
