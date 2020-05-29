unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN conferenceloggerplugin )
project(${PLUGIN} LANGUAGES CXX)
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
endif()
set( CMAKE_AUTOMOC ON )

get_filename_component(ABS_PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(PLUGINS_ROOT_DIR "${ABS_PLUGINS_ROOT_DIR}" CACHE STRING "Plugins root path. Path where include directory placed")
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

set( _HDRS
    typeaheadfind.h
    viewer.h
)
set( _SRCS
    ${PLUGIN}.cpp
    typeaheadfind.cpp
    viewer.cpp
)
set(QT_DEPLIBS
    Qt5::Widgets
    Qt5::Xml
)

if(WIN32)
    set(LIB_TYPE "MODULE")
else()
    set(LIB_TYPE "SHARED")
endif()

add_library(
    ${PLUGIN}
    ${LIB_TYPE}
    ${_SRCS}
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
    ${PLUGINS_INSTALL_PATH}
    RUNTIME DESTINATION
    ${PLUGINS_INSTALL_PATH}
)
