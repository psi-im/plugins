# Native OTR completion plan

The native qca-otr work is completed in protocol layers so each stage can be
validated against libotr 4.1.1 before the Psi plugin switches backend.

## 1. Fragmentation and OTRv3 instance routing

- transport armor/dearmor (`?OTR:base64.`)
- exact OTRv3 fragment format and reassembly
- bounded fragment buffering and invalid-sequence reset
- sender/receiver instance inspection before protocol dispatch
- per-remote-instance AKE/Data session routing
- handling of receiver instance 0 for initial D-H Commit
- rejection/reporting of packets addressed to another local instance
- libotr interoperability for fragmented AKE and encrypted data messages

Exit criterion: fragmented and unfragmented AKE/Data traffic interoperates in
both directions with libotr 4.1.1, including multiple remote instances.

## 2. TLV framing and SMP

- encrypted plaintext + NUL + TLV framing
- strict TLV codec
- disconnected / symmetric-key TLVs needed by the plugin
- OTRv3 SMP state machine, with and without a question
- SMP success, failure, abort and unexpected-message handling
- bidirectional libotr 4.1.1 SMP interoperability

Exit criterion: qca-otr and libotr agree on all SMP outcomes and encrypted TLV
transport.

## 3. Persistence and migration

- read existing `otr.keys` identities without regenerating them
- read/write `otr.fingerprints` including trust state
- read/write `otr.instags`
- migration fixtures from the current libotr backend
- atomic writes and malformed-file handling

Exit criterion: an existing Psi OTR profile can switch backend without identity,
fingerprint/trust or instance-tag loss.

## 4. Unified session API and Psi adapter

- combine AKE, Data, fragmentation, routing and SMP behind `OtrSession`
- `start`, `processIncoming`, `sendMessage`, `disconnect`, `startSmp`,
  `respondSmp`, `abortSmp`
- map qca-otr events to the existing plugin UI and notification surface
- retain existing fingerprint/trust management and options behavior
- register the native backend through Psi's current encryption interface

Exit criterion: the existing plugin UI operates on qca-otr without libotr calls.

## 5. Backend switch and dependency cleanup

- make qca-otr the default OTR backend
- Qt5/Windows 7 and Qt6 CI coverage
- real old-Psi/libotr interoperability smoke tests
- remove libotr, libgcrypt and libgpg-error runtime/build dependencies
- remove dead compatibility adapter code after migration coverage is green

Exit criterion: Psi builds and runs OTR without libotr/libgcrypt/libgpg-error,
while preserving interoperability and existing user data.
