CONFIG += release
include(../../psiplugin.pri)

SOURCES += videostatusplugin.cpp
FORMS += options.ui

unix {
    CONFIG += X11
    QT += dbus
}
