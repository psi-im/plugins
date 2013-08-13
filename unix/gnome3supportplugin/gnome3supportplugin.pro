!mac {
CONFIG += release
include(../../psiplugin.pri)
SOURCES += gnome3supportplugin.cpp
QT += dbus
RESOURCES += resources.qrc
}
