# qca-otr

`qca-otr` is an experimental Qt/QCA implementation of the OTRv3 protocol core.
It deliberately has no dependency on Psi or the Psi plugin API. The existing
`otrplugin` continues to use libotr while this implementation is developed and
validated side-by-side.

The current core contains:

- raw/prehashed DSA operations built on QCA3 modular arithmetic;
- SHA-256 and HMAC-SHA256;
- AES-128-CTR;
- strict OTR wire encoding/decoding for integers, MPI, DATA and DSA public keys;
- RFC 3526 group-5 OTR DH key generation and peer-key validation;
- OTRv3 AKE key derivation (`ssid`, `c`, `c'`, `m1`, `m2`, `m1'`, `m2'`);
- authenticated-signature HMAC/MAC input construction;
- strict binary codecs for D-H Commit, D-H Key, Reveal Signature and Signature messages, including v3 instance tags.

Modular exponentiation, modular inversion and positive modulo are provided by
`QCA::BigIntegerMath` in QCA3 rather than duplicated in this library.

Planned layers are:

1. AKE state machine and inner encrypted-signature payload handling;
2. qca-otr-to-qca-otr AKE tests, including collision/retransmit cases;
3. interoperability tests against libotr 4.1.1 in both initiator directions;
4. encrypted data messages, key rotation, fragmentation and instance routing;
5. SMP;
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
