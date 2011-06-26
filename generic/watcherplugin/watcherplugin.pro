CONFIG += release
include(../../psiplugin.pri)
SOURCES += watcherplugin.cpp \
    view.cpp \
    model.cpp \
    delegate.cpp \
    tooltip.cpp \
    watcheditem.cpp \
    edititemdlg.cpp
HEADERS += view.h \
    model.h \
    delegate.h \
    tooltip.h \
    watcheditem.h \
    edititemdlg.h
FORMS += options.ui \
    edititemdlg.ui
