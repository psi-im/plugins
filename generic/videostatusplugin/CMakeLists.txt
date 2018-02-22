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
set( CMAKE_AUTOMOC TRUE )

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
    include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
    message(FATAL_ERROR "No variables.cmake file found.
        Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

if( NOT WIN32 )
    add_definitions( -Ddbus -DHAVE_DBUS )
    find_package( X11 REQUIRED )
    set(EXTRA_LIBS ${X11_LIBRARIES})
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLUGINS_ROOT_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}
)
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
    ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
)

