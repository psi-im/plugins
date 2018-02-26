isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libsignal-protocol-c
}

load(configure)
qtCompileTest(oldSignal):DEFINES += OLD_SIGNAL

CONFIG += crypto c++11

LIBS += $$LINKAGE
QT += sql

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
    src/crypto.cpp \
    src/omemo.cpp \
    src/omemoplugin.cpp \
    src/signal.cpp \
    src/storage.cpp
