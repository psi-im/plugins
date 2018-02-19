set(PLUGIN omemoplugin)
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_AUTOMOC TRUE)

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
	include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
	include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
	message(FATAL_ERROR "No variables.cmake file found.
        Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

add_subdirectory(lib/libsignal-protocol-c)

include_directories(
    ${CMAKE_SOURCE_DIR}
	${Qca_INCLUDE_DIR}
	${signal-protocol-c_SOURCE_DIR}/src
	${PLUGINS_ROOT_DIR}/include
	)

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

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qca REQUIRED)

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
	signal-protocol-c
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
			DESTINATION
			${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
			RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
	)
endif()
