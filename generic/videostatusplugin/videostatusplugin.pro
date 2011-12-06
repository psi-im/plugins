!mac {
CONFIG += release
include(../../psiplugin.pri)

SOURCES += videostatusplugin.cpp
FORMS += options.ui
}
unix: ! mac {
    CONFIG += X11
    QT += dbus
}
