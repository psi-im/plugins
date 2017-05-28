cmake_minimum_required( VERSION 2.8.11 ) 

set( plugins_list 
	battleshipgameplugin
	ripperccplugin
)
if( "${BUILD_PLUGINS}" STREQUAL "ALL" )
	set( plugins ${plugins_list} )
else( "${BUILD_PLUGINS}" STREQUAL "ALL" )
	set( plugins "${BUILD_PLUGINS}" )
endif( "${BUILD_PLUGINS}" STREQUAL "ALL" )
foreach( plugin ${plugins_list} )
	foreach( subdir ${plugins} )
		if( ${plugin} STREQUAL ${subdir} )
			message("Parse subdirectory: ./${plugin}")
			add_subdirectory("./${plugin}")
		endif( ${plugin} STREQUAL ${subdir} )
	endforeach(subdir)
endforeach(plugin)
