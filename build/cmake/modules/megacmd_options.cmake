option(USE_PCRE "Used to provide support for pcre" ON)

option(FULL_REQS "Fail compilation when some requirement is not met" ON)

if(USE_PCRE)
    add_compile_definitions(USE_PCRE) #this one is no longer added to target_compile_definitions(SDKlib, hence some cpps not using config.h will lack this, causing the full reqs to fail
    # TODO: remove once PCRE optionality is removed
endif()

if(FULL_REQS)
 #On by default in the SDK, but let's enforce anyway
    option(USE_PDFIUM "Used to create previews/thumbnails for PDF files" ON)
    option(USE_FFMPEG "Used to create previews/thumbnails for video files" ON)
    option(ENABLE_MEDIA_FILE_METADATA "Used to determine media properties and set those as node attributes" ON)
    option(USE_MEDIAINFO "[DEPRECATED] Used to determine media properties and set those as node attributes" ON)

    #make compilation fail in case those are not fully available:
    add_compile_definitions(
        REQUIRE_HAVE_FFMPEG
        REQUIRE_HAVE_PDFIUM
        REQUIRE_USE_MEDIAINFO
        REQUIRE_HAVE_LIBUV
        REQUIRE_USE_PCRE
    )
else()
    option(USE_PDFIUM "Used to create previews/thumbnails for PDF files" OFF)
    option(USE_FFMPEG "Used to create previews/thumbnails for video files" OFF)
    option(ENABLE_MEDIA_FILE_METADATA "Used to determine media properties and set those as node attributes" OFF)
    option(USE_MEDIAINFO "[DEPRECATED] Used to determine media properties and set those as node attributes" OFF)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" )
    option(ENABLE_MEGACMD_TESTS "Enable MEGAcmd testing code" ON)
else()
    option(ENABLE_MEGACMD_TESTS "Enable MEGAcmd testing code" OFF)
endif()

if(ENABLE_MEGACMD_TESTS)
    add_compile_definitions(MEGACMD_TESTING_CODE=1)
endif()

#Override SDK's options:
option(ENABLE_ISOLATED_GFX "Turns on isolated GFX processor" OFF)
option(ENABLE_SDKLIB_WERROR "Enable warnings as errors" OFF)
option(USE_LIBUV "Includes the library and turns on internal web and ftp server functionality" ON)

if(WIN32)
    option(WITH_FUSE "Build with FUSE support." ON)
elseif(UNIX AND NOT APPLE)
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE SYSTEM_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "System Architecture: <${SYSTEM_ARCHITECTURE}>")
    if(SYSTEM_ARCHITECTURE MATCHES "^(i[3-6]86|x86|armhf|armv7l)$")
        set(IS_32_BIT ON)
    else()
        set(IS_32_BIT OFF)
    endif()

    if(NOT IS_32_BIT)
        message(STATUS "Configuring with FUSE support")
        option(WITH_FUSE "Build with FUSE support." ON)
    endif()
endif()

if(WITH_FUSE)
    add_compile_definitions(WITH_FUSE=1)
endif()
