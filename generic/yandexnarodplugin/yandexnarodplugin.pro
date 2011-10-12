include(../../psiplugin.pri)
CONFIG += release
QT += network webkit

HEADERS += yandexnarod.h \
    requestauthdialog.h \
    uploaddialog.h \
    yandexnarodsettings.h \
    yandexnarodmanage.h \
    yandexnarodnetman.h \
    proxy.h

SOURCES += yandexnarod.cpp \
    requestauthdialog.cpp \
    uploaddialog.cpp \
    yandexnarodsettings.cpp \
    yandexnarodmanage.cpp \
    yandexnarodnetman.cpp \
    proxy.cpp

RESOURCES += yandexnarod.qrc

FORMS += requestauthdialog.ui \
    uploaddialog.ui \
    yandexnarodsettings.ui \
    yandexnarodmanage.ui
