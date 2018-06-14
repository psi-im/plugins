isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libsignal-protocol-c libcrypto
} else {
    LIBS += $$LINKAGE
    #LIBS += -lsignal-protocol-c
}

load(configure)
qtCompileTest(oldSignal):DEFINES += OLD_SIGNAL

CONFIG += c++11
QT += sql network

RESOURCES = omemoplugin.qrc

HEADERS += \
    src/configwidget.h \
    src/crypto.h \
    src/omemo.h \
    src/omemoplugin.h \
    src/signal.h \
    src/storage.h

SOURCES += \
    src/configwidget.cpp \
    src/crypto_ossl.cpp \
    src/crypto_common.cpp \
    src/omemo.cpp \
    src/omemoplugin.cpp \
    src/signal.cpp \
    src/storage.cpp
