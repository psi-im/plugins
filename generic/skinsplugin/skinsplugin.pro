isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += skinsplugin.cpp \
    skin.cpp \
    optionsparser.cpp
FORMS += skinsplugin.ui \
    previewer.ui \
    getskinname.ui
HEADERS += skin.h \
    optionsparser.h
RESOURCES += skinsplugin.qrc
