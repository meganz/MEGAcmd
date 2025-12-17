# CMakeLists.txt file to build the MEGAcmd
cmake_minimum_required(VERSION 3.16)

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/sdk
    OUTPUT_VARIABLE SDK_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(MEGACMD_MAJOR_VERSION 2)
set(MEGACMD_MINOR_VERSION 3)
set(MEGACMD_MICRO_VERSION 0)

## Configure megacmdversion.h
configure_file("${CMAKE_CURRENT_LIST_DIR}/src/megacmdversion.h.in" "${CMAKE_CURRENT_LIST_DIR}/src/megacmdversion.h" @ONLY)
if (APPLE)
configure_file("${CMAKE_CURRENT_LIST_DIR}/build/installer/Info.plist.in" "${CMAKE_CURRENT_LIST_DIR}/build/installer/Info.plist" @ONLY)
endif()

# Qt Creator configures VCPKG automatically. Disable it, we may want to use different tripplets, paths...
set(QT_CREATOR_SKIP_VCPKG_SETUP TRUE CACHE BOOL "")

## Modules location
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/build/cmake/modules) # Modules from MEGAcmd
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/sdk/cmake/modules) # Modules from MEGAsdk

set(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../vcpkg" CACHE PATH "If set, it will build and use the VCPKG packages defined in the manifest file")

include(detect_host_architecture)

if (WIN32)
execute_process(
    COMMAND bash --version
    RESULT_VARIABLE BASH_VERSION_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)
endif()

if((NOT WIN32 OR BASH_VERSION_RESULT EQUAL 0) AND NOT EXISTS ${VCPKG_ROOT})
    message(STATUS "vcpkg will be cloned into ${VCPKG_ROOT}")
    execute_process(
        #TODO: have the same for windows ... or at least check if bash is available
        COMMAND "bash" "-x" "${CMAKE_CURRENT_LIST_DIR}/build/clone_vcpkg_from_baseline.sh" "${VCPKG_ROOT}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        ERROR_VARIABLE resulte
        OUTPUT_VARIABLE result
        RESULT_VARIABLE status
    )
    if(NOT status EQUAL "0")
        message(FATAL_ERROR "Failed to run the clone_vcpkg_from_baseline. ${status} ${result} ${resulte} ")
    endif()
    message(STATUS "vcpkg cloned successfully: ${status}")
endif()

#TODO: Review the following, move to a separate module and add add_executable WIN32/MACOSX_BUNDLE properties
# Set min OSX version
if(CMAKE_HOST_APPLE)
    # Minimum deployment target differs if we are building for intel or arm64 targets
    # CMAKE_SYSTEM_PROCESSOR and CMAKE_HOST_SYSTEM_PROCESSOR are only available after project()
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE HOST_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Setup CMAKE_OSX_DEPLOYMENT_TARGET before project()
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR (NOT CMAKE_OSX_ARCHITECTURES AND HOST_ARCHITECTURE STREQUAL "arm64"))
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.1" CACHE STRING "Minimum OS X deployment version")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
    endif()
    message(STATUS "Minimum OS X deployment version is set to ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    unset(HOST_ARCHITECTURE)
endif()
if(WIN32)
    add_definitions( -DUNICODE -D_UNICODE )  # needed for visual studio projects to use the unicode runtime libraries

    #supported windows version: 8 and beyond
    add_definitions( -D_WIN32_WINNT=0x0602 ) # 0602: windows 8. Note: NTDDI_VERSIO & WINVER shall be inferred from those by Win SDK
    add_definitions( -DNO_READLINE )  # This one is defined within SdkLib, but we need it too for MEGAcmd code.
    add_definitions( -DUSE_CPPTHREAD ) # For win32, we need this to be defined explicitly (used directly in different targets). TODO: remove in CMD-318
    add_definitions( -DNOMINMAX ) # To fix usages of std::min / std::max
endif()

## Configurable options ##
include(megacmd_options) #Load first MEGAcmd's options (that we have prevalescence over SDK (e.g libuv)
include(sdklib_options) #load default sdk's

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/sdk/include/mega/config.h)
    file(RENAME ${CMAKE_CURRENT_LIST_DIR}/sdk/include/mega/config.h ${CMAKE_CURRENT_LIST_DIR}/sdk/include/mega/config.h_non_cmake_bk)
endif()

message(STATUS "Using VCPKG_ROOT = ${VCPKG_ROOT}")

if(VCPKG_ROOT)
    if (ENABLE_MEGACMD_TESTS)
        list(APPEND VCPKG_MANIFEST_FEATURES "megacmd-enable-tests")
    endif()

    # Include VCPKG management tools.
    include(vcpkg_management)
    list(APPEND vcpkg_overlay ${CMAKE_CURRENT_LIST_DIR}/sdk/cmake) # MEGAsdk overlays
    process_vcpkg_libraries("${vcpkg_overlay}") # Choose and build libraries depending on the configured options.
else()
    # For packages with no pkg-config in the system.
    list(APPEND CMAKE_MODULE_PATH sdk/cmake/modules/packages)
    message(STATUS "Using system dependencies")
endif()

project(MEGAcmd
    VERSION ${MEGACMD_MAJOR_VERSION}.${MEGACMD_MINOR_VERSION}.${MEGACMD_MICRO_VERSION}
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

if(UNIX AND NOT APPLE)
    # Set rpath and location for dirs accordingly:
    # If CMAKE_INSTALL_PREFIX is set (not default), it will set rpath to to such prefix plus /opt/....
    # If CMAKE_INSTALL_PREFIX is not set, it will set rpath to /opt/....
    # Note: using cmake --install --prefix /some/prefix will not set rpath relative to that prefix
    # The above can be used for building packages: in which install dir is a path construction folder that will not be there in packages
    set(CMAKE_INSTALL_LIBDIR "opt/megacmd/lib")
    set(CMAKE_INSTALL_BINDIR "usr/bin") #override default "bin"

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        message(STATUS "Overriding default CMAKE_INSTALL_PREFIX to /")
        set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "Default install path" FORCE) # override default /usr/local
        set(RPATH_FOR_DYNAMIC_LIBS "/${CMAKE_INSTALL_LIBDIR}")
    else()
        # If explicit cmake prefix at cmake call time, make rpath relative to install dir
        set(RPATH_FOR_DYNAMIC_LIBS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    list( APPEND CMAKE_INSTALL_RPATH "${RPATH_FOR_DYNAMIC_LIBS}")
endif()
if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    get_filename_component(ABSOLUTE_RPATH_LIBS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} ABSOLUTE)
    list( APPEND CMAKE_INSTALL_RPATH ${ABSOLUTE_RPATH_LIBS} )
    #set(CMAKE_INSTALL_RPATH ${ABSOLUTE_RPATH_LIBS})
    message(STATUS "Added CMAKE_INSTALL_LIBDIR=${ABSOLUTE_RPATH_LIBS} to rpath: ${CMAKE_INSTALL_RPATH}")
endif()

# Load SDK project to build sdk library
add_subdirectory(sdk)

#LOAD MEGACMD modules:
include(megacmd_configuration) ## Load global CMake configuration for the project
include(megacmd_libraries) # to load libraries dependencies by target
include(megacmd_utility_functions) # utility functions

#TODO: CONSIDER SPLITTING TARGET IN FILES and have add_subdirectory(src) , ... See MR !682

set(ProjectDir "${CMAKE_CURRENT_LIST_DIR}")

# Populate list of MEGAcmd source files (used by logger):
generate_src_file_list("${ProjectDir}/sdk" SDK_SRCS)
generate_src_file_list("${ProjectDir}/src" MEGACMD_SRCS)
foreach(CMDFILE ${MEGACMD_SRCS})
    if (CMDFILE IN_LIST SDK_SRCS)
        message(FATAL_ERROR "MEGAcmd src filename ${CMDFILE} clashing with SDK's: please, pick a new one")
    endif()
endforeach()
string(JOIN ", " MEGACMD_SRC_FILE_LIST ${MEGACMD_SRCS})
configure_file("${ProjectDir}/src/megacmd_src_file_list.h.in" "${ProjectDir}/src/megacmd_src_file_list.h" @ONLY)


add_library(LMEGAcmdCommonUtils STATIC)
add_source_and_corresponding_header_to_target(LMEGAcmdCommonUtils
    PUBLIC
    "${ProjectDir}/src/megacmdcommonutils.cpp"
    "${ProjectDir}/src/megacmd_utf8.cpp"
)

add_library(LMegacmdServer STATIC)
add_source_and_corresponding_header_to_target(LMegacmdServer
    PUBLIC
    "${ProjectDir}/src/megacmd.cpp"
    "${ProjectDir}/src/megacmdexecuter.cpp"
    "${ProjectDir}/src/megacmd_events.cpp"
    "${ProjectDir}/src/megacmdlogger.cpp"
    "${ProjectDir}/src/megacmdsandbox.cpp"
    "${ProjectDir}/src/megacmdutils.cpp"
    "${ProjectDir}/src/comunicationsmanager.cpp"
    "${ProjectDir}/src/comunicationsmanagerfilesockets.cpp"
    "${ProjectDir}/src/comunicationsmanagernamedpipes.cpp"
    "${ProjectDir}/src/configurationmanager.cpp"
    "${ProjectDir}/src/listeners.cpp"
    "${ProjectDir}/src/sync_command.cpp"
    "${ProjectDir}/src/sync_issues.cpp"
    "${ProjectDir}/src/sync_ignore.cpp"
    "${ProjectDir}/src/megacmd_rotating_logger.cpp"
    "${ProjectDir}/src/megacmd_fuse.cpp"
)

target_sources_conditional(LMegacmdServer
    FLAG APPLE
    PRIVATE
    "${ProjectDir}/src/megacmdplatform.h"
    "${ProjectDir}/src/megacmdplatform.mm"
)

# Given we are no longer including the sdk, and some sources use certain defines without including SDK's config.h,
# We need to explicitly pass the compiling options.
target_compile_definitions(LMegacmdServer
    PUBLIC
    $<$<BOOL:${USE_PCRE}>:USE_PCRE>
)

if (NOT WIN32)
    if (ENABLE_ASAN)
        add_compile_options("-fsanitize=address" "-fno-omit-frame-pointer" "-fno-common")
        link_libraries("-fsanitize=address")
    endif()

    if (ENABLE_UBSAN)
        add_compile_options("-fsanitize=undefined" "-fno-omit-frame-pointer")
        link_libraries("-fsanitize=undefined")
    endif()

    if (ENABLE_TSAN)
        add_compile_options("-fsanitize=thread" "-fno-omit-frame-pointer")
        link_libraries("-fsanitize=thread")
    endif()
endif()

if (APPLE)
    set(executablesType MACOSX_BUNDLE)
endif()

add_executable(mega-cmd-server ${executablesType})
if (WIN32)
    set(MEGACMD_RESOURCE_NAME MEGAcmdServer)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/build/installer/winversion.rc.in" "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-server_version.rc" @ONLY)
    set(RESOURCE_FILES_MEGACMD_SERVER "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-server_version.rc")

    set(MEGACMD_RESOURCE_NAME MEGAclient)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/build/installer/winversion.rc.in" "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-client_version.rc" @ONLY)
    set(RESOURCE_FILES_MEGACMD_CLIENT "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-client_version.rc")

    set(MEGACMD_RESOURCE_NAME MEGAcmdShell)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/build/installer/winversion.rc.in" "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-shell_version.rc" @ONLY)
    set(RESOURCE_FILES_MEGACMD_SHELL "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-shell_version.rc")

    set(MEGACMD_RESOURCE_NAME MEGAcmdUpdater)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/build/installer/winversion.rc.in" "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-updater_version.rc" @ONLY)
    set(RESOURCE_FILES_MEGACMD_UPDATER "${CMAKE_CURRENT_LIST_DIR}/build/installer/mega-cmd-updater_version.rc")
elseif (APPLE)
    set(RESOURCE_FILES_MEGACMD_SERVER
        ${CMAKE_CURRENT_SOURCE_DIR}/build/installer/app.icns
    )

    set_target_properties(mega-cmd-server PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/build/installer/Info.plist
        RESOURCE "${RESOURCE_FILES_MEGACMD_SERVER}"
    )
endif()
add_source_and_corresponding_header_to_target(mega-cmd-server PRIVATE
    "${ProjectDir}/src/megacmd_server_main.cpp"
    "${RESOURCE_FILES_MEGACMD_SERVER}"
)
if (APPLE)
set_target_properties(mega-cmd-server PROPERTIES OUTPUT_NAME "MEGAcmd")
elseif (WIN32)
set_target_properties(mega-cmd-server PROPERTIES OUTPUT_NAME "MEGAcmdServer")
endif()


add_library(LMegacmdClient STATIC)
add_source_and_corresponding_header_to_target(LMegacmdClient PUBLIC
    "${ProjectDir}/src/client/megacmdclient.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
)

add_executable(mega-exec ${executablesType})
add_source_and_corresponding_header_to_target(mega-exec PRIVATE
    "${ProjectDir}/src/client/megacmd_client_main.cpp"
    "${RESOURCE_FILES_MEGACMD_CLIENT}"
)

add_executable(mega-cmd ${executablesType})
add_source_and_corresponding_header_to_target(mega-cmd PRIVATE
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunications.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp"
    "${ProjectDir}/src/megacmdshell/megacmdshell.cpp"
    "${RESOURCE_FILES_MEGACMD_SHELL}"
)

if (WIN32)
    add_executable(mega-cmd-updater WIN32)
else()
    add_executable(mega-cmd-updater ${executablesType})
endif()
add_source_and_corresponding_header_to_target(mega-cmd-updater PRIVATE
    "${ProjectDir}/src/updater/MegaUpdater.cpp"
)
add_source_and_corresponding_header_to_target(mega-cmd-updater PRIVATE
    "${ProjectDir}/src/updater/UpdateTask.cpp"
    "${RESOURCE_FILES_MEGACMD_UPDATER}"
)

target_sources_conditional(mega-cmd-updater
    FLAG APPLE
    PRIVATE
    "${ProjectDir}/src/updater/MacUtils.h"
    "${ProjectDir}/src/updater/MacUtils.mm"
)

if (ENABLE_MEGACMD_TESTS)
    #Test Common:
    add_library(LMegacmdTestsCommon STATIC)
    add_source_and_corresponding_header_to_target(LMegacmdTestsCommon PRIVATE
        "${ProjectDir}/tests/common/Instruments.cpp"
        "${ProjectDir}/tests/common/TestUtils.cpp"
    )

    #Integration tests:
    add_executable(mega-cmd-tests-integration ${executablesType})
    add_source_and_corresponding_header_to_target(mega-cmd-tests-integration PRIVATE
        "${ProjectDir}/tests/integration/BasicTests.cpp"
        "${ProjectDir}/tests/integration/CatTests.cpp"
        "${ProjectDir}/tests/integration/ExportTests.cpp"
        "${ProjectDir}/tests/integration/PutTests.cpp"
        "${ProjectDir}/tests/integration/SyncIssuesTests.cpp"
        "${ProjectDir}/tests/integration/SyncIgnoreTests.cpp"
        "${ProjectDir}/tests/integration/FuseTests.cpp"
        "${ProjectDir}/tests/integration/MegaCmdTestingTools.cpp"
        "${ProjectDir}/tests/integration/main.cpp"
    )
    target_include_directories(mega-cmd-tests-integration PUBLIC ${ProjectDir}/src ${ProjectDir}/tests/common)
    target_link_libraries(mega-cmd-tests-integration PUBLIC LMegacmdServer LMegacmdClient LMegacmdTestsCommon)
    if(APPLE)
        target_link_libraries(mega-cmd-tests-integration PRIVATE "-framework Security" )
    endif()

    #Unit tests:
    add_executable(mega-cmd-tests-unit ${executablesType})
    add_source_and_corresponding_header_to_target(mega-cmd-tests-unit PRIVATE
        "${ProjectDir}/tests/unit/PlatformDirectoriesTests.cpp"
        "${ProjectDir}/tests/unit/StringUtilsTests.cpp"
        "${ProjectDir}/tests/unit/UtilsTests.cpp"
        "${ProjectDir}/tests/unit/main.cpp"
    )

    if(APPLE)
        target_link_libraries(mega-cmd-tests-unit PRIVATE "-framework Security" )
    endif()
endif()

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

target_link_libraries(LMegacmdClient PUBLIC MEGA::SDKlib LMEGAcmdCommonUtils)
target_link_libraries(mega-exec LMegacmdClient)
target_link_libraries(mega-cmd PUBLIC MEGA::SDKlib LMEGAcmdCommonUtils)
target_link_libraries(mega-cmd-updater PUBLIC MEGA::SDKlib LMEGAcmdCommonUtils)
target_link_libraries(LMegacmdServer PUBLIC MEGA::SDKlib LMEGAcmdCommonUtils)

if (ENABLE_MEGACMD_TESTS)
  target_link_libraries(LMegacmdServer PUBLIC LMegacmdTestsCommon)
endif()

if (WIN32)
  target_link_libraries(LMegacmdServer PUBLIC Lz32.lib Taskschd.lib)
  target_link_libraries(mega-cmd-updater PUBLIC Lz32.lib Urlmon.lib)
endif()
target_link_libraries(mega-cmd-server PUBLIC LMegacmdServer)

if (ENABLE_MEGACMD_TESTS)
    target_include_directories(LMegacmdTestsCommon PUBLIC ${ProjectDir}/src ${ProjectDir}/tests/common)
    target_link_libraries(mega-cmd-tests-unit PUBLIC LMegacmdServer LMegacmdTestsCommon)
endif()

# Load 3rd parties #TODO: consider splitting by target?
load_megacmdserver_libraries()

#file(GET_RUNTIME_DEPENDENCIES ... _deps)
#foreach(_dep IN LISTS _deps)
#    message( "es una dependencia")
##  if("${_dep}" SAME_FILE "/some/file/built/by/cmake.so") # if(SAME_FILE) doesn't currently exist, we'd have to create it
##    # Target install rules for this CMake-built target
#  #else()
##    # Standard install rules for external runtime dependencies
##  endif()
#endforeach()


list(APPEND all_targets mega-exec mega-cmd mega-cmd-server)
if (APPLE)
    list(APPEND all_targets mega-cmd-updater)
endif()

if(ENABLE_MEGACMD_TESTS)
    list(APPEND all_targets mega-cmd-tests-unit mega-cmd-tests-integration)
endif()
# Install stuff
if (APPLE)
    install(TARGETS ${all_targets}
        BUNDLE
        DESTINATION "./"
    )
elseif(NOT WIN32)
    set(PERMISSIONS755
         OWNER_READ
         OWNER_WRITE
         OWNER_EXECUTE
         GROUP_READ
         GROUP_EXECUTE
         WORLD_READ
         WORLD_EXECUTE
         )
     set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS ${PERMISSIONS755})

    install(TARGETS ${all_targets}
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src/client/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_PERMISSIONS ${PERMISSIONS755}
        FILES_MATCHING
        PATTERN "mega-*"
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.h" EXCLUDE
        PATTERN "mega-exec" EXCLUDE
        PATTERN "megacmd_completion.sh" EXCLUDE
        PATTERN "python" EXCLUDE
        PATTERN "win" EXCLUDE)

    install(FILES "${CMAKE_CURRENT_LIST_DIR}/src/client/megacmd_completion.sh"
        DESTINATION "etc/bash_completion.d"
        )

    # generate 100-megacmd-inotify-limit.conf file and have it installed
    execute_process(COMMAND echo "fs.inotify.max_user_watches = 524288"
                OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/99-megacmd-inotify-limit.conf)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/99-megacmd-inotify-limit.conf
        DESTINATION "etc/sysctl.d"
        )

    #Install vcpkg dynamic libraries in locations defined by GNUInstallDirs.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        SET(vcpkg_lib_folder "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/")
    else()
        SET(vcpkg_lib_folder "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/")
    endif()
    install(DIRECTORY "${vcpkg_lib_folder}"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING
            PATTERN "lib*.so*"
            PATTERN "*dylib*" #macOS
            PATTERN "manual-link" EXCLUDE
            PATTERN "pkgconfig" EXCLUDE)
endif() #not WIN32
