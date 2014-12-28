CONFIG += release
include(../../psiplugin.pri)
HEADERS += battleshipgameplugin.h \
	options.h \
	gamesessions.h \
	invitedialog.h \
    pluginwindow.h \
	boardview.h \
	boardmodel.h \
    gamemodel.h \
    boarddelegate.h
SOURCES += battleshipgameplugin.cpp \
	options.cpp \
	gamesessions.cpp \
	invitedialog.cpp \
    pluginwindow.cpp \
	boardview.cpp \
	boardmodel.cpp \
    gamemodel.cpp \
    boarddelegate.cpp
FORMS += options.ui \
	invitedialog.ui \
	invitationdialog.ui \
    pluginwindow.ui

RESOURCES += \
    battleshipgameplugin.qrc
