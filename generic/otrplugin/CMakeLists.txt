set( PLUGIN otrplugin )
project(${PLUGIN})
cmake_minimum_required(VERSION 3.1.0)
set( CMAKE_AUTOMOC TRUE )

IF( NOT WIN32 )
	set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
	set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ELSE()
	set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
ENDIF()

IF( ${PROJECT_SOURCE_DIR} )
	set( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake/modules" )
ELSE()
	set( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/modules")
ENDIF()

IF( WIN32 )
	if(NOT LIBGCRYPT_ROOT)
		set( LIBGCRYPT_ROOT "C:\\build\\psibuild\\psideps\\libgcrypt\\i386" CACHE STRING "Path to libgcrypt library" )
	endif()
	if(NOT LIBGPGERROR_ROOT)
		set( LIBGPGERROR_ROOT "C:\\build\\psibuild\\psideps\\libgpg-error\\i386" CACHE STRING "Path to libgpg-error library" )
	endif()
	if(NOT LIBOTR_ROOT)
		set( LIBOTR_ROOT "C:\\build\\psibuild\\psideps\\libotr\\i386" CACHE STRING "Path to libotr library" )
	endif()
	if(NOT LIBGCRYPT_ROOT)
		set( LIBTIDY_ROOT "C:\\build\\psibuild\\psideps\\libtidy\\i386" CACHE STRING "Path to libtidy library" )
	endif()
	add_definitions( -DQ_WS_WIN )
ENDIF()
find_package( LibGcrypt REQUIRED )
find_package( LibOtr REQUIRED )
find_package( LibTidy REQUIRED )
find_package( LibGpgError REQUIRED )
add_definitions(
		-DQT_PLUGIN
		${LIBOTR_DEFINITIONS}
		${LIBTIDY_DEFINITIONS}
		${LIBGCRYPT_DEFINITIONS}
		${LIBGPGERROR_DEFINITIONS}
)
#dirty hack for windows
IF( WIN32 )
	set(TIDY_INCLUDES
		${LIBTIDY_INCLUDE_DIR}/..
	)
ENDIF()
#
include_directories(
		${CMAKE_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${LIBOTR_INCLUDE_DIR}
		${LIBTIDY_INCLUDE_DIR}
		${LIBGCRYPT_INCLUDE_DIR}
		${LIBGPGERROR_INCLUDE_DIR}
		${TIDY_INCLUDES}
		../../include
		./src
		.
)
set( _HDRS
	src/psiotrplugin.h
	src/otrmessaging.h
	src/otrinternal.h
	src/psiotrconfig.h
	src/psiotrclosure.h
	src/htmltidy.h
	src/otrlextensions.h
)
set( _SRCS
	src/psiotrplugin.cpp
	src/otrmessaging.cpp
	src/otrinternal.cpp
	src/psiotrconfig.cpp
	src/psiotrclosure.cpp
	src/htmltidy.cpp
	src/otrlextensions.c
)
set( _RSCS
	${PLUGIN}.qrc
)
IF( USE_QT5 )
	find_package( Qt5Widgets REQUIRED )
	find_package( Qt5Xml REQUIRED )
	find_package( Qt5Concurrent REQUIRED )
	set(QT_DEPLIBS
		Qt5::Widgets
		Qt5::Xml
		Qt5::Concurrent
	)
	qt5_add_resources(RSCS ${_RSCS})
ELSE()
	find_package( Qt4 REQUIRED )
	include( ${QT_USE_FILE} )
	include_directories(
		${QT_QTCORE_INCLUDE_DIR}
		${QT_QTGUI_INCLUDE_DIR}
		${QT_QTXML_INCLUDE_DIR}
	)
	set(QT_DEPLIBS
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTXML_LIBRARY}
	)
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
	${LIBOTR_LIBRARY}
	${LIBTIDY_LIBRARY}
	${LIBGCRYPT_LIBRARY}
	${LIBGPGERROR_LIBRARY}
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
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
	)
endif()
