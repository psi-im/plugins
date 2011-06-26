CONFIG += release
include(../../psiplugin.pri)
HEADERS += clientswitcherplugin.h \
    accountsettings.h \
    viewer.h \
    typeaheadfind.h
SOURCES += clientswitcherplugin.cpp \
    accountsettings.cpp \
    viewer.cpp \
    typeaheadfind.cpp
FORMS += options.ui
