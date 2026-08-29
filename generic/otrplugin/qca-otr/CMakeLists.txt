cmake_minimum_required(VERSION 3.16)

project(qca-otr LANGUAGES CXX)

if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
    set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "Qt major version used by qca-otr")
endif()

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Core)
find_package(Qca3 CONFIG REQUIRED)

add_library(qca-otr STATIC
    src/codec.cpp
    src/crypto.cpp
)

add_library(QcaOtr::QcaOtr ALIAS qca-otr)

set_target_properties(qca-otr PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(qca-otr PUBLIC cxx_std_17)
target_include_directories(qca-otr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(qca-otr
    PUBLIC
        Qt${QT_DEFAULT_MAJOR_VERSION}::Core
        Qca3::Qca
)

if(PROJECT_IS_TOP_LEVEL)
    option(QCA_OTR_BUILD_TESTS "Build qca-otr tests" ON)
else()
    option(QCA_OTR_BUILD_TESTS "Build qca-otr tests" OFF)
endif()

if(QCA_OTR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
