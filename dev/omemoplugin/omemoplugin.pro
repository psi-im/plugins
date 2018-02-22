isEmpty(PSISDK) {
    include(../../psiplugin.pri)
    include(../../../../conf.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
    include($$PSISDK/../psiplugin.pri)
}

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libsignal-protocol-c
}

CONFIG += crypto

LIBS += $$LINKAGE
QT += sql

RESOURCES = omemoplugin.qrc

HEADERS += src/configwidget.h
HEADERS += src/crypto.h
HEADERS += src/omemo.h
HEADERS += src/omemoplugin.h
HEADERS += src/signal.h
HEADERS += src/storage.h

SOURCES += src/configwidget.cpp
SOURCES += src/crypto.cpp
SOURCES += src/omemo.cpp
SOURCES += src/omemoplugin.cpp
SOURCES += src/signal.cpp
SOURCES += src/storage.cpp
