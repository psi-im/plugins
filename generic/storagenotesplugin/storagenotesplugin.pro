isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}
SOURCES += storagenotesplugin.cpp \
    notes.cpp \
    editnote.cpp \
    tagsmodel.cpp \
    notesviewdelegate.cpp \
    notescontroller.cpp
FORMS += notes.ui \
    editnote.ui
HEADERS += notes.h \
    editnote.h \
    storagenotesplugin.h \
    tagsmodel.h \
    notesviewdelegate.h \
    notescontroller.h

    storagenotesplugin.qrc
