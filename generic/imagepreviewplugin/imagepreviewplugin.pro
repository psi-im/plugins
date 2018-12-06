isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

QT += network
RESOURCES = imagepreviewplugin.qrc

SOURCES += imagepreviewplugin.cpp \
           ScrollKeeper.cpp
