!mac {
CONFIG += release
include(../../psiplugin.pri)

RESOURCES += resources.qrc

SOURCES += videostatusplugin.cpp
FORMS += options.ui
}
unix: ! mac {
    CONFIG += X11
    QT += dbus
}
