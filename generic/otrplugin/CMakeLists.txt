unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN otrplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    if(${CMAKE_VERSION} VERSION_LESS "3.10.0")
        cmake_policy(SET CMP0071 OLD)
    else()
        cmake_policy(SET CMP0071 NEW)
    endif()
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

if( WIN32 )
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
endif()

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
if( WIN32 )
    set(TIDY_INCLUDES
        ${LIBTIDY_INCLUDE_DIR}/..
    )
endif()
#
include_directories(
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
    ${LIBOTR_LIBRARY}
    ${LIBTIDY_LIBRARY}
    ${LIBGCRYPT_LIBRARY}
    ${LIBGPGERROR_LIBRARY}
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
