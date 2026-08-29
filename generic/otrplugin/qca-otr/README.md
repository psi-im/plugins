# qca-otr

`qca-otr` is an experimental Qt/QCA implementation of the OTRv3 protocol core.
It deliberately has no dependency on Psi or the Psi plugin API. The existing
`otrplugin` continues to use libotr while this implementation is developed and
validated side-by-side.

The first stage contains only cryptographic primitives required by OTRv3:

- modular exponentiation and modular inversion over `QCA::BigInteger`;
- raw/prehashed DSA operations;
- SHA-256 and HMAC-SHA256;
- AES-128-CTR.

The modular arithmetic helpers are temporary compatibility code. They are
intended to move into QCA3 as native `BigInteger` operations, after which this
library can consume those public APIs without changing its OTR-facing API.

Planned layers are:

1. protocol encoding/decoding and key derivation;
2. OTRv3 authenticated key exchange and session state machine;
3. encrypted data messages, fragmentation and instance tags;
4. SMP;
5. interoperability tests against libotr 4.1.1;
6. replacement of the libotr backend in Psi's OTR plugin.

## Standalone build

```sh
cmake -S . -B build \
  -DQT_DEFAULT_MAJOR_VERSION=6 \
  -DQca3_DIR=/path/to/qca3/lib/cmake/Qca3
cmake --build build
ctest --test-dir build --output-on-failure
```

Use `QT_DEFAULT_MAJOR_VERSION=5` with the Qt5 build of QCA3 for the Windows 7
profile.
