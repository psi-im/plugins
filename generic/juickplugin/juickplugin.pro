CONFIG += release
include(../../psiplugin.pri)

SOURCES +=  http.cpp\
            juickplugin.cpp \
    juickjidlist.cpp

HEADERS += http.h \
    juickjidlist.h

QT += network

FORMS += \
    juickjidlist.ui
