CONFIG += release
include(../../psiplugin.pri)

SOURCES += options.cpp \
           model.cpp \
           gpgprocess.cpp \
           addkeydlg.cpp \
           gnupg.cpp

HEADERS += options.h \
           model.h \
           gpgprocess.h \
           addkeydlg.h \
           gnupg.h

FORMS   += options.ui \
           addkeydlg.ui
