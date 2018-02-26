set(PLUGIN omemoplugin)
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
endif()
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_CXX_STANDARD 11)

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")

if(WIN32)
    if(NOT QCA_DIR)
        set( QCA_DIR "E:\\build\\psisdk\\qca" CACHE STRING "Path to qca library" )
    endif()
    if(NOT SIGNAL_PROTOCOL_C_ROOT)
        set( SIGNAL_PROTOCOL_C_ROOT "E:\\build\\psisdk\\signal-protocol-c" CACHE STRING "Path to signal-protocol-c library" )
    endif()
endif()

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
    include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
    message(FATAL_ERROR "No variables.cmake file found.
        Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Qt5 COMPONENTS Widgets Xml Sql REQUIRED )
find_package(Qca REQUIRED)
find_package(SignalProtocolC REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${Qca_INCLUDE_DIR}
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
    src/crypto.cpp
    src/omemo.cpp
    src/signal.cpp
    src/configwidget.cpp
)

qt5_add_resources(RESOURCES omemoplugin.qrc)

set(QT_DEPLIBS
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
    ${Qca_LIBRARY}
    ${SIGNAL_PROTOCOL_C_LIBRARY}
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
