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

QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    greaterThan(QT_MINOR_VERSION, 5) {
        QT += webengine webenginewidgets
        DEFINES += HAVE_WEBENGINE
    }
    else {
        QT += webkit webkitwidgets
        DEFINES += HAVE_WEBKIT
    }
}
else {
    QT += webkit
    DEFINES += HAVE_WEBKIT
}

FORMS += \
    juickjidlist.ui \
    settings.ui

RESOURCES += resources.qrc
