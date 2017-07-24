unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN videostatusplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()

add_definitions( -DQT_PLUGIN -DHAVE_QT5 )
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

find_package( Qt5 COMPONENTS Widgets Xml REQUIRED )
set(QT_DEPLIBS
	Qt5::Widgets
	Qt5::Xml
)
IF( UNIX AND NOT( APPLE OR CYGWIN ) )
	find_package( Qt5 COMPONENTS DBus X11Extras REQUIRED )
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

add_library(
	${PLUGIN}
	MODULE
	${_SRCS}
	${UIS}
	${RSCS}
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
