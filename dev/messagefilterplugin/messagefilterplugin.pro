CONFIG += release
include(../../psiplugin.pri)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += options.cpp \
           messagefilter.cpp

HEADERS += options.h \
           messagefilter.h

FORMS   += options.ui

RESOURCES += resources.qrc
