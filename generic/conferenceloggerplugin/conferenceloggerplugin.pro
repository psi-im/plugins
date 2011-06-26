CONFIG += release
RESOURCES = conferenceloggerplugin.qrc
include(../../psiplugin.pri)
SOURCES += conferenceloggerplugin.cpp \
    typeaheadfind.cpp \
    viewer.cpp
HEADERS += typeaheadfind.h \
    viewer.h
