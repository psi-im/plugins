CONFIG += release
TARGET = stopspamplugin
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
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
