CONFIG += release
TARGET = stopspamplugin
include(../../psiplugin.pri)
SOURCES += stopspamplugin.cpp \
    view.cpp \
    model.cpp \
    viewer.cpp \
    typeaheadfind.cpp \
    deferredstanzasender.cpp
HEADERS += view.h \
    model.h \
    viewer.h \
    typeaheadfind.h \
    deferredstanzasender.h
FORMS += options.ui
RESOURCES += resources.qrc
