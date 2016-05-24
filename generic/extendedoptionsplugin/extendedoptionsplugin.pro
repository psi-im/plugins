CONFIG += release
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += extendedoptionsplugin.cpp
RESOURCES += resources.qrc
