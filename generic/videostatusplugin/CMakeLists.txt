unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN videostatusplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
set( CMAKE_AUTOMOC TRUE )

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")
set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PLUGINS_ROOT_DIR}/cmake/modules
)
find_package(PsiPluginsApi REQUIRED)
include(${PsiPluginsApi_DIR}/variables.cmake)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${PsiPluginsApi_INCLUDE_DIR}
)
if(NOT PLUGINS_INSTALL_PATH)
    set(PLUGINS_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if( NOT WIN32 )
    add_definitions( -Ddbus -DHAVE_DBUS )
    find_package( X11 REQUIRED )
    set(EXTRA_LIBS ${X11_LIBRARIES})
endif()

if( NOT WIN32 )
    set( x11_SRCS
        x11info.cpp
        )
    set( x11_HDRS
        x11info.h
    )
endif()
set( _SRCS
    ${PLUGIN}.cpp
    ${x11_SRCS}
)
set( _UIS
    options.ui
)
set( _RSCS
    resources.qrc
)

find_package( Qt5 COMPONENTS Widgets Xml REQUIRED )
set(QT_DEPLIBS
    Qt5::Widgets
    Qt5::Xml
)
IF( UNIX AND NOT( APPLE OR CYGWIN ) )
    find_package( Qt5 COMPONENTS DBus X11Extras REQUIRED )
    find_package( XCB REQUIRED )
    add_definitions(
        -DHAVE_DBUS
    )
    set(QT_DEPLIBS
        ${QT_DEPLIBS}
        Qt5::DBus
        Qt5::X11Extras
        ${XCB_LIBRARY}
    )
ENDIF()
qt5_wrap_ui(UIS ${_UIS})
qt5_add_resources(RSCS ${_RSCS})

add_library(
    ${PLUGIN}
    MODULE
    ${_SRCS}
    ${UIS}
    ${RSCS}
)

target_link_libraries(
    ${PLUGIN}
    ${QT_DEPLIBS}
    ${EXTRA_LIBS}
)

install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION
    ${PLUGINS_INSTALL_PATH}
    RUNTIME DESTINATION
    ${PLUGINS_INSTALL_PATH}
)
