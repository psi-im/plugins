include(../../psiplugin.pri)
CONFIG += release
QT += network

HEADERS = screenshot.h \
    server.h \
    editserverdlg.h \
    screenshotoptions.h \
    toolbar.h \
    pixmapwidget.h \
    options.h \
    optionsdlg.h \
    optionswidget.h \
    iconset.h \
    controller.h \
    defines.h \
    proxysettingsdlg.h
SOURCES = screenshotplugin.cpp \
    screenshot.cpp \
    server.cpp \
    editserverdlg.cpp \
    screenshotoptions.cpp \
    toolbar.cpp \
    pixmapwidget.cpp \
    options.cpp \
    optionsdlg.cpp \
    optionswidget.cpp \
    iconset.cpp \
    controller.cpp \
    proxysettingsdlg.cpp
FORMS += optionswidget.ui \
    editserverdlg.ui \
    screenshot.ui \
    screenshotoptions.ui \
    optionsdlg.ui \
    proxysettingsdlg.ui
RESOURCES += screenshotplugin.qrc
