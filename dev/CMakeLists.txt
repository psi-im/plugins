cmake_minimum_required( VERSION 3.10.0 )

set( plugins_list 
    battleshipgameplugin
    noughtsandcrossesplugin
)

message(STATUS "Plugins from dev subdirectory")
# If BUILD_PLUGINS variable was set as "-plugin1;-plugin2"
# We proceed all plugins with plugin1 and plugin2 excluded
string(REGEX MATCHALL "[-][^;]+" DISABLED_PLUGINS "${BUILD_PLUGINS}")
if(NOT "${DISABLED_PLUGINS}" STREQUAL "")
    string(REPLACE "-" "" DISABLED_LIST "${DISABLED_PLUGINS}")
    set(plugins ${plugins_list})
    foreach(dp ${DISABLED_LIST})
        list(FIND plugins "${dp}" _IS_FOUND)
        if(${_IS_FOUND} GREATER_EQUAL 0)
            message(STATUS "Exclude subdirectory: ${dp}")
            list(REMOVE_ITEM plugins "${dp}")
        endif()
        unset(_IS_FOUND)
    endforeach()
else()
    if( "${BUILD_PLUGINS}" STREQUAL "ALL" )
        set ( plugins ${plugins_list} )
    else()
        set ( plugins "${BUILD_PLUGINS}" )
    endif()
endif()

foreach(plugin ${plugins})
    list(FIND plugins_list "${plugin}" _IS_FOUND)
    if(${_IS_FOUND} GREATER_EQUAL 0)
        message(STATUS "Add subdirectory: ${plugin}")
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
    else()
        message(WARNING "Plugin ${plugin} set by BUILD_PLUGINS variable not found")
    endif()
    unset(_IS_FOUND)
endforeach()
