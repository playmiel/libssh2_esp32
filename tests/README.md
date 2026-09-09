# RSA backend regression checks

Run from PowerShell with GCC 12 or later on PATH:

```powershell
./tests/test_rsa.ps1
```

The runner extracts the three affected functions from `src/mbedtls.c` and
compiles them with mbedTLS test doubles for both the 2.x and 3.x API branches.
It checks allocation bounds for 1024/2048-bit modulus serialization, RSA
initialization on successful and failed parsing, successful public-key
derivation, and cleanup when either output allocation fails. Pattern-filled
automatic variables make the uninitialized return regression deterministic.

These are host regression checks, not cryptographic or ESP32 integration tests.
Full validation additionally requires an ESP32 build and authentication against
an SSH server with a real RSA private key.

To check an earlier source snapshot or isolate a case:

```powershell
./tests/test_rsa.ps1 -SourcePath /path/to/mbedtls.c -Cases derivation
```
