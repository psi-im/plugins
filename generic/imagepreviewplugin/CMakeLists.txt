unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN imagepreviewplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()

option( USE_WEBENGINE "Use WebEngine support instead of Webkit" OFF )

find_package( Qt5 COMPONENTS Core REQUIRED )
if(${Qt5Core_VERSION} VERSION_GREATER 5.6.0)
	find_package( Qt5 COMPONENTS WebEngine QUIET )
	if( Qt5WebEngine_FOUND )
		set(USE_WEBENGINE ON)
		add_definitions(
			-DWEBENGINE=1
			-DHAVE_WEBENGINE
		)
	else()
		set(USE_WEBENGINE OFF)
		add_definitions(
			-DHAVE_WEBKIT
		)
	endif()
endif()

add_definitions( -DQT_PLUGIN -DHAVE_QT5 )
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	../../include
	.
)

set( _SRCS
	${PLUGIN}.cpp
	ScrollKeeper.cpp
)

set( _HDRS
	ScrollKeeper.h
)

set( _RSCS
	${PLUGIN}.qrc
)

add_definitions(-DHAVE_QT5)
find_package(Qt5 COMPONENTS Widgets Xml Network REQUIRED)
set(QT_DEPLIBS
	Qt5::Widgets
	Qt5::Xml
	Qt5::Network
)
if( USE_WEBENGINE )
	find_package(Qt5 COMPONENTS WebEngine WebEngineWidgets REQUIRED)
	list(APPEND QT_DEPLIBS
		Qt5::WebEngine
		Qt5::WebEngineWidgets
	)
else()
	find_package(Qt5 COMPONENTS WebKit WebKitWidgets REQUIRED)
	list(APPEND QT_DEPLIBS
		Qt5::WebKit
		Qt5::WebKitWidgets
	)
endif()
qt5_wrap_ui(UIS ${_UIS})
qt5_add_resources(RSCS ${_RSCS})

add_library(
	${PLUGIN}
	MODULE
	${_SRCS}
	${RSCS}
)
target_link_libraries(
	${PLUGIN}
	${QT_DEPLIBS}
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

