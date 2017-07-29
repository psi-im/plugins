isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += options.cpp \
           messagefilter.cpp

HEADERS += options.h \
           messagefilter.h

FORMS   += options.ui

RESOURCES += resources.qrc
