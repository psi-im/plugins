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

Exit criterion: fragmented and unfragmented AKE/Data traffic interoperates in
both directions with libotr 4.1.1, including multiple remote instances.

## 2. Negotiation, message envelope and control TLVs — active

The current Psi plugin does not start OTR by sending a D-H Commit directly: it
sends `otrl_proto_default_query_msg()` and also delegates ordinary outgoing
messages to libotr policy handling. The native backend therefore needs the
transport-facing protocol envelope before plugin integration.

- OTR query generation/parsing and version negotiation (`?OTRv3?` and combined queries)
- OTRv3 whitespace-tag detection/generation for opportunistic policy
- policy behavior needed by the current plugin (manual/opportunistic/always)
- encrypted framing as `plaintext || NUL || TLVs`
- strict TLV codec with duplicate/length/truncation handling
- disconnected TLV and FINISHED-state semantics
- symmetric-key TLV / extra-key plumbing used by libotr-compatible applications
- protocol error parsing/generation and unreadable/error result events
- preserve per-instance routing for every control path

Exit criterion: start/disconnect/error and encrypted TLV traffic round-trip with
libotr 4.1.1 without relying on libotr message wrappers.

## 3. SMP

- OTRv3 SMP state machine
- initiation with and without a question
- response, progress and expected-message tracking
- success, failure, abort, cheating/unexpected-message handling
- SMP TLVs over the normal encrypted Data Message path
- bidirectional libotr 4.1.1 interoperability

Exit criterion: qca-otr and libotr agree on every SMP outcome and event sequence.

## 4. Persistence and migration

- read existing `otr.keys` DSA identities without regenerating them
- read/write `otr.fingerprints` including arbitrary libotr trust strings
- read/write `otr.instags`
- preserve account/protocol/user mapping exactly as the current plugin expects
- fixtures produced by libotr 4.1.1 plus fixtures from real/current Psi files
- deterministic round-trip tests and malformed/partial-file handling
- atomic writes so a crash cannot destroy identity or trust databases

Exit criterion: an existing Psi OTR profile can switch backend without identity,
fingerprint/trust or instance-tag loss, and can still be read by the old backend
where the file format is intended to remain compatible.

## 5. Complete `OtrSession` and Psi adapter

- extend the routed `OtrSession` already introduced by stage 1 with negotiation,
  TLVs, disconnect and SMP
- final API: `start`, `processIncoming`, `sendMessage`, `disconnect`, `startSmp`,
  `respondSmp`, `abortSmp`
- expose peer identity/fingerprint/trust and per-instance secure state
- map qca-otr events to the existing plugin state-change, notification and SMP UI
- retain current fingerprint/key management and options behavior
- register the native backend through Psi's current encryption interface
- keep the old libotr backend available behind a temporary build/runtime switch
  until migration and interoperability smoke tests are complete

Exit criterion: the existing plugin UI and encryption interface operate on
qca-otr with no libotr calls on the native path.

## 6. Backend switch and dependency cleanup

- make qca-otr the default OTR backend
- Qt5/Windows 7 and Qt6 CI coverage
- real new-Psi <-> old-Psi/libotr interoperability smoke tests
- migration test using an existing profile before and after backend switch
- remove the temporary libotr backend switch
- remove libotr, libgcrypt and libgpg-error runtime/build dependencies
- remove dead compatibility adapter code

Exit criterion: Psi builds and runs OTR without libotr/libgcrypt/libgpg-error,
while preserving interoperability, Win7/Qt5 support and existing user data.
