cmake_minimum_required(VERSION 3.10.0)

set(OmemoCGitRepo "https://github.com/psi-im/libomemo-c.git")

message(STATUS "OMEMO_C: using bundled")
set(OMEMO_C_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/omemo-c)
set(OMEMO_C_BUILD_DIR ${OMEMO_C_PREFIX}/build)
set(OMEMO_C_INCLUDE_DIR ${OMEMO_C_PREFIX}/src/OmemoCProject/src)
set(OMEMO_C_LIBRARY ${OMEMO_C_BUILD_DIR}/src/${CMAKE_STATIC_LIBRARY_PREFIX}omemo-c${CMAKE_STATIC_LIBRARY_SUFFIX})
set(Protobuf_C_LIBRARY ${OMEMO_C_BUILD_DIR}/src/protobuf-c/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf-c${CMAKE_STATIC_LIBRARY_SUFFIX})
if(APPLE)
    set(COREFOUNDATION_LIBRARY "-framework CoreFoundation")
    set(COREFOUNDATION_LIBRARY_SECURITY "-framework Security")
    list(APPEND OMEMO_C_LIBRARY ${COREFOUNDATION_LIBRARY} ${COREFOUNDATION_LIBRARY_SECURITY})
endif()

include(FindGit)
find_package(Git REQUIRED)

include(ExternalProject)
#set CMake options and transfer the environment to an external project
set(OMEMO_C_BUILD_OPTIONS
    -DBUILD_SHARED_LIBS=OFF
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${OMEMO_C_PREFIX}/build 
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DBUILD_WITH_PROTOBUF=bundled
    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} 
    -DOSX_FRAMEWORK=OFF
)

ExternalProject_Add(OmemoCProject
    PREFIX ${OMEMO_C_PREFIX}
    BINARY_DIR ${OMEMO_C_BUILD_DIR}
    GIT_REPOSITORY "${OmemoCGitRepo}"
    GIT_TAG omemo
    CMAKE_ARGS ${OMEMO_C_BUILD_OPTIONS}
    BUILD_BYPRODUCTS ${OMEMO_C_LIBRARY} ${Protobuf_C_LIBRARY}
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    DEPENDS ${DEPENDS}
)
