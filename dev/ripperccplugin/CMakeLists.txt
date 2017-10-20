unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set(PLUGIN ripperccplugin)
project(${PLUGIN})
cmake_minimum_required( VERSION 3.1.0 )

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
	include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
	include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
	message(FATAL_ERROR "No variables.cmake file found.
		Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${PLUGINS_ROOT_DIR}/include
	${CMAKE_CURRENT_LIST_DIR}
	)

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
add_definitions(-DHAVE_QT5)
set(QT_DEPLIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network)

unset(MOC_SOURCES)
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
unset(UI_SOURCES)
qt5_wrap_ui(UI_SOURCES ${FORMS})
unset(QRC_SOURCES)
qt5_add_resources(QRC_SOURCES ${RESOURCES})

add_library(${PLUGIN} MODULE ${SOURCES} ${HEADERS} ${PLAIN_HEADERS} ${QRC_SOURCES} ${RESOURCES} ${MOC_SOURCES} ${FORMS} ${UI_SOURCES})

target_link_libraries(${PLUGIN} ${QT_DEPLIBS})

if(LINUX)
	install(
		TARGETS ${PLUGIN}
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		)
elseif(WIN32)
	install(
		TARGETS ${PLUGIN}
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		)
endif()
