unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set( PLUGIN battleshipgameplugin )
project(${PLUGIN})
cmake_minimum_required( VERSION 3.1.0 )
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 OLD)
endif()
set( CMAKE_AUTOMOC TRUE )

set(PLUGINS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE STRING "Plugins root path. Path where include directory placed")
set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PLUGINS_ROOT_DIR}/cmake/modules
)
find_package(PsiPluginsApi REQUIRED)
include(${PsiPluginsApi_DIR}/variables.cmake)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ${PsiPluginsApi_INCLUDE_DIR}
)
if(NOT PLUGINS_INSTALL_PATH)
    set(PLUGINS_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}")
endif()

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

install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION
    ${PLUGINS_INSTALL_PATH}
    RUNTIME DESTINATION
    ${PLUGINS_INSTALL_PATH}
)
