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
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += webkitwidgets
}

FORMS += \
    juickjidlist.ui \
    settings.ui

RESOURCES += resources.qrc
