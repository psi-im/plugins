set( PLUGIN gmailserviceplugin )
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
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ../../include
    .
)
set( _HDRS
    ${PLUGIN}.h
    accountsettings.h
    common.h
    actionslist.h
    viewmaildlg.h
)
set( _SRCS
    ${PLUGIN}.cpp
    accountsettings.cpp
    common.cpp
    actionslist.cpp
    viewmaildlg.cpp
)
set( _UIS
    options.ui
    viewmaildlg.ui
)
set( _RSCS
    ${PLUGIN}.qrc
)

find_package( Qt5 COMPONENTS Widgets Xml REQUIRED )
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
