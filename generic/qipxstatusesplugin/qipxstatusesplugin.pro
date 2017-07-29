isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += qipxstatusesplugin.cpp
RESOURCES += resources.qrc
