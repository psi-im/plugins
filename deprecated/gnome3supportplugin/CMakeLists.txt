set( PLUGIN gnome3supportplugin )
project(${PLUGIN})
cmake_minimum_required( VERSION 2.8.12)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE( NOT WIN32 )
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF( NOT WIN32 )


add_definitions( -Wall -Ddbus -DQT_PLUGIN -DHAVE_QT5 )
include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	../../include
	.
)
set( _SRCS
	${PLUGIN}.cpp
)
set( _RSCS
	resources.qrc
)

find_package( Qt5 COMPONENTS Widgets Xml DBus REQUIRED )

set(QT_DEPLIBS
	Qt5::Widgets
	Qt5::Xml
	Qt5::DBus
)
qt5_add_resources(RSCS ${_RSCS})

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
install(
	TARGETS
	${PLUGIN}
	LIBRARY
	DESTINATION
	${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
)
