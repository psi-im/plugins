CONFIG += release
QT += network
include(../../psiplugin.pri)

SOURCES +=  pstoplugin.cpp \
    preferenceswidget.cpp

FORMS += \
    preferences.ui

HEADERS += \
    preferenceswidget.h \
    pstoplugin.h

RESOURCES += resources.qrc
