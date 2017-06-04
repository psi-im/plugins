set( PLUGIN videostatusplugin )
cmake_minimum_required(VERSION 3.1.0)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()


add_definitions( -DQT_PLUGIN )
if( NOT WIN32 )
	add_definitions( -Ddbus -DHAVE_DBUS )
	find_package( X11 REQUIRED )
else()
	add_definitions( -DQ_OS_WIN )
endif()
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	../../include
	.
)
if( NOT WIN32 )
	set( x11_SRCS
		x11info.cpp
		)
	set( x11_HDRS
		x11info.h
	)
endif()
set( _SRCS
	${PLUGIN}.cpp
	${x11_SRCS}
)
set( _UIS
	options.ui
)
set( _RSCS
	resources.qrc
)
IF( USE_QT5 )
	find_package( Qt5Widgets REQUIRED )
	find_package( Qt5Xml REQUIRED )
	set(QT_DEPLIBS
		Qt5::Widgets
		Qt5::Xml
	)
	IF( UNIX AND NOT( APPLE OR CYGWIN ) )
		find_package( Qt5DBus REQUIRED )
		find_package( Qt5X11Extras REQUIRED )
		find_package( XCB REQUIRED )
		add_definitions(
			-DHAVE_DBUS
		)
		set(QT_DEPLIBS
			${QT_DEPLIBS}
			Qt5::DBus
			Qt5::X11Extras
			${XCB_LIBRARY}
		)
	ENDIF()
	qt5_wrap_ui(UIS ${_UIS})
	qt5_add_resources(RSCS ${_RSCS})
ELSE()
	find_package( Qt4 REQUIRED )
	include( ${QT_USE_FILE} )
	include_directories(
		${QT_QTCORE_INCLUDE_DIR}
		${QT_QTGUI_INCLUDE_DIR}
		${QT_QTXML_INCLUDE_DIR}

	)
	IF( UNIX AND NOT( APPLE OR CYGWIN ) )
		include_directories(${QT_QTDBUS_INCLUDE_DIR})
	ENDIF()
	set(QT_DEPLIBS
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTXML_LIBRARY}
	)
	IF( UNIX AND NOT( APPLE OR CYGWIN ) )
		set(QT_DEPLIBS "${QT_DEPLIBS}" "${QT_QTDBUS_LIBRARY}" )
	ENDIF()
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
if( UNIX AND NOT( APPLE OR CYGWIN ) )
	target_link_libraries(
		${PLUGIN}
		${X11_LIBRARIES}
		${QT_DEPLIBS}
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
endif()

if( WIN32 )
	target_link_libraries(
		${PLUGIN}
		${QT_DEPLIBS}
	)
	install(
		TARGETS
		${PLUGIN}
		LIBRARY
		DESTINATION
		${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
	)
endif()
