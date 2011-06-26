CONFIG += release
include(../../psiplugin.pri)
SOURCES += jabberdiskplugin.cpp \
    model.cpp \
    jabberdiskcontroller.cpp \
    jd_commands.cpp \
    jd_item.cpp \
    jd_mainwin.cpp \
    jd_view.cpp
FORMS += options.ui \
    jd_mainwin.ui
HEADERS += jabberdiskplugin.h \
    model.h \
    jabberdiskcontroller.h \
    jd_commands.h \
    jd_item.h \
    jd_mainwin.h \
    jd_view.h
