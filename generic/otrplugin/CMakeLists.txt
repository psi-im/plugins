unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN otrplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
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

IF( WIN32 )
    if(NOT LIBGCRYPT_ROOT)
        set( LIBGCRYPT_ROOT "C:\\build\\psisdk\\libgcrypt\\i386" CACHE STRING "Path to libgcrypt library" )
    endif()
    if(NOT LIBGPGERROR_ROOT)
        set( LIBGPGERROR_ROOT "C:\\build\\psisdk\\libgpg-error\\i386" CACHE STRING "Path to libgpg-error library" )
    endif()
    if(NOT LIBOTR_ROOT)
        set( LIBOTR_ROOT "C:\\build\\psisdk\\libotr\\i386" CACHE STRING "Path to libotr library" )
    endif()
    if(NOT LIBGCRYPT_ROOT)
        set( LIBTIDY_ROOT "C:\\build\\psisdk\\libtidy\\i386" CACHE STRING "Path to libtidy library" )
    endif()
ENDIF()

find_package( LibGcrypt REQUIRED )
find_package( LibOtr REQUIRED )
find_package( LibTidy REQUIRED )
find_package( LibGpgError REQUIRED )
add_definitions(
        ${LIBOTR_DEFINITIONS}
        ${LIBTIDY_DEFINITIONS}
        ${LIBGCRYPT_DEFINITIONS}
        ${LIBGPGERROR_DEFINITIONS}
)

#dirty hack for windows
IF( WIN32 )
    set(TIDY_INCLUDES
        ${LIBTIDY_INCLUDE_DIR}/..
    )
ENDIF()
#
include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LIBOTR_INCLUDE_DIR}
        ${LIBTIDY_INCLUDE_DIR}
        ${LIBGCRYPT_INCLUDE_DIR}
        ${LIBGPGERROR_INCLUDE_DIR}
        ${TIDY_INCLUDES}
        ${PLUGINS_ROOT_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
)
set( _HDRS
    src/psiotrplugin.h
    src/otrmessaging.h
    src/otrinternal.h
    src/psiotrconfig.h
    src/psiotrclosure.h
    src/htmltidy.h
    src/otrlextensions.h
)
set( _SRCS
    src/psiotrplugin.cpp
    src/otrmessaging.cpp
    src/otrinternal.cpp
    src/psiotrconfig.cpp
    src/psiotrclosure.cpp
    src/htmltidy.cpp
    src/otrlextensions.c
)
set( _RSCS
    ${PLUGIN}.qrc
)

find_package( Qt5 COMPONENTS Widgets Xml Concurrent REQUIRED )
set(QT_DEPLIBS
    Qt5::Widgets
    Qt5::Xml
    Qt5::Concurrent
)
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
    ${LIBOTR_LIBRARY}
    ${LIBTIDY_LIBRARY}
    ${LIBGCRYPT_LIBRARY}
    ${LIBGPGERROR_LIBRARY}
)
if( UNIX AND NOT( APPLE OR CYGWIN ) )
    install(
        TARGETS
        ${PLUGIN}
        LIBRARY
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
        RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    )
endif()
if( WIN32 )
    install(
        TARGETS
        ${PLUGIN}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    )
endif()
