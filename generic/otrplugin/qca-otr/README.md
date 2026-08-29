# qca-otr

`qca-otr` is an experimental Qt/QCA implementation of the OTRv3 protocol core.
It deliberately has no dependency on Psi or the Psi plugin API. The existing
`otrplugin` continues to use libotr while this implementation is developed and
validated side-by-side.

The first stage contains only cryptographic and wire-format primitives required
by OTRv3:

- raw/prehashed DSA operations built on QCA3 modular arithmetic;
- SHA-256 and HMAC-SHA256;
- AES-128-CTR;
- strict OTR wire encoding/decoding for integers, MPI, DATA and DSA public keys.

Modular exponentiation, modular inversion and positive modulo are provided by
`QCA::BigIntegerMath` in QCA3 rather than duplicated in this library.

Planned layers are:

1. key derivation and OTRv3 authenticated key exchange;
2. session state machine;
3. encrypted data messages, fragmentation and instance tags;
4. SMP;
5. interoperability tests against libotr 4.1.1;
6. replacement of the libotr backend in Psi's OTR plugin.

## Standalone build

Install a Qt5 or Qt6 build of QCA3 into a prefix and point CMake at that prefix:

```sh
cmake -S . -B build \
  -DQT_DEFAULT_MAJOR_VERSION=6 \
  -DCMAKE_PREFIX_PATH=/path/to/qca3-prefix
cmake --build build
ctest --test-dir build --output-on-failure
```

Use `QT_DEFAULT_MAJOR_VERSION=5` with `Qca3-qt5` for the Windows 7 profile. The
repository CI builds and tests the same source against both Qt 5 and Qt 6.
