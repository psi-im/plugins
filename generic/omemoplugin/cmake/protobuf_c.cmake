cmake_minimum_required(VERSION 3.10.0)

set(ProtobufCGitRepo "https://github.com/protobuf-c/protobuf-c.git")

message(STATUS "Protobuf_C: using bundled")
set(PROTOBUF_C_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/protobuf-c)
set(PROTOBUF_C_BUILD_DIR ${PROTOBUF_C_PREFIX}/build)
set(Protobuf_C_INCLUDE_DIR ${PROTOBUF_C_PREFIX}/src/ProtobufCProject)
set(PROTOBUF_C_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf-c/src/ProtobufCProject/build-cmake)
set(Protobuf_C_LIBRARY ${PROTOBUF_C_BUILD_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf-c${D}${CMAKE_STATIC_LIBRARY_SUFFIX})
if(APPLE)
    set(COREFOUNDATION_LIBRARY "-framework CoreFoundation")
    set(COREFOUNDATION_LIBRARY_SECURITY "-framework Security")
    list(APPEND PROTOBUF_C_LIBRARY ${COREFOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY_SECURITY})
endif()

include(ExternalProject)
#set CMake options and transfer the environment to an external project

set(PROTOBUF_C_BUILD_OPTIONS
    -DCMAKE_IGNORE_PATH="/usr/include/protobuf-c"
    -DCMAKE_SYSTEM_IGNORE_PATH="/usr/include/protobuf-c"
    -DBUILD_SHARED_LIBS=OFF 
    -DBUILD_PROTOC=OFF
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${PROTOBUF_C_PREFIX}/build 
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} 
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DOSX_FRAMEWORK=OFF
)

include(FindGit)
find_package(Git REQUIRED)
ExternalProject_Add(ProtobufCProject
    PREFIX ${PROTOBUF_C_PREFIX}
    BINARY_DIR ${PROTOBUF_C_BUILD_DIR}
    GIT_REPOSITORY "${ProtobufCGitRepo}"
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -G${CMAKE_GENERATOR} ${PROTOBUF_C_BUILD_OPTIONS} ${PROTOBUF_C_SOURCE_DIR} 
    CMAKE_ARGS ${PROTOBUF_C_BUILD_OPTIONS}
    BUILD_BYPRODUCTS ${Protobuf_C_LIBRARY}
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)
