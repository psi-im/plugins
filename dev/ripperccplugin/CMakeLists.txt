unset(_HDRS)
unset(_UIS)
unset(_SRCS)
unset(_RSCS)
unset(PLUGIN)

set(PLUGIN ripperccplugin)
project(${PLUGIN})
cmake_minimum_required( VERSION 3.1.0 )
if(NOT WIN32)
    set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
    set(PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins")
else()
    set(PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins")
endif()

add_definitions(-DQT_PLUGIN)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ../../include
)

set(HEADERS
    rippercc.h
    ripperccoptions.h
)

set(PLAIN_HEADERS
    qjsonwrapper.h
)

set(SOURCES
    rippercc.cpp
    ripperccoptions.cpp
    qjsonwrapper.cpp
)

set(FORMS
    ripperccoptions.ui
)

set(RESOURCES
    resources.qrc
)

find_package(Qt5 COMPONENTS Core Gui Widgets Xml Network REQUIRED)
add_definitions(-DHAVE_QT5)
set(QT_DEPLIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network)

unset(MOC_SOURCES)
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
unset(UI_SOURCES)
qt5_wrap_ui(UI_SOURCES ${FORMS})
unset(QRC_SOURCES)
qt5_add_resources(QRC_SOURCES ${RESOURCES})

add_library(${PLUGIN} MODULE ${SOURCES} ${HEADERS} ${PLAIN_HEADERS} ${QRC_SOURCES} ${RESOURCES} ${MOC_SOURCES} ${FORMS} ${UI_SOURCES})

target_link_libraries(${PLUGIN} ${QT_DEPLIBS})

if(LINUX)
    install(
        TARGETS ${PLUGIN}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    )
elseif(WIN32)
    install(
        TARGETS ${PLUGIN}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PLUGINS_PATH}
    )
endif()
