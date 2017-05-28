set(PLUGIN ripperccplugin)

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

if(QT5_BUILD)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Gui REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Xml REQUIRED)
    find_package(Qt5Network REQUIRED)

    add_definitions(-DHAVE_QT5)

    macro(qt4_add_resources)
        qt5_add_resources(${ARGN})
    endmacro()

    macro(qt4_wrap_cpp)
        qt5_wrap_cpp(${ARGN})
    endmacro()

    macro(qt4_wrap_ui)
        qt5_wrap_ui(${ARGN})
    endmacro()
    set(QT_DEPLIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml Qt5::Network)
else()
    find_package(Qt4 REQUIRED)
    find_package(QJSON REQUIRED)
    include(${QT_USE_FILE})
    include_directories(
	    ${QJSON_INCLUDE_DIR}
	    ${QJSON_INCLUDE_DIR}/../
	    ${QT_QTCORE_INCLUDE_DIR}
	    ${QT_QTNETWORK_INCLUDE_DIR}
	    ${QT_QTXML_INCLUDE_DIR}
	    )
    set(QT_LIBRARIES
	    ${QT_QTCORE_LIBRARY}
	    ${QT_QTGUI_LIBRARY}
	    ${QT_QTXML_LIBRARY}
	    ${QT_QTNETWORK_LIBRARY}
	)
    set(QT_DEPLIBS ${QT_LIBRARIES} ${QJSON_LIBRARY})
endif()

unset(MOC_SOURCES)
qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
unset(UI_SOURCES)
qt4_wrap_ui(UI_SOURCES ${FORMS})
unset(QRC_SOURCES)
qt4_add_resources(QRC_SOURCES ${RESOURCES})

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
