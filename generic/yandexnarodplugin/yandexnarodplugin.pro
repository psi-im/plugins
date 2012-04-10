include(../../psiplugin.pri)
CONFIG += release
QT += network

HEADERS += yandexnarod.h \
    requestauthdialog.h \
    uploaddialog.h \
    yandexnarodsettings.h \
    yandexnarodmanage.h \
    yandexnarodnetman.h \
    options.h \
    uploadmanager.h \
    authmanager.h \
    common.h

SOURCES += yandexnarod.cpp \
    requestauthdialog.cpp \
    uploaddialog.cpp \
    yandexnarodsettings.cpp \
    yandexnarodmanage.cpp \
    yandexnarodnetman.cpp \
    options.cpp \
    uploadmanager.cpp \
    authmanager.cpp \
    common.cpp

RESOURCES += yandexnarod.qrc

FORMS += requestauthdialog.ui \
    uploaddialog.ui \
    yandexnarodsettings.ui \
    yandexnarodmanage.ui
