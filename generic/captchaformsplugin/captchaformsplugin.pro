CONFIG += release
include(../../psiplugin.pri)
SOURCES += captchaformsplugin.cpp \
    captchadialog.cpp \
    loader.cpp
FORMS += captchadialog.ui \
    options.ui
HEADERS += captchadialog.h \
    loader.h
QT += network
