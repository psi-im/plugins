isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += options.cpp \
           model.cpp \
           gpgprocess.cpp \
           addkeydlg.cpp \
           gnupg.cpp \
           lineeditwidget.cpp \
           datewidget.cpp

HEADERS += options.h \
           model.h \
           gpgprocess.h \
           addkeydlg.h \
           gnupg.h \
           lineeditwidget.h \
           datewidget.h

FORMS   += options.ui \
           addkeydlg.ui

RESOURCES += resources.qrc

win32:LIBS += -ladvapi32
