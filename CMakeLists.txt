# CMakeLists.txt file to build the MEGAcmd
cmake_minimum_required(VERSION 3.16)

# Qt Creator configures VCPKG automatically. Disable it, we may want to use different tripplets, paths...
set(QT_CREATOR_SKIP_VCPKG_SETUP TRUE CACHE BOOL "")

## Modules location
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/build/cmake/modules) # Modules from MEGAcmd #TODO: place/fill these
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/sdk/contrib/cmake/modules) # Modules from MEGAsdk

set(VCPKG_ROOT "" CACHE PATH "If set, it will build and use the VCPKG packages defined in the manifest file")

## Configurable options ##
include(megacmd_options) #Load first MEGAcmd's options (that we have prevalescence over SDK (e.g libuv)
include(sdklib_options) #load default sdk's

message(STATUS "Using VCPKG_ROOT = ${VCPKG_ROOT}")

# Main modules location
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/sdk/contrib/cmake/modules)

if(VCPKG_ROOT)
    # Include VCPKG management tools.
    include(vcpkg_management)
    list(APPEND vcpkg_overlay ${CMAKE_CURRENT_LIST_DIR}/sdk/contrib/cmake) # MEGAsdk overlays
    process_vcpkg_libraries("${vcpkg_overlay}") # Choose and build libraries depending on the configured options.
else()
    # For packages with no pkg-config in the system.
    list(APPEND CMAKE_MODULE_PATH sdk/contrib/cmake/modules/packages)
    message(STATUS "Using system dependencies")
endif()

project(MEGAcmd
    VERSION 1.7.0
    DESCRIPTION "MEGAcmd"
    )

# In-source build not allowed
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source build is not allowed. Remove CMakeCache.txt and the CMakeFiles directory and set a new binary directory different than the source tree.")
endif()


message(STATUS "Building MEGAcmd v${PROJECT_VERSION}")

#utilities/helper functions
include(GNUInstallDirs) # Values for installation directories. All platforms
include(CMakePackageConfigHelpers) # For the CMake package
include(target_sources_conditional) # To add files to the project without building them
include(target_platform_compile_options) # To add compile options depeding on the platform

# Load SDK project to build sdk library
add_subdirectory(sdk)

#LOAD MEGACMD modules:
include(megacmd_configuration) ## Load global CMake configuration for the project
include(megacmd_libraries) # to load libraries dependencies by target

#TODO: CONSIDER SPLITTING TARGET IN FILES and have add_subdirectory(src) , ...

set(ProjectDir "${CMAKE_CURRENT_LIST_DIR}")


#TODO: Move this to option too
set (ENABLE_BACKUP 1 CACHE STRING "")
if (ENABLE_BACKUP)
    add_definitions( -DENABLE_BACKUPS )
endif (ENABLE_BACKUP)


add_library(LMegacmdServer STATIC
    "${ProjectDir}/src/megacmd.cpp"
    "${ProjectDir}/src/megacmdexecuter.cpp"
    "${ProjectDir}/src/megacmd_events.cpp"
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
)

# Given we are no longer including the sdk, and some sources use certain defines without including SDK's config.h, 
# We need to explicitly pass the copmiling options.
# TODO: REVIEW THE BELLOW LIST: a hint:
# megacmd/src$ grep ifdef `find . -name "*.cpp"` | awk -F : '{print $NF}' | awk '{$1=$1};1' | sort  | uniq | grep -v "WIN32\|WINDOWS_PHONE\|USE_IOS\|MINGW32\|WIN64\|__linux__\|__MACH\|__APPLE\|__FreeBSD"
# or without megacmdexecuter (that eventually includes sdk's config.h):
#ifdef ECONNREFUSED #comming from libs includes
#ifdef ENABLE_BACKUPS
#ifdef ENABLE_CHAT
#ifdef HAVE_DOWNLOADS_COMMAND #no
#ifdef HAVE_LIBUV
#ifdef MEGACMD_DEPRECATED_OS #TODO: THIS ONE SHOULD BE defined by debian.rules and others
#ifdef NO_READLINE #not for server, handled for mega-cmd (Shell)
#ifdef _TRUNCATE #win internal stuff, coming from compiler/libs includes
#ifdef USE_PCRE
#ifdef USE_PORT_COMMS #no apply/deprecated

target_compile_definitions(LMegacmdServer
    PUBLIC
    $<$<BOOL:${ENABLE_CHAT}>:ENABLE_CHAT>
    $<$<BOOL:${ENABLE_BACKUPS}>:ENABLE_BACKUPS>
    $<$<BOOL:${USE_LIBUV}>:HAVE_LIBUV>
    $<$<BOOL:${USE_PCRE}>:USE_PCRE>
    $<$<CONFIG:Debug>:DEBUG,NDEBUG>
)


add_executable(mega-cmd-server
    "${ProjectDir}/src/megacmd_server_main.cpp"
    "${ProjectDir}/contrib/QtCreator/MEGAcmd/MEGAcmdServer/icon.rc"
)

add_library(LMegacmdClient STATIC
    "${ProjectDir}/src/client/megacmdclient.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdcommonutils.cpp"
)

add_executable(mega-exec
    "${ProjectDir}/src/client/megacmd_client_main.cpp"
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

#Test Common:
add_library(LMegacmdTestsCommon STATIC
    "${ProjectDir}/tests/common/Instruments.cpp"
    "${ProjectDir}/tests/common/TestUtils.cpp"
)

#Integration tests:
add_executable(mega-cmd-tests-integration
    "${ProjectDir}/tests/integration/BasicTests.cpp"
    "${ProjectDir}/tests/integration/ExportTests.cpp"
    "${ProjectDir}/tests/integration/MegaCmdTestingTools.cpp"
    "${ProjectDir}/tests/integration/main.cpp"
)
target_include_directories(mega-cmd-tests-integration PUBLIC ${ProjectDir}/src ${ProjectDir}/tests/common)
target_link_libraries(mega-cmd-tests-integration PUBLIC LMegacmdServer LMegacmdClient LMegacmdTestsCommon)
if(APPLE)
    target_link_libraries(mega-cmd-tests-integration PRIVATE "-framework Security" )
endif()

#Unit tests:
add_executable(mega-cmd-tests-unit
    "${ProjectDir}/tests/unit/StringUtilsTests.cpp"
    "${ProjectDir}/tests/unit/UtilsTests.cpp"
    "${ProjectDir}/tests/unit/PlatformDirectoriesTest.cpp"
    "${ProjectDir}/tests/unit/main.cpp"
)

if(APPLE)
    target_link_libraries(mega-cmd-tests-unit PRIVATE "-framework Security" )
endif()

if (APPLE)
    target_sources(LMegacmdServer  PRIVATE "${ProjectDir}/src/megacmdplatform.mm")
    target_sources(mega-cmd-updater PRIVATE "${ProjectDir}/src/updater/MacUtils.mm")
endif ()

if (WIN32)
set_target_properties(mega-exec PROPERTIES OUTPUT_NAME MEGAclient)
set_target_properties(mega-cmd PROPERTIES OUTPUT_NAME MEGAcmdShell)
set_target_properties(mega-cmd-updater PROPERTIES OUTPUT_NAME MEGAcmdUpdater)
set_target_properties(mega-cmd-server PROPERTIES OUTPUT_NAME MEGAcmdServer)
set_target_properties(mega-cmd-server  PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE /DEBUG" )

#TODO: if this is still required, these paths will need adjusting
#set(3RDPARTY_RUNTIME_PATH_DEBUG "PATH=%PATH%" "${Mega3rdPartyDir}/vcpkg/installed/${VCPKG_TRIPLET}/debug/bin")
#set(3RDPARTY_RUNTIME_PATH_RELEASE "PATH=%PATH%" "${Mega3rdPartyDir}/vcpkg/installed/${VCPKG_TRIPLET}/bin")
#set_target_properties(mega-exec        PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
#set_target_properties(mega-cmd         PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
#set_target_properties(mega-cmd-updater PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
#set_target_properties(mega-cmd-server  PROPERTIES VS_DEBUGGER_ENVIRONMENT "${3RDPARTY_RUNTIME_PATH_DEBUG}")
endif()

target_link_libraries(LMegacmdClient PUBLIC MEGA::SDKlib)
target_link_libraries(mega-exec LMegacmdClient)
target_link_libraries(mega-cmd PUBLIC MEGA::SDKlib)
target_link_libraries(mega-cmd-updater PUBLIC MEGA::SDKlib)

target_link_libraries(LMegacmdServer PUBLIC MEGA::SDKlib)

if (WIN32)
  target_link_libraries(LMegacmdServer PUBLIC Lz32.lib Taskschd.lib)
  target_link_libraries(mega-cmd-updater PUBLIC Lz32.lib Urlmon.lib)
endif()
target_link_libraries(mega-cmd-server PUBLIC LMegacmdServer)
if (NOT NO_READLINE)
    target_link_libraries(mega-cmd PRIVATE ${readline_LIBRARIES})
endif (NOT NO_READLINE)

target_include_directories(mega-cmd-tests-unit PUBLIC ${ProjectDir}/src ${ProjectDir}/tests/common)
target_link_libraries(mega-cmd-tests-unit PUBLIC LMegacmdServer LMegacmdTestsCommon)



# Load 3rd parties #TODO: consider splitting by target?
load_megacmdserver_libraries()

