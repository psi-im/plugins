isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

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

QT += network widgets

FORMS += \
    juickjidlist.ui \
    settings.ui

