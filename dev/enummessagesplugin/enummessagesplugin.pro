CONFIG += release
include(../../psiplugin.pri)

SOURCES += \
    enummessagesplugin.cpp

HEADERS += \
    defines.h \
    enummessagesplugin.h


RESOURCES += resources.qrc

OTHER_FILES += \
    changelog.txt

FORMS += \
    options.ui
