CONFIG += release
RESOURCES = chessplugin.qrc
include(../../psiplugin.pri)
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += chessplugin.cpp \
    figure.cpp \
    boardmodel.cpp \
    mainwindow.cpp \
    boardview.cpp \
    boarddelegate.cpp \
    invitedialog.cpp
HEADERS += figure.h \
    boardmodel.h \
    mainwindow.h \
    boardview.h \
    boarddelegate.h \
    invitedialog.h \
    request.h
FORMS += mainwindow.ui \
    options.ui \
    invitedialog.ui \
    invitationdialog.ui
