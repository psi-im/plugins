isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += pepchangenotifyplugin.cpp
FORMS += options.ui
