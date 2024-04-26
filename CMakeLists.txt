cmake_minimum_required( VERSION 3.10.0 )
get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

if(NOT IS_SUBPROJECT)
    project(psi-plugins)
    find_program(CLF_BIN clang-format DOC "Path to clang-format binary")
    if(CLF_BIN)
        #Obtain list of source files
        file(GLOB_RECURSE SRC_LIST
            *.c
            *.cc
            *.cpp
            *.hpp
            *.h
            *.mm
        )
        add_custom_target(fix-codestyle
            COMMAND ${CLF_BIN}
            --verbose
            -style=file
            -i ${SRC_LIST}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Fix codestyle with clang-format"
            VERBATIM
        )
    endif()
endif()
set_directory_properties(PROPERTIES
    INCLUDE_DIRECTORIES ""
    COMPILE_DEFINITIONS ""
)

set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_LIST_DIR}/cmake/modules
)

#Find Psi Plugins API
find_package(PsiPluginsApi QUIET)
if(PsiPluginsApi_DIR)
    include(${PsiPluginsApi_DIR}/variables.cmake)
    message(STATUS "PsiPluginsApi_DIR: ${PsiPluginsApi_DIR}")
    message(STATUS "PsiPluginsApi_INCLUDE_DIR: ${PsiPluginsApi_INCLUDE_DIR}")
endif()

if((NOT IS_SUBPROJECT) AND MAIN_PROGRAM_NAME)
    message(STATUS "Main Program: ${MAIN_PROGRAM_NAME}")
    if(PLUGINS_INSTALL_PATH)
        message(STATUS "Plugins Install Path: ${PLUGINS_INSTALL_PATH}")
    endif()
    if(PLUGINS_DATA_PATH)
        message(STATUS "Plugins Data Path: ${PLUGINS_DATA_PATH}")
    endif()
endif()

option( BUILD_DEV_PLUGINS "Build plugins from dev directory" OFF )
set(MAIN_PROGRAM_NAME "psi" CACHE STRING "Main program name: psi or psi-plus")

if(NOT (DEFINED USE_X11))
    option( USE_X11 "Enable X11 features support" ON )
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/generic")
    set(GENERIC_PLUGINS_FOUND ON)
    message(STATUS "Generic plugins found: ${CMAKE_CURRENT_LIST_DIR}")
else()
    message(FATAL_ERROR "ENABLE_PLUGINS flag is enabled but no generic plugins found at ${CMAKE_CURRENT_LIST_DIR}")
endif()
if(LINUX)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/unix")
        set(UNIX_PLUGINS_FOUND ON)
        message(STATUS "Unix plugins found: ${CMAKE_CURRENT_LIST_DIR}")
    else()
        message(WARNING "ENABLE_PLUGINS flag is enabled but no unix plugins found at ${CMAKE_CURRENT_LIST_DIR}")
    endif()
endif()

if(BUILD_DEV_PLUGINS)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/dev")
        set(DEV_PLUGINS_FOUND ON)
        message(STATUS "Dev plugins found: ${CMAKE_CURRENT_LIST_DIR}")
    else()
        message(WARNING "ENABLE_PLUGINS flag is enabled but no dev plugins found at ${CMAKE_CURRENT_LIST_DIR}")
    endif()
endif()

if( NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY )
    if( NOT IS_SUBPROJECT )
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/psi")
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    endif()
endif()

#Set library output path for plugins
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins")

set(BUILD_PLUGINS "ALL" CACHE STRING "List of plugins to build")

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DQT_NO_DEBUG )
else()
    add_definitions( -Wall )
endif()

if(PSI_PLUS)
    set(MAIN_PROGRAM_NAME "psi-plus")
endif()

if(NOT WIN32)
    set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
    set(PLUGINS_PATH "lib${LIB_SUFFIX}/${MAIN_PROGRAM_NAME}/plugins" CACHE STRING "Install suffix for plugins")
else()
    set(PLUGINS_PATH "${MAIN_PROGRAM_NAME}/plugins" CACHE STRING "Install suffix for plugins")
endif()

if("${QT_DEFAULT_MAJOR_VERSION}" STREQUAL "")
    set(QT_DEFAULT_MAJOR_VERSION 5)
endif()

if(GENERIC_PLUGINS_FOUND)
    add_subdirectory(generic)
endif()
if(LINUX AND UNIX_PLUGINS_FOUND)
    add_subdirectory(unix)
endif()
if(BUILD_DEV_PLUGINS AND DEV_PLUGINS_FOUND)
    add_subdirectory(dev)
endif()

unset(BUILD_PLUGINS CACHE)
unset(PLUGINS_PATH CACHE)
