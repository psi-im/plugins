CONFIG += release
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += icqdieplugin.cpp
FORMS += icqdieoptions.ui
RESOURCES += resources.qrc
