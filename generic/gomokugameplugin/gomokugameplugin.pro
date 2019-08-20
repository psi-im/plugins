isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
HEADERS += gomokugameplugin.h \
    pluginwindow.h \
    boardview.h \
    boardmodel.h \
    boarddelegate.h \
    gameelement.h \
    invitedialog.h \
    gamesessions.h \
    common.h \
    options.h \
    gamemodel.h
SOURCES += gomokugameplugin.cpp \
    pluginwindow.cpp \
    boardview.cpp \
    boardmodel.cpp \
    boarddelegate.cpp \
    gameelement.cpp \
    invitedialog.cpp \
    gamesessions.cpp \
    options.cpp \
    gamemodel.cpp
FORMS += pluginwindow.ui \
    invitedialog.ui \
    invitationdialog.ui \
    options.ui
RESOURCES += gomokugameplugin.qrc
