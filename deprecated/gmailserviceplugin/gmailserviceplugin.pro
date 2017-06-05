CONFIG += release
isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
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
