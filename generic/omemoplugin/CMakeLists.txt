cmake_minimum_required(VERSION 3.10.0)

set(PLUGIN omemoplugin)
project(${PLUGIN} LANGUAGES CXX)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_CXX_STANDARD 14)

if(WIN32)
    if(NOT QCA_DIR)
        set( QCA_DIR "E:\\build\\psisdk\\qca" CACHE STRING "Path to qca library" )
    endif()
    if(NOT OMEMO_C_ROOT)
        set( OMEMO_C_ROOT "E:\\build\\psisdk\\omemo-c" CACHE STRING "Path to omemo-c library" )
    endif()
endif()

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

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Network Widgets Xml Sql REQUIRED)
#find_package(OpenSSL COMPONENTS Crypto REQUIRED)
#find_package(SignalProtocolC REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OMEMO_C REQUIRED IMPORTED_TARGET libomemo-c)

include_directories(
    ${PLUGINS_ROOT_DIR}/include
)

set(HEADERS
    src/omemoplugin.h
    src/storage.h
    src/crypto.h
    src/omemo.h
    src/signal.h
    src/configwidget.h
    src/crypto_ossl.h
)

set(SOURCES
    src/omemoplugin.cpp
    src/storage.cpp
    src/crypto.cpp
    src/crypto_ossl.cpp
    src/omemo.cpp
    src/signal.cpp
    src/configwidget.cpp
)

qt_add_resources(RESOURCES omemoplugin.qrc)

set(QT_DEPLIBS
    Qt${QT_DEFAULT_MAJOR_VERSION}::Network
    Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
    Qt${QT_DEFAULT_MAJOR_VERSION}::Xml
    Qt${QT_DEFAULT_MAJOR_VERSION}::Sql
)

if(WIN32)
    set(LIB_TYPE "MODULE")
else()
    set(LIB_TYPE "SHARED")
endif()

add_library(
    ${PLUGIN}
    ${LIB_TYPE}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    src/psiplugin.json
)

target_link_libraries(
    ${PLUGIN}
    ${QT_DEPLIBS}
    PkgConfig::OMEMO_C
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
