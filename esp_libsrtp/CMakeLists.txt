cmake_minimum_required(VERSION 3.5)

set(SOURCES_C
  libsrtp/srtp/srtp.c
)

set(CIPHERS_SOURCES_C
  libsrtp/crypto/cipher/cipher.c
  libsrtp/crypto/cipher/null_cipher.c
)

list(APPEND CIPHERS_SOURCES_C
  libsrtp/crypto/cipher/aes.c
  libsrtp/crypto/cipher/aes_icm.c
  libsrtp/crypto/cipher/cipher_test_cases.c
)

set(HASHES_SOURCES_C
    libsrtp/crypto/hash/auth.c
    libsrtp/crypto/hash/null_auth.c
)

list(APPEND HASHES_SOURCES_C
  libsrtp/crypto/hash/hmac.c
  libsrtp/crypto/hash/sha1.c
  libsrtp/crypto/hash/auth_test_cases.c
)

set(KERNEL_SOURCES_C
  libsrtp/crypto/kernel/alloc.c
  libsrtp/crypto/kernel/crypto_kernel.c
  libsrtp/crypto/kernel/err.c
  libsrtp/crypto/kernel/key.c
)

set(MATH_SOURCES_C
  libsrtp/crypto/math/datatypes.c
)

set(REPLAY_SOURCES_C
  libsrtp/crypto/replay/rdb.c
  libsrtp/crypto/replay/rdbx.c
)

set(SRTP_SRCS
  ${SOURCES_C}
  ${CIPHERS_SOURCES_C}
  ${HASHES_SOURCES_C}
  ${KERNEL_SOURCES_C}
  ${MATH_SOURCES_C}
  ${REPLAY_SOURCES_C}
)

set(SRTP_INCLUDE_DIRS
	esp-port
	libsrtp/include
	libsrtp/crypto/include
)

idf_component_register(SRCS ${SRTP_SRCS} INCLUDE_DIRS ${SRTP_INCLUDE_DIRS})
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DHAVE_CONFIG_H")
