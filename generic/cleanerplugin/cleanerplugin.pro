isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
RESOURCES = cleanerplugin.qrc
SOURCES += cleanerplugin.cpp \
    cleaner.cpp \
    common.cpp \
    models.cpp \
    viewers.cpp \
    optionsparser.cpp
HEADERS += cleaner.h \
    cleanerplugin.h \
    common.h \
    models.h \
    viewers.h \
    optionsparser.h
FORMS += cleaner.ui \
    clearingtab.ui
