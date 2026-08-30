<!--
SPDX-FileCopyrightText: 2026 Sergei Ilinykh
SPDX-License-Identifier: MIT
-->

# Native OTR completion plan

The native qca-otr protocol stack and Psi backend switch are complete. libotr
4.1.1 is retained only as an optional interoperability oracle; the normal plugin
runtime is qca-otr/QCA. This file records the implementation stages and the
remaining external/manual validation items.

## 1. Fragmentation and OTRv3 instance routing — completed

- transport armor/dearmor (`?OTR:base64.`)
- exact OTRv3 fragment format and reassembly
- bounded fragment buffering and invalid-sequence reset
- sender/receiver instance inspection before protocol dispatch
- per-remote-instance AKE/Data session routing
- handling of receiver instance 0 for initial D-H Commit
- rejection/reporting of packets addressed to another local instance
- broadcast/master AKE cloned independently for every answering instance
- libotr interoperability for fragmented AKE and encrypted data messages

## 2. Negotiation, message envelope and control TLVs — completed

- OTR query generation/parsing and version negotiation
- OTRv3 whitespace-tag detection/generation for opportunistic policy
- manual/opportunistic/always policy behavior
- encrypted plaintext/TLV envelope
- disconnected and symmetric-key TLVs
- protocol errors/unreadable handling
- libotr interoperability coverage

## 3. SMP — completed

- standalone OTRv3 SMP state machine and zero-knowledge proofs
- libotr-compatible combined-secret derivation
- secret material retained in `QCA::SecureArray`
- complete question/answer, abort and result event flow
- libotr interoperability coverage

## 4. Persistence and migration — completed

- libotr-compatible `otr.keys`, `otr.fingerprints` and `otr.instags`
- exact migration of existing Psi identities and trust data
- private-key serialization retained in `QCA::SecureArray`
- private-key disk I/O through `QCA::SecureFile`
- explicit application-owned opaque protocol id
- Linux Qt5/Qt6 and Windows Qt5/Win7-baseline coverage

## 5. Native Psi adapter and default dependency switch — completed

The normal OTR plugin now uses qca-otr as its only runtime OTR backend rather
than maintaining a long-lived dual-backend abstraction.

- application-facing qca-otr `ProfileStore` handles identity generation,
  instance tags and fingerprint/trust management
- `psiotr::Fingerprint` is an owning value type with no pointer into libotr
- `OtrInternal` is implemented with qca-otr `OtrSession` + `ProfileStore` while
  preserving the existing `OtrMessaging`/UI-facing API
- native negotiation, encryption, disconnect, session state and SMP events map
  to the existing Psi callbacks
- historical Psi persistence id `prpl-jabber` exists only in the Psi adapter;
  qca-otr treats protocol ids as opaque application-owned metadata
- `LIBOTR`, `LIBGCRYPT` and `LIBGPGERROR` are absent from the normal plugin
  dependency graph; libotr remains only in optional qca-otr oracle tests
- Qt5/Windows 7 and Qt6 coverage exercise the native path

Exit criterion met: the normal OTR plugin builds and operates on qca-otr/QCA
without linking libotr, libgcrypt or libgpg-error, while existing profile formats
remain compatible.

## 6. Legacy cleanup and final integration — code cleanup completed

The native backend is the only supported runtime path. The cleanup removes
legacy implementation baggage without removing wire/profile interoperability
with older libotr-based clients.

Completed in the cleanup branch:

- removed obsolete libotr callbacks, `otrlextensions` and legacy dependency
  finder modules
- removed the libtidy runtime/build dependency and replaced HTML normalization
  with Qt `QTextDocument`
- removed raw libotr-backed fingerprint pointer semantics
- simplified `OtrMessaging` and related adapter plumbing around the native model
- corrected XEP-0364 whitespace advertisement/discovery to the actually
  implemented OTRv3 version
- preserved old plugin GPL provenance/license headers where substantial legacy
  code remains
- licensed the independently implemented qca-otr library under MIT and added
  Doxygen contracts to its public API
- kept Qt5/Qt6/Linux and Qt5/Windows 7 native CI coverage

Still useful as external/manual release validation rather than implementation
work:

- run a real new-Psi <-> old-Psi/libotr conversation smoke test
- verify migration with a representative existing user profile before and after
  the backend switch

The code-level exit criterion is met: the OTR plugin contains no normal
runtime/build dependency on libotr/libgcrypt/libgpg-error or libtidy and remains
covered by libotr interoperability tests for wire and persistence formats.
