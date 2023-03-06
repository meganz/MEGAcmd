# For convenient building on windows, eg via generating Visual Studio projects and solutions.  Adjust the 'include' line to refer to your repo of MegaSDK

cmake_minimum_required(VERSION 3.8)
project(MEGAcmd)

# Turn on SDK flags which might be off by default in the SDK, but should be on for MEGAcmd, also set to 0 flags we want to be aware of but not use yet
set (USE_LIBUV 1 CACHE STRING "Includes the library and turns on internal web and ftp server functionality")
set (USE_PDFIUM 1 CACHE STRING "Used to create previews/thumbnails for PDF files")
set (USE_MEDIAINFO 1 CACHE STRING "Used to determine media properties and set those as node attributes")
set (USE_FREEIMAGE 1 CACHE STRING "Used to create previews/thumbnails for photos/pictures")
set (ENABLE_SYNC 1 CACHE STRING "Turns on sync functionality")
set (ENABLE_CHAT 1 CACHE STRING "Turns on chat management functionality")
set (ENABLE_LOG_PERFORMANCE 0 CACHE STRING "Faster log message generation")
set (USE_ROTATIVEPERFORMANCELOGGER 0 CACHE STRING "Internal logger, not just callbacks")
set (HAVE_FFMPEG 1 CACHE STRING "Used to create previews/thumbnails for video files")
set (USE_PCRE 1 CACHE STRING "Provides pattern matching functionality for sync rules or flie listings")

# should it be have, or use?  Use is our usual pattern.  Turn on both for surety
set (HAVE_FFMPEG 1 CACHE STRING "Used to create previews/thumbnails for video files")
set (USE_FFMPEG 1 CACHE STRING "Used to create previews/thumbnails for video files")

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
    "${ProjectDir}/src/megacmdtransfermanager.cpp"
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
    "${ProjectDir}/contrib/QtCreator/MEGAcmd/MEGAcmdServer/icon.rc"
)

add_executable(mega-exec
    "${ProjectDir}/src/client/megacmdclient.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
    "${ProjectDir}/contrib/QtCreator/MEGAcmd/MEGAcmdClient/icon.rc"
)

add_executable(mega-cmd
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshell.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
    "${ProjectDir}/contrib/QtCreator/MEGAcmd/MEGAcmdShell/icon.rc"
)

add_executable(mega-cmd-updater WIN32
    "${ProjectDir}/src/updater/MegaUpdater.cpp"
    "${ProjectDir}/src/updater/UpdateTask.cpp"
    "${ProjectDir}/contrib/QtCreator/MEGAcmd/MEGAcmdUpdater/icon.rc"
)

if (APPLE)
    target_sources(mega-cmd-server  PRIVATE "${ProjectDir}/src/megacmdplatform.mm")
    target_sources(mega-cmd-updater PRIVATE "${ProjectDir}/src/updater/MacUtils.mm")
endif ()

if (WIN32)
set_target_properties(mega-exec PROPERTIES OUTPUT_NAME MEGAclient)
set_target_properties(mega-cmd PROPERTIES OUTPUT_NAME MEGAcmdShell)
set_target_properties(mega-cmd-updater PROPERTIES OUTPUT_NAME MEGAcmdUpdater)
set_target_properties(mega-cmd-server PROPERTIES OUTPUT_NAME MEGAcmdServer)
set_target_properties(mega-cmd-server  PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE /DEBUG" )

set(3RDPARTY_RUNTIME_PATH_DEBUG "PATH=%PATH%" "${Mega3rdPartyDir}/vcpkg/installed/${VCPKG_TRIPLET}/debug/bin")
set(3RDPARTY_RUNTIME_PATH_RELEASE "PATH=%PATH%" "${Mega3rdPartyDir}/vcpkg/installed/${VCPKG_TRIPLET}/bin")
set_target_properties(mega-exec        PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
set_target_properties(mega-cmd         PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
set_target_properties(mega-cmd-updater PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
set_target_properties(mega-cmd-server  PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
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
