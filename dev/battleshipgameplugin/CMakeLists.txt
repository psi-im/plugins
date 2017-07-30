unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN battleshipgameplugin )
project(${PLUGIN})
cmake_minimum_required( VERSION 3.1.0 )
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE( NOT WIN32 )
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF( NOT WIN32 )

add_definitions( -DQT_PLUGIN )
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	../../include
	.
)
set( _HDRS
	${PLUGIN}.h
	boarddelegate.h
	boardmodel.h
	boardview.h
	gamemodel.h
	gamesessions.h
	invitedialog.h
	options.h
	pluginwindow.h

)
set( _SRCS
	${PLUGIN}.cpp
	boarddelegate.cpp
	boardmodel.cpp
	boardview.cpp
	gamemodel.cpp
	gamesessions.cpp
	invitedialog.cpp
	options.cpp
	pluginwindow.cpp
)
set( _UIS
	invitationdialog.ui
	invitedialog.ui
	options.ui
	pluginwindow.ui
)
set( _RSCS
	${PLUGIN}.qrc
)
find_package( Qt5 COMPONENTS Widgets Xml REQUIRED )
add_definitions(
	${Qt5Widgets_DEFINITIONS}
	${Qt5Xml_DEFINITIONS}
)
set(QT_DEPLIBS
	Qt5::Widgets
	Qt5::Xml
)
qt5_wrap_ui(UIS ${_UIS})
qt5_add_resources(RSCS ${_RSCS})

add_library(
	${PLUGIN}
	MODULE
	${_SRCS}
	${UIS}
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
endif( UNIX AND NOT( APPLE OR CYGWIN ) )
if( WIN32 )
	install(
		TARGETS
		${PLUGIN}
		LIBRARY
		DESTINATION
		${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
	)
endif( WIN32 )

