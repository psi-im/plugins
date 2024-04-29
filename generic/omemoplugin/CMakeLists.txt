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

if(WIN32 AND (NOT OMEMO_C_ROOT))
    set( OMEMO_C_ROOT "E:\\build\\psisdk\\omemo-c" CACHE STRING "Path to omemo-c library" )
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

option(BUNDLED_OMEMO_C_ALL "Build plugin with bundlel omemo-c and protobuf-c libraries from GitHub" OFF)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Network Widgets Xml Sql REQUIRED)
find_package(OpenSSL COMPONENTS Crypto REQUIRED)
#find_package(SignalProtocolC REQUIRED)

if(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    string(TOLOWER ${CMAKE_TOOLCHAIN_FILE} TOOLCHAIN_FILE)
    string(REGEX MATCH "mxe-conf" MXE_DETECTED "${TOOLCHAIN_FILE}")
    if(MXE_DETECTED)
        set(BUNDLED_OMEMO_C_ALL ON)
    endif()
endif()
if(BUNDLED_OMEMO_C_ALL)
    include(cmake/omemo_c.cmake)
elseif(WIN32)
    find_package(PROTOBUF_C REQUIRED)
else()
    find_package(OMEMO_C REQUIRED)
endif()


include_directories(
    ${PLUGINS_ROOT_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${OMEMO_C_INCLUDE_DIR}
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

if(BUNDLED_OMEMO_C_ALL)
    add_dependencies(${PLUGIN} OmemoCProject)
    target_include_directories(${PLUGIN} PRIVATE ${OMEMO_C_INCLUDE_DIR})
endif()

target_link_libraries(
    ${PLUGIN}
    ${QT_DEPLIBS}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OMEMO_C_LIBRARIES}
)

if(WIN32)
    target_link_libraries(${PLUGIN}
        ${Protobuf_C_LIBRARY}
    )
endif()

install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION
    ${PLUGINS_INSTALL_PATH}
    RUNTIME DESTINATION
    ${PLUGINS_INSTALL_PATH}
)
