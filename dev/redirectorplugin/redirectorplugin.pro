CONFIG += release
TARGET = redirectplugin

exists($$sdkdir) {
    include($$sdkdir/psiplugin.pri)
} else {
    include(../../psiplugin.pri)
}
SOURCES += redirectplugin.cpp
HEADERS +=
FORMS += options.ui
