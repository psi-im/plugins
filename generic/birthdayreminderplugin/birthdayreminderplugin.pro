CONFIG += release
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
RESOURCES = birthdayreminderplugin.qrc
SOURCES += birthdayreminderplugin.cpp
FORMS += options.ui
