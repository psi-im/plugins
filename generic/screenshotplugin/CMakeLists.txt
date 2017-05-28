set( PLUGIN screenshotplugin )
cmake_minimum_required( VERSION 2.8.12 )
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()


add_definitions( -DQT_PLUGIN )
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	../../include
	qxt/core
	qxt/gui
	.
)
if( UNIX AND NOT( APPLE OR CYGWIN ) )
	find_package( X11 REQUIRED )
	add_definitions( -DX11 )
	set( qxt_X11_SRCS
		qxt/gui/qxtwindowsystem_x11.cpp
		#qxt/gui/x11info.cpp
	)
	set( qxt_X11_HDRS
		qxt/gui/x11info.h
	)
endif()
if( APPLE )
	set( qxt_mac_SRCS
		qxt/gui/qxtwindowsystem_mac.cpp
	)
	set( qxt_mac_HRDS
		qxt/gui/qxtwindowsystem_mac.h
	)
endif()
if( WIN32 )
	add_definitions( -DQ_WS_WIN )
	set( qxt_win_SRCS
		qxt/gui/qxtwindowsystem_win.cpp
	)
endif()
set ( _HDRS
	screenshot.h
	server.h
	editserverdlg.h
	screenshotoptions.h
	toolbar.h
	pixmapwidget.h
	options.h
	optionsdlg.h
	optionswidget.h
	screenshoticonset.h
	controller.h
	defines.h
	proxysettingsdlg.h
	qxt/core/qxtglobal.h
	qxt/gui/qxtwindowsystem.h
	${qxt_X11_HDRS}
	${qxt_mac_HRDS}
)
set ( _SRCS
	${PLUGIN}.cpp
	screenshot.cpp
	server.cpp
	editserverdlg.cpp
	screenshotoptions.cpp
	toolbar.cpp
	pixmapwidget.cpp
	options.cpp
	optionsdlg.cpp
	optionswidget.cpp
	screenshoticonset.cpp
	controller.cpp
	proxysettingsdlg.cpp
	qxt/core/qxtglobal.cpp
	qxt/gui/qxtwindowsystem.cpp
	${qxt_X11_SRCS}
	${qxt_mac_SRCS}
	${qxt_win_SRCS}
)
set ( _UIS
	optionswidget.ui
	editserverdlg.ui
	screenshot.ui
	screenshotoptions.ui
	optionsdlg.ui
	proxysettingsdlg.ui
)
set ( _RSCS
	${PLUGIN}.qrc
)
IF( USE_QT5 )
	find_package( Qt5Widgets REQUIRED )
	find_package( Qt5Xml REQUIRED )
	find_package( Qt5Network REQUIRED )
	find_package( Qt5PrintSupport REQUIRED )
	set(QT_DEPLIBS
		Qt5::Widgets
		Qt5::Xml
		Qt5::Network
		Qt5::PrintSupport
	)
	if( UNIX AND NOT( APPLE OR CYGWIN ) )
		find_package( Qt5X11Extras REQUIRED )
		set(QT_DEPLIBS
			${QT_DEPLIBS}
			Qt5::X11Extras
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
	)
	set(QT_DEPLIBS
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTXML_LIBRARY}
		${QT_QTNETWORK_LIBRARY}
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
