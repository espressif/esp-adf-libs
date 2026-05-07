idf_component_register(
    SRCS "test_app_main.c" "test_case.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES
        "xml/rootDesc.xml"
        "xml/rootDescLogo.xml"
        "xml/devicelist.xml"
        "xml/SoapResponseOk.xml"
        "xml/SoapResponseErr.xml"
        "xml/rootDescService.xml"
    PRIV_REQUIRES unity esp_media_protocols media_lib_sal esp_http_server
    WHOLE_ARCHIVE)
