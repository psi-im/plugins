CONFIG += release
RESOURCES = attentionplugin.qrc

isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += attentionplugin.cpp
FORMS += options.ui
