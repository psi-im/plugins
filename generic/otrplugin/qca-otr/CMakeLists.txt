cmake_minimum_required(VERSION 3.16)

project(qca-otr LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
    set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "Qt major version used by qca-otr")
endif()

if(NOT QT_DEFAULT_MAJOR_VERSION MATCHES "^[56]$")
    message(FATAL_ERROR "qca-otr supports Qt 5 or Qt 6")
endif()

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Core)
set(QCA_PACKAGE_NAME "Qca3-qt${QT_DEFAULT_MAJOR_VERSION}")
find_package(${QCA_PACKAGE_NAME} CONFIG REQUIRED)

add_library(qca-otr STATIC
    src/ake.cpp
    src/akesession.cpp
    src/codec.cpp
    src/crypto.cpp
    src/data.cpp
    src/negotiation.cpp
    src/persistence.cpp
    src/profile.cpp
    src/session.cpp
    src/smp.cpp
    src/tlv.cpp
    src/transport.cpp
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
option(QCA_OTR_BUILD_LIBOTR_INTEROP_TESTS "Build tests against libotr 4.1.1" OFF)

if(QCA_OTR_BUILD_LIBOTR_INTEROP_TESTS AND NOT QCA_OTR_BUILD_TESTS)
    message(FATAL_ERROR "QCA_OTR_BUILD_LIBOTR_INTEROP_TESTS requires QCA_OTR_BUILD_TESTS")
endif()

if(QCA_OTR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
