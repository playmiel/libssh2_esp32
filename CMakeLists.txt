# ESP-IDF component build for the vendored libssh2 sources.
set(LIBSSH2_SOURCES
    src/agent.c src/bcrypt_pbkdf.c src/blowfish.c src/chacha.c
    src/channel.c src/cipher-chachapoly.c src/comp.c src/crypt.c
    src/crypto.c src/global.c src/hostkey.c src/keepalive.c
    src/kex.c src/knownhost.c src/mac.c src/mbedtls.c src/misc.c
    src/packet.c src/pem.c src/poly1305.c src/publickey.c src/scp.c
    src/session.c src/sftp.c src/transport.c src/userauth.c
    src/userauth_kbd_packet.c src/version.c src/libssh2_esp.c)

idf_component_register(
    SRCS ${LIBSSH2_SOURCES}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS src
    REQUIRES mbedtls esp_netif esp_timer esp_system)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LIBSSH2_MBEDTLS HAVE_CONFIG_H HAVE_LIBSSH2_H LIBSSH2_NO_ZLIB)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=format -Wno-error=maybe-uninitialized
    -Wno-error=uninitialized)
