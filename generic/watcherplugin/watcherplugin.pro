CONFIG += release
include(../../psiplugin.pri)
SOURCES += watcherplugin.cpp \
    view.cpp \
    model.cpp \
    delegate.cpp \
    watcheditem.cpp \
    edititemdlg.cpp
HEADERS += view.h \
    model.h \
    delegate.h \
    watcheditem.h \
    edititemdlg.h
FORMS += options.ui \
    edititemdlg.ui
RESOURCES += resources.qrc
