CONFIG += release
include(../../psiplugin.pri)

unix {
    SOURCES += videostatusplugin.cpp
    FORMS += options.ui
    CONFIG += X11
    QT += dbus
}
win32 {
    SOURCES += videostatuspluginwin.cpp
    FORMS += winoptions.ui
}
