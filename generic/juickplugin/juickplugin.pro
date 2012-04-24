CONFIG += release
include(../../psiplugin.pri)

SOURCES +=  http.cpp\
            juickplugin.cpp \
    juickjidlist.cpp \
    juickparser.cpp

HEADERS += http.h \
    juickjidlist.h \
    juickparser.h

QT += network webkit

FORMS += \
    juickjidlist.ui \
    settings.ui
