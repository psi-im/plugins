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

if(EXISTS "${PLUGINS_ROOT_DIR}/variables.cmake")
    include("${PLUGINS_ROOT_DIR}/variables.cmake")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/../../variables.cmake")
else()
    message(FATAL_ERROR "No variables.cmake file found.
        Please set path to this file to PLUGINS_ROOT_DIR variable")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLUGINS_ROOT_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}
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

install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
)
