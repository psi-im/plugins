isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
HEADERS += clientswitcherplugin.h \
    accountsettings.h \
    viewer.h \
    typeaheadfind.h
SOURCES += clientswitcherplugin.cpp \
    accountsettings.cpp \
    viewer.cpp \
    typeaheadfind.cpp
RESOURCES += resources.qrc
FORMS += options.ui
