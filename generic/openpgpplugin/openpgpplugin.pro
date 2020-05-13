isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

DEPENDPATH  += $$PWD/src
INCLUDEPATH += $$PWD/src

SOURCES += $$PWD/src/*.cpp
HEADERS += $$PWD/src/*.h
FORMS   += $$PWD/src/*.ui

RESOURCES += $$PWD/resources/*.qrc

win32:LIBS += -ladvapi32
