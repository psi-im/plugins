isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

RESOURCES = attentionplugin.qrc

SOURCES += attentionplugin.cpp
FORMS += options.ui
