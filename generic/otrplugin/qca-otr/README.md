<!--
SPDX-FileCopyrightText: 2026 Sergei Ilinykh
SPDX-License-Identifier: MIT
-->

# qca-otr

`qca-otr` is an experimental Qt/QCA implementation of the OTRv3 protocol core.
It deliberately has no dependency on Psi or the Psi plugin API. The existing
`otrplugin` continues to use libotr while this implementation is developed and
validated side-by-side.

The current core contains:

- raw/prehashed DSA operations built on QCA3 modular arithmetic, with unbiased nonce generation;
- SHA-1, SHA-256, HMAC-SHA1 and HMAC-SHA256;
- AES-128-CTR;
- strict OTR wire encoding/decoding for integers, MPI, DATA and DSA public keys;
- RFC 3526 group-5 OTR DH key generation and peer-key validation;
- OTRv3 AKE key derivation (`ssid`, `c`, `c'`, `m1`, `m2`, `m1'`, `m2'`);
- libotr-compatible encrypted authenticators (`PUBKEY || keyid || SIG`) and DSA fingerprints;
- strict binary codecs for D-H Commit, D-H Key, Reveal Signature, Signature and Data messages, including v3 instance tags;
- the OTRv3 AKE state machine, including simultaneous-start collision resolution and retransmission behavior;
- OTRv3 encrypted Data Messages with directional session-key derivation, AES-CTR counters and replay protection;
- the OTRv3 DH/key-id ratchet, including current/old session-key slots and revealed MAC keys;
- OTR transport armoring and exact libotr-compatible OTRv3 fragmentation/reassembly;
- libotr-compatible Query Message/version negotiation and OTR whitespace capability tags;
- manual, opportunistic and require-encryption policy behavior at the routed session layer;
- strict generic OTR TLV framing integrated into the `plaintext || NUL || TLVs` Data envelope;
- DISCONNECTED control handling with per-instance PLAINTEXT/ENCRYPTED/FINISHED state semantics and restart support;
- SYMKEY control messages with the derived extra symmetric key kept in `QCA::SecureArray` at the session API boundary;
- canonical `?OTR Error: ...` generation, tolerant error parsing, unreadable-message handling and automatic error-triggered AKE restart policy;
- a complete OTRv3 SMP state machine with question/answer, success/failure, abort and cheating handling, using libotr-compatible combined-secret derivation while retaining secret material in `QCA::SecureArray`;
- transport-facing `OtrSession` routing with independent AKE/Data/SMP children for multiple remote instance tags and broadcast/master AKE cloning;
- libotr-compatible persistence codecs for `otr.keys`, `otr.fingerprints` and `otr.instags`, including Psi's historical `prpl-jabber` on-disk protocol identifier;
- profile migration/loading with exact DSA identity, arbitrary fingerprint trust and OTRv3 instance-tag preservation, plus atomic store replacement.

Modular exponentiation and modular inversion are provided by
`QCA::BigIntegerMath` in QCA3. Small normalization helpers that are only needed
inside qca-otr remain private implementation details.

The regular qca-otr library remains QtCore + QCA3 only. Optional test targets
link against libotr 4.1.1 as an interoperability oracle; CI enables them and
validates complete AKE handshakes, encrypted Data Message exchanges, exact
fragment streams, fragmented public-API exchanges, multi-instance broadcast AKE,
query and whitespace negotiation, generic TLVs, disconnect/restart semantics,
SYMKEY extra-key agreement, protocol-error/unreadable behavior, bidirectional
SMP success/failure/question/abort flows, and persistence/profile migration in
both directions. The persistence oracle also performs a complete disk migration
from a libotr-created legacy profile through qca-otr and back to libotr while
checking the exact identity fingerprint, trust string and instance tag.

The detailed completion roadmap is in [PLAN.md](PLAN.md). The next layer is the
Psi adapter and the remaining adapter-facing identity/trust/key-management API,
followed by the backend switch and dependency cleanup.

## Standalone build

Install a Qt5 or Qt6 build of QCA3 into a prefix and point CMake at that prefix:

```sh
cmake -S . -B build \
  -DQT_DEFAULT_MAJOR_VERSION=6 \
  -DCMAKE_PREFIX_PATH=/path/to/qca3-prefix
cmake --build build
ctest --test-dir build --output-on-failure
```

To run the optional libotr oracle tests, install libotr 4.1.1 development files
and add `-DQCA_OTR_BUILD_LIBOTR_INTEROP_TESTS=ON` when configuring.

Use `QT_DEFAULT_MAJOR_VERSION=5` with `Qca3-qt5` for the Windows 7 profile. The
repository CI builds and tests the same source against both Qt 5 and Qt 6. The
Qt 6 Linux job consumes the published QCA 3.0.3 Ubuntu package directly, while
the Qt 5 compatibility job builds the same QCA 3.0.3 tag because no Qt 5 Linux
release package is currently published.

## License

qca-otr is licensed under the MIT License. Copyright (c) 2026 Sergei Ilinykh.
