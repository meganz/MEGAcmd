# Define debug symbols covering all platforms.
add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

# Build the project with C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # https://gitlab.kitware.com/cmake/cmake/-/issues/18837
    add_compile_options(/Zc:__cplusplus) # Enable updated __cplusplus macro

    # Enable build with multiple processes.
    add_compile_options(/MP)

    # Create a separated PDB file with debug symbols.
    add_compile_options($<$<CONFIG:Release>:/Zi>)

else()
    include(CheckIncludeFile)
    include(CheckFunctionExists)
    check_include_file(inttypes.h HAVE_INTTYPES_H)
    check_include_file(dirent.h HAVE_DIRENT_H)
    check_include_file(glob.h HAVE_GLOB_H)
    check_function_exists(aio_write, HAVE_AIO_RT)
endif()
