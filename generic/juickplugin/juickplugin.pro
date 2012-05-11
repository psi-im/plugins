CONFIG += release
include(../../psiplugin.pri)

SOURCES += \
            juickplugin.cpp \
    juickjidlist.cpp \
    juickparser.cpp \
    juickdownloader.cpp

HEADERS += \
    juickjidlist.h \
    juickparser.h \
    juickdownloader.h \
    defines.h \
    juickplugin.h

QT += network webkit

FORMS += \
    juickjidlist.ui \
    settings.ui
