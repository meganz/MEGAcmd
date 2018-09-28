# For convenient building on windows, eg via generating Visual Studio projects and solutions.  Adjust the 'include' line to refer to your repo of MegaSDK

cmake_minimum_required(VERSION 3.8)

#specify where 3rd party libraries are available
set(Mega3rdPartyDir "${CMAKE_CURRENT_LIST_DIR}/../../../3rdparty")

# this line could also point to an sdk repo outside this one, or you can use the submodule in ./sdk folder
include(${CMAKE_CURRENT_LIST_DIR}/../../sdk/contrib/cmake/CMakeLists.txt)

set(ProjectDir "${CMAKE_CURRENT_LIST_DIR}/../..")

set (ENABLE_BACKUP 1 CACHE TYPE BOOL)

if (ENABLE_BACKUP)
    add_definitions( -DENABLE_BACKUPS )
endif (ENABLE_BACKUP)


add_executable(mega-cmd-server 
    "${ProjectDir}/src/megacmd.cpp"
    "${ProjectDir}/src/megacmdexecuter.cpp"
    "${ProjectDir}/src/megacmdlogger.cpp"
    "${ProjectDir}/src/megacmdsandbox.cpp"
    "${ProjectDir}/src/megacmdutils.cpp"
    "${ProjectDir}/src/comunicationsmanager.cpp"
    "${ProjectDir}/src/comunicationsmanagernamedpipes.cpp"
    "${ProjectDir}/src/comunicationsmanagerportsockets.cpp"
    "${ProjectDir}/src/configurationmanager.cpp"
    "${ProjectDir}/src/listeners.cpp"
)

add_executable(mega-exec 
    "${ProjectDir}/src/client/megacmdclient.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
)

add_executable(mega-cmd 
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshell.cpp"
)

target_link_libraries(mega-exec Mega )
target_link_libraries(mega-cmd Mega )
target_link_libraries(mega-cmd-server Mega Lz32.lib)
if (NOT NO_READLINE)
    target_link_libraries(mega-cmd readline)
endif (NOT NO_READLINE)


