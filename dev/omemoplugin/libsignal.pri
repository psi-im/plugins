INCLUDEPATH += lib/libsignal-protocol-c/src
INCLUDEPATH += lib/libsignal-protocol-c/src/curve25519/ed25519
INCLUDEPATH += lib/libsignal-protocol-c/src/curve25519/ed25519/nacl_includes
INCLUDEPATH += lib/libsignal-protocol-c/src/curve25519/ed25519/additions
INCLUDEPATH += lib/libsignal-protocol-c/src/curve25519/ed25519/additions/generalized

SOURCES += lib/libsignal-protocol-c/src/*.c
SOURCES += lib/libsignal-protocol-c/src/protobuf-c/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/ed25519/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/ed25519/additions/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/ed25519/additions/generalized/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/ed25519/nacl_sha512/*.c
SOURCES += lib/libsignal-protocol-c/src/curve25519/ed25519/tests/*.c