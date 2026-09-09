/* Focused host tests; the runner inserts the actual backend functions below.
 * mbedTLS doubles check lifecycle and encoding, not cryptographic operations. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MBEDTLS_PRIVATE(x) x
#define MBEDTLS_RSA_PKCS_V15 0
#define MBEDTLS_PK_RSA 1
#define LIBSSH2_ERROR_FILE -16
typedef int LIBSSH2_SESSION;
typedef struct { size_t size; unsigned char bytes[256]; } mbedtls_mpi;
typedef struct { int initialized; mbedtls_mpi E, N; } mbedtls_rsa_context;
typedef mbedtls_rsa_context libssh2_rsa_ctx;
typedef struct { int type; mbedtls_rsa_context rsa; } mbedtls_pk_context;
static int fail_alloc, allocations, live, parse_error, parsed_type;
static mbedtls_rsa_context fixture;

typedef union { size_t size; max_align_t alignment; } block_header;
static void *checked_alloc(size_t size)
{
    block_header *block;
    unsigned char *data;
    if(++allocations == fail_alloc)
        return NULL;
    block = malloc(sizeof(*block) + size + 1);
    assert(block);
    block->size = size;
    data = (unsigned char *)(block + 1);
    memset(data, 0, size);
    data[size] = 0xa5;
    live++;
    return data;
}
static void checked_free(void *ptr)
{
    block_header *block;
    if(!ptr)
        return;
    block = (block_header *)ptr - 1;
    assert(((unsigned char *)ptr)[block->size] == 0xa5);
    live--;
    free(block);
}
#define LIBSSH2_ALLOC(s, n) ((void)(s), checked_alloc(n))
#define LIBSSH2_FREE(s, p) ((void)(s), checked_free(p))
#define mbedtls_calloc(n, s) checked_alloc((n) * (s))
static void _libssh2_mbedtls_safe_free(void *p, size_t n)
{ memset(p, 0, n); checked_free(p); }
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
static void mbedtls_rsa_init(mbedtls_rsa_context *rsa)
#else
static void mbedtls_rsa_init(mbedtls_rsa_context *rsa, int padding, int hash)
#endif
{
#if MBEDTLS_VERSION_NUMBER < 0x03000000
    assert(padding == MBEDTLS_RSA_PKCS_V15 && hash == 0);
#endif
    memset(rsa, 0, sizeof(*rsa));
    rsa->initialized = 1;
}
static void mbedtls_rsa_free(mbedtls_rsa_context *rsa)
{ assert(rsa->initialized); rsa->initialized = 0; }
static int mbedtls_rsa_copy(mbedtls_rsa_context *dst,
                            const mbedtls_rsa_context *src)
{ assert(dst->initialized); *dst = *src; return 0; }
static void mbedtls_pk_init(mbedtls_pk_context *pk)
{ memset(pk, 0, sizeof(*pk)); }
static void mbedtls_pk_free(mbedtls_pk_context *pk) { (void)pk; }
static int mbedtls_pk_get_type(const mbedtls_pk_context *pk)
{ return pk->type; }
#define mbedtls_pk_rsa(pk) (&(pk).rsa)
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
static int _libssh2_mbedtls_ctr_drbg;
static int mbedtls_ctr_drbg_random(void *p, unsigned char *out, size_t n)
{ (void)p; (void)out; (void)n; return 0; }
#endif
static int mbedtls_pk_parse_key(mbedtls_pk_context *pk,
    const unsigned char *data, size_t len, const unsigned char *pwd,
    size_t pwdlen
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    , int (*rng)(void *, unsigned char *, size_t), void *rng_ctx
#endif
    )
{
    assert(len == 4 && memcmp(data, "key\0", 4) == 0);
    assert(pwd ? pwdlen == 6 : pwdlen == 0);
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    assert(rng && rng_ctx);
#endif
    pk->type = parsed_type;
    pk->rsa = fixture;
    return parse_error;
}
static size_t mbedtls_mpi_size(const mbedtls_mpi *mpi) { return mpi->size; }
static int mbedtls_mpi_write_binary(const mbedtls_mpi *mpi,
                                   unsigned char *out, size_t len)
{
    assert(len >= mpi->size);
    memset(out, 0, len - mpi->size);
    memcpy(out + len - mpi->size, mpi->bytes, mpi->size);
    return 0;
}
static void _libssh2_htonu32(unsigned char *p, uint32_t n)
{ p[0] = n >> 24; p[1] = n >> 16; p[2] = n >> 8; p[3] = n; }
static int _libssh2_error(LIBSSH2_SESSION *s, int code, const char *msg)
{ (void)s; (void)msg; return code; }

/* BACKEND_FUNCTIONS */

int main(int argc, char **argv)
{
    LIBSSH2_SESSION session = 0;
    size_t i;
    fixture.initialized = 1;
    fixture.E.size = 3;
    fixture.E.bytes[0] = 1;
    fixture.E.bytes[2] = 1;
    fixture.N.size = 256;
    memset(fixture.N.bytes, 0xc3, sizeof(fixture.N.bytes));
    assert(argc == 2);
    if(strcmp(argv[1], "encoding") == 0) {
        for(i = 128; i <= 256; i += 128) {
            size_t len = 0;
            unsigned char *key;
            fixture.N.size = i;
            key = gen_publickey_from_rsa(&session, &fixture, &len);
            assert(len == 23 + i);
            assert(memcmp(key, "\0\0\0\7ssh-rsa\0\0\0\3\1\0\1", 18) == 0);
            assert(key[22] == 0);
            assert(memcmp(key + 23, fixture.N.bytes, i) == 0);
            checked_free(key); /* Check the byte beyond the allocation. */
        }
    }
    else if(strcmp(argv[1], "lifecycle") == 0) {
        for(i = 0; i < 4; i++) {
            libssh2_rsa_ctx *rsa = NULL;
            int rc;
            parse_error = i == 2 ? -1 : 0;
            parsed_type = i == 3 ? 2 : MBEDTLS_PK_RSA;
            rc = _libssh2_mbedtls_rsa_new_private_frommemory(
                &rsa, &session, "key", 3,
                i == 1 ? (const unsigned char *)"secret" : NULL);
            if(i < 2) {
                assert(rc == 0 && rsa && rsa->initialized);
                mbedtls_rsa_free(rsa);
                checked_free(rsa);
            }
            else
                assert(rc != 0 && rsa == NULL);
            assert(live == 0);
        }
    }
    else if(strcmp(argv[1], "derivation") == 0) {
        for(i = 0; i < 3; i++) {
            unsigned char *method = NULL, *key = NULL;
            size_t method_len = 0, key_len = 0;
            mbedtls_pk_context pk = {MBEDTLS_PK_RSA, fixture};
            int rc;
            allocations = 0;
            fail_alloc = (int)i;
            rc = _libssh2_mbedtls_pub_priv_key(&session, &method,
                &method_len, &key, &key_len, &pk);
            if(i == 0) {
                assert(rc == 0 && method && key);
                assert(method_len == 7 && memcmp(method, "ssh-rsa", 7) == 0);
                assert(key_len == 279);
                checked_free(method);
                checked_free(key);
            }
            else
                assert(rc != 0 && method == NULL && key == NULL);
            assert(live == 0);
        }
    }
    else
        return 2;
    assert(live == 0);
    puts("PASS");
    return 0;
}
