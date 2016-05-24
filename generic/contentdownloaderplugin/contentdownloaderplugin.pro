# -------------------------------------------------
# Project created by QtCreator 2010-08-20T17:55:47
# -------------------------------------------------
QT += network xml
# CONFIG += debug
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += contentdownloader.cpp \
    cditemmodel.cpp \
    form.cpp \
    contentitem.cpp
HEADERS += contentdownloader.h \
    cditemmodel.h \
    form.h \
    contentitem.h
RESOURCES += resources.qrc
FORMS += form.ui 
OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

