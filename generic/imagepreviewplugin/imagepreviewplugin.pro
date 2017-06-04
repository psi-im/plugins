CONFIG += release
QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    qtHaveModule(webengine) {
        QT += webengine webenginewidgets
        DEFINES += HAVE_WEBENGINE
    }
    qtHaveModule(webkit) {
        QT += webkit webkitwidgets
        DEFINES += HAVE_WEBKIT
    }
}
else {
    QT += webkit
    DEFINES += HAVE_WEBKIT
}

isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

RESOURCES = imagepreviewplugin.qrc

SOURCES += imagepreviewplugin.cpp \
           ScrollKeeper.cpp
