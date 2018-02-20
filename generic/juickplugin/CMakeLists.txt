unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN juickplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
endif()
set( CMAKE_AUTOMOC TRUE )

option( USE_WEBENGINE "Use WebEngine support instead of Webkit" OFF )

find_package( Qt5 COMPONENTS Core REQUIRED )
if(${Qt5Core_VERSION} VERSION_GREATER 5.6.0)
    find_package( Qt5 COMPONENTS WebEngine QUIET )
    if( Qt5WebEngine_FOUND )
        set(USE_WEBENGINE ON)
        add_definitions(
            -DWEBENGINE=1
            -DHAVE_WEBENGINE
        )
    else()
        set(USE_WEBENGINE OFF)
        add_definitions(
            -DHAVE_WEBKIT
        )
    endif()
endif()

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
    include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
    message(FATAL_ERROR "No variables.cmake file found.
        Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLUGINS_ROOT_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}
)
set( _HDRS
    ${PLUGIN}.h
    juickjidlist.h
    juickparser.h
    juickdownloader.h
    defines.h
)
set( _SRCS
    ${PLUGIN}.cpp
    juickjidlist.cpp
    juickparser.cpp
    juickdownloader.cpp
)
set( _UIS
    juickjidlist.ui
    settings.ui
)
set( _RSCS
    resources.qrc
)

find_package(Qt5 COMPONENTS Widgets Xml Network REQUIRED)
set(QT_DEPLIBS
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
)
if( USE_WEBENGINE )
    find_package(Qt5 COMPONENTS WebEngine WebEngineWidgets REQUIRED)
    list(APPEND QT_DEPLIBS
        Qt5::WebEngine
        Qt5::WebEngineWidgets
    )
else()
    find_package(Qt5 COMPONENTS WebKit WebKitWidgets REQUIRED)
    list(APPEND QT_DEPLIBS
        Qt5::WebKit
        Qt5::WebKitWidgets
    )
endif()

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
