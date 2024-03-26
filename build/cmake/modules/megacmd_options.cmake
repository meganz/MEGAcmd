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
    add_compile_definitions(REQUIRE_HAVE_PDFIUM REQUIRE_HAVE_FFMPEG REQUIRE_HAVE_LIBUV REQUIRE_USE_MEDIAINFO REQUIRE_USE_PCRE)
endif()
option(ENABLE_MEGACMD_TESTS "Enable MEGAcmd testing code" OFF)

if(ENABLE_MEGACMD_TESTS)
    add_compile_definitions(MEGACMD_TESTING_CODE=1)
endif()
