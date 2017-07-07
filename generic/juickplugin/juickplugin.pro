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
    contains(psi_features, qtwebengine) {
        QT += webengine webenginewidgets
    }
    contains(psi_features, qtwebkit) {
        QT += webkit webkitwidgets
    }
}
else {
    contains(psi_features, qtwebkit) {
        QT += webkit
    }
}

FORMS += \
    juickjidlist.ui \
    settings.ui

RESOURCES += resources.qrc
