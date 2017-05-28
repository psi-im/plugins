cmake_minimum_required( VERSION 2.8.12 )

set( plugins_list
	historykeeperplugin
	stopspamplugin
	juickplugin
	translateplugin
	gomokugameplugin
	attentionplugin
	cleanerplugin
	autoreplyplugin
	contentdownloaderplugin
	qipxstatusesplugin
	skinsplugin
	icqdieplugin
	captchaformsplugin
	watcherplugin
	videostatusplugin
	screenshotplugin
	jabberdiskplugin
	storagenotesplugin
	extendedoptionsplugin
	imageplugin
	extendedmenuplugin
	birthdayreminderplugin
	gmailserviceplugin
	gnupgplugin
	pepchangenotifyplugin
	otrplugin
	chessplugin
	conferenceloggerplugin
	messagefilterplugin
	enummessagesplugin
	httpuploadplugin
	imagepreviewplugin
)

if( NOT MSVC )
	set( plugins_list ${plugins_list}
		clientswitcherplugin
	)
endif()

if( "${BUILD_PLUGINS}" STREQUAL "ALL" )
	set ( plugins ${plugins_list} )
else()
	set ( plugins "${BUILD_PLUGINS}" )
endif()

foreach(plugin ${plugins_list})
	foreach(subdir ${plugins})
		if( ${plugin} STREQUAL ${subdir} )
			message("Parse subdirectory: ./${plugin}")
			add_subdirectory("./${plugin}")
		endif()
	endforeach()
endforeach()
