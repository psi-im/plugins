cmake_minimum_required(VERSION 3.1.0)

#set( deprecated_list
#    captchaformsplugin
#    gmailserviceplugin
#)

set( plugins_list
    attentionplugin
    autoreplyplugin
    birthdayreminderplugin
    chessplugin
    cleanerplugin
    conferenceloggerplugin
    contentdownloaderplugin
    enummessagesplugin
    extendedmenuplugin
    extendedoptionsplugin
    gnupgplugin
    gomokugameplugin
    historykeeperplugin
    httpuploadplugin
    icqdieplugin
    imageplugin
    imagepreviewplugin
    jabberdiskplugin
    juickplugin
    messagefilterplugin
    omemoplugin
    otrplugin
    pepchangenotifyplugin
    qipxstatusesplugin
    skinsplugin
    stopspamplugin
    storagenotesplugin
    translateplugin
    watcherplugin
)

if(NOT HAIKU)
    # qxt library is buildable only in Linux, *BSD, Windows and macOS
    list(APPEND plugins_list
        screenshotplugin
    )
endif()

if(NOT APPLE AND NOT HAIKU)
    list(APPEND plugins_list
        videostatusplugin
    )
endif()

#if( NOT MSVC )
    list(APPEND plugins_list
        clientswitcherplugin
    )
#endif()

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
