isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

RESOURCES = imageplugin.qrc

SOURCES += imageplugin.cpp \

