# For convenient building on windows, eg via generating Visual Studio projects and solutions.  Adjust the 'include' line to refer to your repo of MegaSDK

cmake_minimum_required(VERSION 3.8)
project(MEGAcmd)

# Turn on SDK flags which are off by default in the SDK, but should be on for MEGAcmd
set (USE_LIBUV 1 CACHE STRING "Includes the library and turns on internal web and ftp server functionality")
set (USE_PDFIUM 1 CACHE STRING "Used to create previews/thumbnails for PDF files")

if(WIN32)
    add_definitions( -DUNICODE -D_UNICODE )  # needed for visual studio projects to use the unicode runtime libraries

    #supported windows version: 7 and beyond
    add_definitions( -DNTDDI_VERSION=NTDDI_WIN7 )
    add_definitions( -D_WIN32_WINNT=0x0601 ) # 0601: windows 7
endif()

#specify where 3rd party libraries are available
set(Mega3rdPartyDir "${CMAKE_CURRENT_LIST_DIR}/../../../3rdparty" CACHE STRING "")

# this line could also point to an sdk repo outside this one, or you can use the submodule in ./sdk folder
include(${CMAKE_CURRENT_LIST_DIR}/../../sdk/contrib/cmake/CMakeLists.txt)

set(ProjectDir "${CMAKE_CURRENT_LIST_DIR}/../..")

set (ENABLE_BACKUP 1 CACHE STRING "")

if (ENABLE_BACKUP)
    add_definitions( -DENABLE_BACKUPS )
endif (ENABLE_BACKUP)


add_executable(mega-cmd-server
    "${ProjectDir}/src/megacmd.cpp"
    "${ProjectDir}/src/megacmdexecuter.cpp"
    "${ProjectDir}/src/megacmdlogger.cpp"
    "${ProjectDir}/src/megacmdsandbox.cpp"
    "${ProjectDir}/src/megacmdutils.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
    "${ProjectDir}/src/comunicationsmanager.cpp"
    "${ProjectDir}/src/comunicationsmanagerfilesockets.cpp"
    "${ProjectDir}/src/comunicationsmanagernamedpipes.cpp"
    "${ProjectDir}/src/comunicationsmanagerportsockets.cpp"
    "${ProjectDir}/src/configurationmanager.cpp"
    "${ProjectDir}/src/listeners.cpp"
)

add_executable(mega-exec
    "${ProjectDir}/src/client/megacmdclient.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
)

add_executable(mega-cmd
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshell.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
)

add_executable(mega-cmd-updater WIN32
    "${ProjectDir}/src/updater/MegaUpdater.cpp"
    "${ProjectDir}/src/updater/UpdateTask.cpp"
)

if (WIN32)
set_target_properties(mega-exec PROPERTIES OUTPUT_NAME MEGAclient)
set_target_properties(mega-cmd PROPERTIES OUTPUT_NAME MEGAcmdShell)
set_target_properties(mega-cmd-updater PROPERTIES OUTPUT_NAME MEGAcmdUpdater)
set_target_properties(mega-cmd-server PROPERTIES OUTPUT_NAME MEGAcmdServer)
endif()


target_link_libraries(mega-exec Mega)
target_link_libraries(mega-cmd Mega)
target_link_libraries(mega-cmd-updater Mega)
if (WIN32)
  target_link_libraries(mega-cmd-server Mega Lz32.lib Taskschd.lib)
  target_link_libraries(mega-cmd-updater Mega Lz32.lib Urlmon.lib)
else()
  target_link_libraries(mega-cmd-server Mega)
endif()
if (NOT NO_READLINE)
    target_link_libraries(mega-cmd readline)
endif (NOT NO_READLINE)
