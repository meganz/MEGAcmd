option(USE_PCRE "Used to provide support for pcre" ON)
option(USE_LIBUV "Includes the library and turns on internal web and ftp server functionality" ON)

#Backups:
option(ENABLE_BACKUPS "Used to enable backups (scheduled copies)" ON)

option(FULL_REQS "Fail compilation when some requirement is not met" ON)

if(USE_PCRE)
    add_compile_definitions(USE_PCRE) #this one is no longer added to target_compile_definitions(SDKlib, hence some cpps not using config.h will lack this, causing the full reqs to fail
    # TODO: remove once PCRE optionality is removed
endif()

if(FULL_REQS)
 #On by default in the SDK, but let's enforce anyway
    option(USE_PDFIUM "Used to create previews/thumbnails for PDF files" ON)
    option(USE_FFMPEG "Used to create previews/thumbnails for video files" ON)
    option(USE_MEDIAINFO "Used to determine media properties and set those as node attributes" ON)

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
    option(USE_MEDIAINFO "Used to determine media properties and set those as node attributes" OFF)
endif()

option(ENABLE_MEGACMD_TESTS "Enable MEGAcmd testing code" OFF)

if(ENABLE_MEGACMD_TESTS)
    add_compile_definitions(MEGACMD_TESTING_CODE=1)
endif()

#Override SDK's options:
option(ENABLE_ISOLATED_GFX "Turns on isolated GFX processor" OFF)
