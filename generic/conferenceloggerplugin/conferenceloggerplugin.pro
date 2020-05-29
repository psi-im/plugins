isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}


SOURCES += conferenceloggerplugin.cpp \
    typeaheadfind.cpp \
    viewer.cpp
HEADERS += typeaheadfind.h \
    viewer.h
