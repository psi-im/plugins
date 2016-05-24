CONFIG += release

isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += extendedmenuplugin.cpp

FORMS += \
    options.ui

OTHER_FILES +=

RESOURCES += \
    extendedmenuplugin.qrc
