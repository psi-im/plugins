CONFIG += release
TARGET = redirectorplugin

exists($$sdkdir) {
    include($$sdkdir/psiplugin.pri)
} else {
    include(../../psiplugin.pri)
}

SOURCES += redirectorplugin.cpp
HEADERS += redirectorplugin.h
FORMS += options.ui
