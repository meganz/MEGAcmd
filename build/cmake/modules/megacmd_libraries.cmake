macro(load_megacmdserver_libraries)

    if(VCPKG_ROOT)
        if(USE_PCRE)
            find_package(unofficial-pcre REQUIRED)
            target_link_libraries(LMegacmdServer PRIVATE unofficial::pcre::pcrecpp)
            set(USE_PCRE 1)
        endif()

        if (ENABLE_MEGACMD_TESTS)
            find_package(GTest CONFIG REQUIRED)
            target_link_libraries(LMegacmdTestsCommon PUBLIC GTest::gtest GTest::gmock)
        endif()

    else() # No VCPKG usage. Use pkg-config
        find_package(PkgConfig REQUIRED) # For libraries loaded using pkg-config

        if(USE_PCRE) #TODO: UNTESTED!
            pkg_check_modules(pcre REQUIRED IMPORTED_TARGET libpcre)
            target_link_libraries(LMegacmdServer PRIVATE PkgConfig::pcre)
            set(USE_PCRE 1)
        endif()
    endif()

endmacro()
