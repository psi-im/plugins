include(../../psiplugin.pri)

CONFIG += release
LIBS += -lotr -ltidy -lgcrypt -lgpg-error
RESOURCES = otrplugin.qrc
unix {
	INCLUDEPATH += /usr/include/tidy
}
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += concurrent
}

HEADERS += src/psiotrplugin.h
HEADERS += src/otrmessaging.h
HEADERS += src/otrinternal.h
HEADERS += src/psiotrconfig.h
HEADERS += src/psiotrclosure.h
HEADERS += src/htmltidy.h
HEADERS += src/otrlextensions.h

SOURCES += src/psiotrplugin.cpp
SOURCES += src/otrmessaging.cpp
SOURCES += src/otrinternal.cpp
SOURCES += src/psiotrconfig.cpp
SOURCES += src/psiotrclosure.cpp
SOURCES += src/htmltidy.cpp
SOURCES += src/otrlextensions.c
