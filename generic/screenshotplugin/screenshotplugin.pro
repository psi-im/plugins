include(../../psiplugin.pri)
CONFIG += release
QT += network

DEPENDPATH += . qxt/core qxt/gui
INCLUDEPATH += . qxt/gui qxt/core

MOC_DIR = tmp/
OBJECTS_DIR = tmp/

HEADERS = screenshot.h \
    server.h \
    editserverdlg.h \
    screenshotoptions.h \
    toolbar.h \
    pixmapwidget.h \
    options.h \
    optionsdlg.h \
    optionswidget.h \
    iconset.h \
    controller.h \
    defines.h \
    proxysettingsdlg.h
SOURCES = screenshotplugin.cpp \
    screenshot.cpp \
    server.cpp \
    editserverdlg.cpp \
    screenshotoptions.cpp \
    toolbar.cpp \
    pixmapwidget.cpp \
    options.cpp \
    optionsdlg.cpp \
    optionswidget.cpp \
    iconset.cpp \
    controller.cpp \
    proxysettingsdlg.cpp
FORMS += optionswidget.ui \
    editserverdlg.ui \
    screenshot.ui \
    screenshotoptions.ui \
    optionsdlg.ui \
    proxysettingsdlg.ui
RESOURCES += screenshotplugin.qrc

#QXT
HEADERS  +=	qxt/core/qxtglobal.h \
		qxt/gui/qxtwindowsystem.h

SOURCES  +=	qxt/core/qxtglobal.cpp \
		qxt/gui/qxtwindowsystem.cpp

unix:!macx {
	CONFIG += X11
	SOURCES += qxt/gui/qxtwindowsystem_x11.cpp
}
macx {
	SOURCES += qxt/gui/qxtwindowsystem_mac.cpp

	HEADERS  += qxt/gui/qxtwindowsystem_mac.h

	QMAKE_LFLAGS += -framework Carbon -framework CoreFoundation
}
win32 {
	SOURCES += qxt/gui/qxtwindowsystem_win.cpp
}
