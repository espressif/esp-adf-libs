
idf_component_register(
    INCLUDE_DIRS include include/port
    SRC_DIRS ./ ./port ./mem_trace
    REQUIRES esp-tls mbedtls esp_netif
    WHOLE_ARCHIVE
)
