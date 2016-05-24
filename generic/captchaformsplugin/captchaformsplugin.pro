CONFIG += release
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += captchaformsplugin.cpp \
    captchadialog.cpp \
    loader.cpp
FORMS += captchadialog.ui \
    options.ui
HEADERS += captchadialog.h \
    loader.h
RESOURCES += resources.qrc
QT += network
