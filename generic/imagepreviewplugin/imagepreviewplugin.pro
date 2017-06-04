CONFIG += release
QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    greaterThan(QT_MINOR_VERSION, 6) {
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

isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

RESOURCES = imagepreviewplugin.qrc

SOURCES += imagepreviewplugin.cpp \
           ScrollKeeper.cpp
