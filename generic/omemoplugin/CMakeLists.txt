set(PLUGIN omemoplugin)
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
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
    if(NOT SIGNAL_PROTOCOL_C_ROOT)
        set( SIGNAL_PROTOCOL_C_ROOT "E:\\build\\psisdk\\signal-protocol-c" CACHE STRING "Path to signal-protocol-c library" )
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

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

find_package(Qt5 COMPONENTS Network Widgets Xml Sql REQUIRED)
find_package(OpenSSL COMPONENTS Crypto REQUIRED)
find_package(SignalProtocolC REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${SIGNAL_PROTOCOL_C_INCLUDE_DIR}
    ${PLUGINS_ROOT_DIR}/include
)

#Hack to check signal-protocol-c version
if(SignalProtocolC_FOUND)
    try_compile(OLD_SIGNAL
                ${CMAKE_BINARY_DIR}/oldSignal
                ${PROJECT_SOURCE_DIR}/config.tests/oldSignal/main.cpp
                LINK_LIBRARIES ${SIGNAL_PROTOCOL_C_LIBRARY}
                CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${SIGNAL_PROTOCOL_C_INCLUDE_DIR}
    )
    set(MSG_TEST "Performing test: OLD_SIGNAL")
    if(OLD_SIGNAL)
        message(STATUS "${MSG_TEST} - Complete")
        add_definitions(-DOLD_SIGNAL)
    else()
        message(STATUS "${MSG_TEST}  - Failed")
    endif()
endif()

set(HEADERS
    src/omemoplugin.h
    src/storage.h
    src/crypto.h
    src/omemo.h
    src/signal.h
    src/configwidget.h
)

set(SOURCES
    src/omemoplugin.cpp
    src/storage.cpp
    src/crypto_ossl.cpp
    src/crypto_common.cpp
    src/omemo.cpp
    src/signal.cpp
    src/configwidget.cpp
)

qt5_add_resources(RESOURCES omemoplugin.qrc)

set(QT_DEPLIBS
    Qt5::Network
    Qt5::Widgets
    Qt5::Xml
    Qt5::Sql
)

add_library(
    ${PLUGIN}
    MODULE
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(
    ${PLUGIN}
    ${QT_DEPLIBS}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${SIGNAL_PROTOCOL_C_LIBRARY}
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
