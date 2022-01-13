#[[
    Run this file from its current folder in script mode, with triplet as a defined parameter:
        cmake -DTRIPLET=<triplet> [-DTARGET=<target>[;<targets>...] ] -P build_from_scratch.cmake
		
	eg. for windows:
	     cmake -DTRIPLET=x64-windows-mega -P build_from_scratch.cmake
	     cmake -DTRIPLET=x86-windows-mega -P build_from_scratch.cmake
		 (on a highly locked down machine you may need to run it from a git console so that sh.exe can be invoked)
		
    It will set up and build 3rdparty in a folder next to the MEGAcmd repo, and also
    build MEGAcmd against those 3rd party libraries.
    pdfium must be supplied manually, or it can be commented out in preferred-ports-megacmd.txt
    After 3rd party dependencies are downloaded, the build of the project itself may be controlled with
    an optional TARGET parameter. It accepts one target, or multiple targets separated by semicolons:
        -DTARGET=target1
        "-DTARGET=target1;target2"
    If provided, the script will build only the targets specified. If not, the script will build the
    whole project in a manner equivalent to calling `make all`.
]]

function(usage_exit err_msg)
    message(FATAL_ERROR
"${err_msg}
Build from scratch helper script usage:
    cmake -DTRIPLET=<triplet> [-DTARGET=<target>[;<targets>...] ] -P build_from_scratch.cmake
Script must be run from its current folder")
endfunction()

set(_script_cwd ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT EXISTS "${_script_cwd}/build_from_scratch.cmake")
    usage_exit("Script was not run from its containing directory")
endif()

if(NOT TRIPLET)
    usage_exit("Triplet was not provided")
endif()

if(NOT EXTRA_ARGS)
	message(STATUS "No extra args")
else()
    set(_extra_cmake_args ${EXTRA_ARGS})
	message(STATUS "Applying extra args: ${EXTRA_ARGS}")
endif()

set(_triplet ${TRIPLET})
set(_cmd_dir "${_script_cwd}/../..")
set(_sdk_dir "${_cmd_dir}/sdk")

message(STATUS "Building for triplet ${_triplet} with CMD ${_cmd_dir} and SDK ${_sdk_dir}")

set (_3rdparty_dir "${_cmd_dir}/../3rdparty_megacmd")

file(MAKE_DIRECTORY ${_3rdparty_dir})

set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)

set(_cmake ${CMAKE_COMMAND})

function(execute_checked_command)

    message(STATUS "build_from_scratch executing: ${ARGV}")
    message(STATUS "------")

    execute_process(
        ${ARGV}
        RESULT_VARIABLE _result
        ERROR_VARIABLE _error
    )

    if(_result)
        message(FATAL_ERROR "Execute_process command had nonzero exit code ${_result} with error ${_error}")
    endif()
endfunction()


# Configure and build the build3rdparty tool

execute_checked_command(
    COMMAND ${_cmake}
        -S ${_sdk_dir}/contrib/cmake/build3rdParty
        -B ${_3rdparty_dir}
        -DCMAKE_BUILD_TYPE=Release
)

execute_checked_command(
    COMMAND ${_cmake}
        --build ${_3rdparty_dir}
        --config Release
)

# Use the prep tool to set up just our dependencies and no others

if(WIN32)
    set(_3rdparty_tool_exe "${_3rdparty_dir}/Release/build3rdParty.exe")
    set(_3rdparty_vcpkg_dir "${_3rdparty_dir}/Release/vcpkg/")
else()
    set(_3rdparty_tool_exe "${_3rdparty_dir}/build3rdParty")
    set(_3rdparty_vcpkg_dir "${_3rdparty_dir}/vcpkg/")
endif()

set(_3rdparty_tool_common_args
    --ports "${_script_cwd}/preferred-ports-megacmd.txt"
    --triplet ${_triplet}
    --sdkroot ${_sdk_dir}
)

execute_checked_command(
    COMMAND ${_3rdparty_tool_exe}
        --setup
        --removeunusedports
        --nopkgconfig
        ${_3rdparty_tool_common_args}
    WORKING_DIRECTORY ${_3rdparty_dir}
)

execute_checked_command(
    COMMAND ${_3rdparty_tool_exe}
        --build
        ${_3rdparty_tool_common_args}
    WORKING_DIRECTORY ${_3rdparty_dir}
)


# Allows use of the VCPKG_XXXX variables defined in the triplet file
# We search our own custom triplet folder, and then the standard ones searched by vcpkg
foreach(_triplet_dir
    "${_sdk_dir}/contrib/cmake/vcpkg_extra_triplets/"
    "${_3rdparty_vcpkg_dir}/triplets/"
    "${_3rdparty_vcpkg_dir}/triplets/community"
)
    set(_triplet_file "${_triplet_dir}/${_triplet}.cmake")
    if(EXISTS ${_triplet_file})
        message(STATUS "Using triplet ${_triplet} at ${_triplet_file}")
        include(${_triplet_file})
        set(_triplet_file_found 1)
        break()
    endif()
endforeach()

if(NOT _triplet_file_found)
    message(FATAL_ERROR "Could not find triplet ${_triplet} in Mega vcpkg_extra_triplets nor in vcpkg triplet folders")
endif()

# Now set up to build this repo
# Logic between Windows and other platforms diverges slightly here:
# in the CMake paradigm, Visual Studio is what's called a multi-configuration generator,
# meaning we can do separate builds (Debug, Release) from one configuration.
# The default generators on *nix platforms (Ninja, Unix Makefiles) are single-config, so we
# configure once for each build type
# In a future extension, we may wish to have users provide the configs as a semicolon-separated list

set(_common_cmake_args
    "-DMega3rdPartyDir=${_3rdparty_dir}"
    "-DVCPKG_TRIPLET=${_triplet}"
    -DUSE_THIRDPARTY_FROM_VCPKG=1
    -S ${_script_cwd}
)

if(TARGET)
    set(_cmake_target_args "--target" ${TARGET})
endif()

if(WIN32)
    if(_triplet MATCHES "staticdev$")
        set(_extra_cmake_args ${_extra_cmake_args} -DMEGA_LINK_DYNAMIC_CRT=0 -DUNCHECKED_ITERATORS=1)
    endif()

    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
        set(_arch "Win32")
    else()
        set(_arch ${VCPKG_TARGET_ARCHITECTURE})
    endif()

    set(_toolset ${VCPKG_PLATFORM_TOOLSET})
    set(_build_dir "${_cmd_dir}/build-${_triplet}")

    file(MAKE_DIRECTORY ${_build_dir})

    execute_checked_command(
        COMMAND ${_cmake}
            -G "Visual Studio 16 2019"
            -A ${_arch}
            # Could also pass -T VCPKG_PLATFORM_TOOLSET
            -B ${_build_dir}
            ${_common_cmake_args}
            ${_extra_cmake_args}
    )

    foreach(_config "Debug" "Release")
        execute_checked_command(
            COMMAND ${_cmake}
                --build ${_build_dir}
                ${_cmake_target_args}
                --config ${_config}
                --parallel 4
        )
    endforeach()
else()
    foreach(_config "Debug" "Release")
        set(_build_dir "${_cmd_dir}/build-${_triplet}-${_config}")
        file(MAKE_DIRECTORY ${_build_dir})

        execute_checked_command(
            COMMAND ${_cmake}
                ${_common_cmake_args}
                ${_extra_cmake_args}
                -B ${_build_dir}
                "-DCMAKE_BUILD_TYPE=${_config}"
        )

        execute_checked_command(
            COMMAND ${_cmake}
                --build ${_build_dir}
                ${_cmake_target_args}
                --parallel 4
        )
    endforeach()
endif()
