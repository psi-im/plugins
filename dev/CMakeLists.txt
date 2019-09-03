cmake_minimum_required( VERSION 3.1.0 )

set( plugins_list 
    battleshipgameplugin
    ripperccplugin
)
if( "${BUILD_PLUGINS}" STREQUAL "ALL" )
    set( plugins ${plugins_list} )
else( "${BUILD_PLUGINS}" STREQUAL "ALL" )
    set( plugins "${BUILD_PLUGINS}" )
endif( "${BUILD_PLUGINS}" STREQUAL "ALL" )
message(STATUS "Plugins from dev subdirectory")
foreach( plugin ${plugins_list} )
    foreach( subdir ${plugins} )
        if( ${plugin} STREQUAL ${subdir} )
            message(STATUS "Add subdirectory: ${plugin}")
            add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
        endif( ${plugin} STREQUAL ${subdir} )
    endforeach(subdir)
endforeach(plugin)
