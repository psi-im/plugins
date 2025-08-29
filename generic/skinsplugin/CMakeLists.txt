unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN skinsplugin )
project(${PLUGIN} LANGUAGES CXX)
cmake_minimum_required(VERSION 3.10.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
set( CMAKE_AUTOMOC TRUE )

if(CMAKE_VERSION GREATER_EQUAL 3.20)
    cmake_path(SET ABS_PLUGINS_ROOT_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
else()
    get_filename_component(ABS_PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
set(PLUGINS_ROOT_DIR "${ABS_PLUGINS_ROOT_DIR}" CACHE STRING "Plugins root path. Path where include directory placed")
set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
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
    if(CMAKE_VERSION GREATER_EQUAL 3.20)
        cmake_path(SET PLUGINS_INSTALL_PATH NORMALIZE "${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}")
    else()
        set(PLUGINS_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}")
    endif()
endif()

if(USE_QT6)
    set(QT_DEFAULT_MAJOR_VERSION 6)
endif()

set( _HDRS
    skin.h
    optionsparser.h
)
set( _SRCS
    ${PLUGIN}.cpp
    skin.cpp
    optionsparser.cpp
)
set( _UIS
    ${PLUGIN}.ui
    previewer.ui
    getskinname.ui
)
set( _RSCS
    ${PLUGIN}.qrc
)

find_package( Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Widgets Xml REQUIRED )
set(QT_DEPLIBS
    Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
    Qt${QT_DEFAULT_MAJOR_VERSION}::Xml
)
qt_wrap_ui(UIS ${_UIS})
qt_add_resources(RSCS ${_RSCS})

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
