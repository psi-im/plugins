# qca-otr

`qca-otr` is an experimental Qt/QCA implementation of the OTRv3 protocol core.
It deliberately has no dependency on Psi or the Psi plugin API. The existing
`otrplugin` continues to use libotr while this implementation is developed and
validated side-by-side.

The current core contains:

- raw/prehashed DSA operations built on QCA3 modular arithmetic, with unbiased nonce generation;
- SHA-1, SHA-256 and HMAC-SHA256;
- AES-128-CTR;
- strict OTR wire encoding/decoding for integers, MPI, DATA and DSA public keys;
- RFC 3526 group-5 OTR DH key generation and peer-key validation;
- OTRv3 AKE key derivation (`ssid`, `c`, `c'`, `m1`, `m2`, `m1'`, `m2'`);
- libotr-compatible encrypted authenticators (`PUBKEY || keyid || SIG`) and DSA fingerprints;
- strict binary codecs for D-H Commit, D-H Key, Reveal Signature and Signature messages, including v3 instance tags;
- the OTRv3 AKE state machine, including simultaneous-start collision resolution and retransmission behavior.

Modular exponentiation, modular inversion and positive modulo are provided by
`QCA::BigIntegerMath` in QCA3 rather than duplicated in this library.

The regular qca-otr library remains QtCore + QCA3 only. An optional test target
can link against libotr 4.1.1 as an interoperability oracle; CI enables it and
runs complete AKE handshakes with qca-otr and libotr acting as initiator in turn.

Planned layers are:

1. encrypted data messages and OTRv3 session key rotation;
2. fragmentation and full instance routing;
3. SMP;
4. persistence/migration of existing OTR identities, fingerprints and instance tags;
5. replacement of the libotr backend in Psi's OTR plugin;
6. removal of the libotr/libgcrypt/libgpg-error runtime/build dependencies.

## Standalone build

Install a Qt5 or Qt6 build of QCA3 into a prefix and point CMake at that prefix:

```sh
cmake -S . -B build \
  -DQT_DEFAULT_MAJOR_VERSION=6 \
  -DCMAKE_PREFIX_PATH=/path/to/qca3-prefix
cmake --build build
ctest --test-dir build --output-on-failure
```

To run the optional libotr oracle test, install libotr 4.1.1 development files
and add `-DQCA_OTR_BUILD_LIBOTR_INTEROP_TESTS=ON` when configuring.

Use `QT_DEFAULT_MAJOR_VERSION=5` with `Qca3-qt5` for the Windows 7 profile. The
repository CI builds and tests the same source against both Qt 5 and Qt 6.
