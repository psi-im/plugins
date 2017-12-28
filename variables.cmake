set(PLUGINS_ROOT_DIR "." CACHE STRING "Plugins root path. Path where include directory placed")

if( NOT WIN32 )
    set( LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
    set( PLUGINS_PATH "lib${LIB_SUFFIX}/psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
else()
    set( PLUGINS_PATH "psi-plus/plugins" CACHE STRING "Install suffix for plugins" )
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        set(DEFAULT_DEBUG_FLAG "/ENTRY:mainCRTStartup /DEBUG /INCREMENTAL /SAFESEH:NO /MANIFEST:NO")
        set(DEFAULT_LINKER_FLAG "/ENTRY:mainCRTStartup /INCREMENTAL:NO /LTCG")
        set (CMAKE_MODULE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL /SAFESEH:NO /MANIFEST:NO" CACHE STRING "" FORCE)
        set (CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL "/INCREMENTAL:NO /LTCG" CACHE STRING "" FORCE)
        set (CMAKE_MODULE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /LTCG" CACHE STRING "" FORCE)
        set (CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /MANIFEST:NO" CACHE STRING "" FORCE)
        set (CMAKE_SHARED_LINKER_FLAGS_DEBUG "${DEFAULT_DEBUG_FLAG}" CACHE STRING "" FORCE)
        set (CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${DEFAULT_LINKER_FLAG}" CACHE STRING "" FORCE)
        set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${DEFAULT_LINKER_FLAG}" CACHE STRING "" FORCE)
        set (CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${DEFAULT_DEBUG_FLAG}" CACHE STRING "" FORCE)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /MTd")
        add_definitions(-DNOMINMAX)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
        add_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
        add_definitions(-D_UNICODE)
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
    endif()
    add_definitions( -DQ_OS_WIN )
endif()

add_definitions( -DQT_PLUGIN -DHAVE_QT5 )

set( CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${PLUGINS_ROOT_DIR}/cmake/modules
    ${PLUGINS_ROOT_DIR}/../../cmake/modules
    ${CMAKE_CURRENT_LIST_DIR}/cmake/modules
)
