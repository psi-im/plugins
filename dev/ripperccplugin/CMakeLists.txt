unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set(PLUGIN ripperccplugin)
project(${PLUGIN} LANGUAGES CXX)
cmake_minimum_required( VERSION 3.1.0 )

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

set(HEADERS
    rippercc.h
    ripperccoptions.h
    )

set(PLAIN_HEADERS
    qjsonwrapper.h
    )

set(SOURCES
    rippercc.cpp
    ripperccoptions.cpp
    qjsonwrapper.cpp
    )

set(FORMS
    ripperccoptions.ui
    )

set(RESOURCES
    resources.qrc
    )

find_package(Qt5 COMPONENTS Core Gui Widgets Xml Network REQUIRED)
set(QT_DEPLIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network)

unset(MOC_SOURCES)
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
unset(UI_SOURCES)
qt5_wrap_ui(UI_SOURCES ${FORMS})
unset(QRC_SOURCES)
qt5_add_resources(QRC_SOURCES ${RESOURCES})

if(WIN32)
    set(LIB_TYPE "MODULE")
else()
    set(LIB_TYPE "SHARED")
endif()

add_library(${PLUGIN}
            ${LIB_TYPE}
            ${SOURCES}
            ${HEADERS}
            ${PLAIN_HEADERS}
            ${QRC_SOURCES}
            ${RESOURCES}
            ${MOC_SOURCES}
            ${FORMS}
            ${UI_SOURCES}
            )

target_link_libraries(${PLUGIN} ${QT_DEPLIBS})

install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION
    ${PLUGINS_INSTALL_PATH}
    RUNTIME DESTINATION
    ${PLUGINS_INSTALL_PATH}
)
