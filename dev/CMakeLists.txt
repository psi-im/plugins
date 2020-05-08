cmake_minimum_required( VERSION 3.1.0 )

set( plugins_list 
    battleshipgameplugin
)

message(STATUS "Plugins from dev subdirectory")
# If BUILD_PLUGINS variable was set as "-plugin1;-plugin2"
# We proceed all plugins with plugin1 and plugin2 excluded
string(REGEX MATCH ".*[-]*" DISABLED_PLUGINS "${BUILD_PLUGINS}")
if(NOT "${DISABLED_PLUGINS}" STREQUAL "")
    set(plugins ${plugins_list})
    foreach(mp ${BUILD_PLUGINS})
        string(SUBSTRING "${mp}" 0 1 FIRST_LETTER)
        string(SUBSTRING ${mp} 1 -1 PLUGIN_BODY)
        if(${FIRST_LETTER} STREQUAL "-")
            message(STATUS "Exclude subdirectory: ${PLUGIN_BODY}")
            list(REMOVE_ITEM plugins "${PLUGIN_BODY}")
        endif()
    endforeach()
else()
    if( "${BUILD_PLUGINS}" STREQUAL "ALL" )
        set ( plugins ${plugins_list} )
    else()
        set ( plugins "${BUILD_PLUGINS}" )
    endif()
endif()

foreach( plugin ${plugins_list} )
    foreach( subdir ${plugins} )
        if( ${plugin} STREQUAL ${subdir} )
            message(STATUS "Add subdirectory: ${plugin}")
            add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
        endif()
    endforeach()
endforeach()
