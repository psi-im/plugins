unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN videostatusplugin )
project(${PLUGIN} LANGUAGES CXX)
cmake_minimum_required(VERSION 3.10.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
set( CMAKE_AUTOMOC TRUE )

get_filename_component(ABS_PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(PLUGINS_ROOT_DIR "${ABS_PLUGINS_ROOT_DIR}" CACHE STRING "Plugins root path. Path where include directory placed")
set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_LIST_DIR}/cmake
    ${PLUGINS_ROOT_DIR}/cmake/modules
)
find_package(PsiPluginsApi REQUIRED)
include(${PsiPluginsApi_DIR}/variables.cmake)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${PsiPluginsApi_INCLUDE_DIR}
)
if(NOT PLUGINS_INSTALL_PATH)
    set(PLUGINS_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}")
endif()

if( NOT WIN32 AND NOT HAIKU )
    add_definitions( -Ddbus -DHAVE_DBUS )
    find_package( X11 REQUIRED )
    set(EXTRA_LIBS ${X11_LIBRARIES})
endif()

if( NOT WIN32 AND NOT HAIKU )
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
find_package( Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Widgets Xml REQUIRED )
set(QT_DEPLIBS
    Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
    Qt${QT_DEFAULT_MAJOR_VERSION}::Xml
)
if( UNIX AND NOT( APPLE OR CYGWIN OR HAIKU ) )
    if(QT_DEFAULT_MAJOR_VERSION GREATER_EQUAL 6)
        find_package( Qt6 COMPONENTS Gui REQUIRED )
        list(APPEND QT_DEPLIBS Qt6::Gui)
    else()
        #find_package( Qt5 COMPONENTS X11Extras REQUIRED )
        #list(APPEND QT_DEPLIBS Qt5::X11Extras)
    endif()
    find_package( Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS DBus REQUIRED )
    find_package( XCB REQUIRED )
    add_definitions(
        -DHAVE_DBUS
    )
    list( APPEND QT_DEPLIBS
        Qt${QT_DEFAULT_MAJOR_VERSION}::DBus
        ${XCB_LIBRARY}
    )
endif()

qt_wrap_ui(UIS ${_UIS})

if(WIN32)
    set(LIB_TYPE "MODULE")
else()
    set(LIB_TYPE "SHARED")
endif()

add_library(
    ${PLUGIN}
    ${LIB_TYPE}
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
