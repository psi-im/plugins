# Native OTR completion plan

The native qca-otr work is completed in protocol layers so each stage can be
validated against libotr 4.1.1 before the Psi plugin switches backend.

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

## 5. Native Psi adapter and default dependency switch

The primary objective of this stage is not a long-lived dual-backend abstraction.
The normal OTR plugin build must stop depending on libotr, libgcrypt and
libgpg-error as soon as the native adapter is feature-complete enough to replace
`OtrInternal`.

- add an application-facing qca-otr `ProfileStore` for identity generation,
  instance tags and fingerprint/trust management
- make `psiotr::Fingerprint` an owning value type with no pointer into libotr
- replace the implementation of `OtrInternal` with qca-otr `OtrSession` +
  `ProfileStore`, preserving the existing `OtrMessaging`/UI-facing API
- map native negotiation, encryption, disconnect, session state and SMP events
  to the existing Psi callbacks
- preserve historical Psi persistence id `prpl-jabber` only in the Psi adapter
- remove `LIBOTR`, `LIBGCRYPT` and `LIBGPGERROR` from the normal plugin CMake
  dependency graph; libotr/libgcrypt remain only in optional qca-otr oracle tests
- keep Qt5/Windows 7 and Qt6 coverage on the native path

Exit criterion: the normal OTR plugin builds and operates on qca-otr/QCA without
linking libotr, libgcrypt or libgpg-error. Existing profiles remain compatible.

## 6. Legacy cleanup and final integration

There is no intention to return to the old libotr backend after the native switch.
Once native behavior and interoperability smoke tests are green:

- delete obsolete libotr callbacks, headers, `otrlextensions` and compatibility
  implementation code
- remove any temporary migration-only adapter seams
- simplify `OtrMessaging` and related UI plumbing around the native model
- run real new-Psi <-> old-Psi/libotr interoperability smoke tests
- verify migration using an existing profile before and after the backend switch
- remove remaining dead build configuration and documentation

Exit criterion: the OTR plugin contains no runtime/build dependency or dead
adapter code for libotr/libgcrypt/libgpg-error while remaining interoperable with
older clients over the wire and preserving existing user data.
