isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += birthdayreminderplugin.cpp
FORMS += options.ui
