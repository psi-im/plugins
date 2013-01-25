CONFIG += release
include(../../psiplugin.pri)

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
