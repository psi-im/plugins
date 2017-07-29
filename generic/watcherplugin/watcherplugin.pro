isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += watcherplugin.cpp \
    view.cpp \
    model.cpp \
    delegate.cpp \
    watcheditem.cpp \
    edititemdlg.cpp
HEADERS += view.h \
    model.h \
    delegate.h \
    watcheditem.h \
    edititemdlg.h
FORMS += options.ui \
    edititemdlg.ui
RESOURCES += resources.qrc
