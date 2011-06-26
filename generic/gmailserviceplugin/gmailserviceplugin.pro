CONFIG += release
include(../../psiplugin.pri)
SOURCES += gmailserviceplugin.cpp \
    accountsettings.cpp \
    common.cpp \
    actionslist.cpp \
    viewmaildlg.cpp
FORMS += options.ui \
    viewmaildlg.ui
HEADERS += gmailserviceplugin.h \
    accountsettings.h \
    common.h \
    actionslist.h \
    viewmaildlg.h
RESOURCES += gmailserviceplugin.qrc
