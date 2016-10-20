CONFIG += release

QT += network

isEmpty(PSISDK) {
    include(../../psiplugin.pri)
} else {
    include($$PSISDK/plugins/psiplugin.pri)
}

equals(QT_MAJOR_VERSION, 4) {
   CONFIG += link_pkgconfig
   PKGCONFIG += QJson
}
    
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += ripperccoptions.cpp \
           rippercc.cpp \
           qjsonwrapper.cpp

HEADERS += ripperccoptions.h \
           rippercc.h \
           qjsonwrapper.h

FORMS   += ripperccoptions.ui

RESOURCES += resources.qrc
