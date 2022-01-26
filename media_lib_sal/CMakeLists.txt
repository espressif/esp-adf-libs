set(COMPONENT_ADD_INCLUDEDIRS include include/port)

# Edit following two lines to set component requirements (see docs)

set(COMPONENT_SRCS ./media_lib_common.c
     ./media_lib_os.c ./port/media_lib_os_freertos.c
    ./media_lib_adapter.c)

if (CONFIG_MEDIA_PROTOCOL_LIB_ENABLE)
    set(COMPONENT_REQUIRES mbedtls)
    list(APPEND COMPONENT_SRCS  ./media_lib_socket.c ./port/media_lib_socket_default.c
                                ./media_lib_crypt.c  ./port/media_lib_crypt_default.c
                                ./media_lib_tls.c  ./port/media_lib_tls_default.c)
endif()

register_component()
