set( PLUGIN juickplugin )
cmake_minimum_required(VERSION 3.1.0)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()

option( USE_WEBENGINE "Use WebEngine support instead of Webkit" OFF )

if(USE_QT5)
	add_definitions(-DHAVE_QT5)
	find_package( Qt5Core REQUIRED )
	if(${Qt5Core_VERSION} VERSION_GREATER 5.7.0)
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
endif()

add_definitions( -DQT_PLUGIN )
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	#$
	../../include
	.
)
set( _HDRS
	${PLUGIN}.h
	juickjidlist.h
	juickparser.h
	juickdownloader.h
	defines.h
)
set( _SRCS
	${PLUGIN}.cpp
	juickjidlist.cpp
	juickparser.cpp
	juickdownloader.cpp
)
set( _UIS
	juickjidlist.ui
	settings.ui
)
set( _RSCS
	resources.qrc
)
IF( USE_QT5 )
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
ELSE()
	find_package( Qt4 REQUIRED )
	include( ${QT_USE_FILE} )
	include_directories(
		${QT_QTCORE_INCLUDE_DIR}
		${QT_QTGUI_INCLUDE_DIR}
		${QT_QTXML_INCLUDE_DIR}
		${QT_QTNETWORK_INCLUDE_DIR}
		${QT_QTWEBKIT_INCLUDE_DIR}
	)
	set(QT_DEPLIBS
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTXML_LIBRARY}
		${QT_QTNETWORK_LIBRARY}
		${QT_QTWEBKIT_LIBRARY}
	)
	QT4_WRAP_UI(UIS ${_UIS})
	QT4_ADD_RESOURCES(RSCS ${_RSCS})
ENDIF()

add_library(
	${PLUGIN}
	MODULE
	${_SRCS}
	${UIS}
	${RSCS}
	${TRS}
	${MOCS}
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
