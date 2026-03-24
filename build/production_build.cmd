IF NOT [%MEGA_VCPKGPATH%]==[] (
	SET "VCPKG_ROOT_ARG=-DVCPKG_ROOT=%MEGA_VCPKGPATH%"
)

IF [%MEGA_CORES%]==[] (
	FOR /f "tokens=2 delims==" %%f IN ('wmic cpu get NumberOfLogicalProcessors /value ^| find "="') DO SET MEGA_CORES=%%f
)

REM Clean up any previous leftovers
IF EXIST build-x64-windows-mega (
    rmdir /s /q build-x64-windows-mega
)

REM download winsfp
call get_winfsp.cmd || exit 1 /b

mkdir build-x64-windows-mega
cd build-x64-windows-mega
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/Zi /O2 /Ob2 /DNDEBUG" %VCPKG_ROOT_ARG% -DCMAKE_C_FLAGS_RELWITHDEBINFO="/Zi /O2 /Ob2 /DNDEBUG" -S "..\.." -B . || exit 1 /b
cmake --build . --config RelWithDebInfo --target mega-exec --target mega-cmd-server --target mega-cmd --target  mega-cmd-updater -j%MEGA_CORES% || exit 1 /b
cd ..

IF "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	GOTO :EOF
)

REM Clean up any previous leftovers
IF EXIST build-x86-windows-mega (
	rmdir /s /q build-x86-windows-mega
)

mkdir build-x86-windows-mega
cd build-x86-windows-mega
cmake -G "Visual Studio 17 2022" -A Win32 -DCMAKE_VERBOSE_MAKEFILE="ON" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/Zi /O2 /Ob2 /DNDEBUG" %VCPKG_ROOT_ARG% -DCMAKE_C_FLAGS_RELWITHDEBINFO="/Zi /O2 /Ob2 /DNDEBUG" -S "..\.." -B . || exit 1 /b
cmake --build . --config RelWithDebInfo --target mega-exec --target mega-cmd-server --target mega-cmd --target  mega-cmd-updater -j%MEGA_CORES% || exit 1 /b
cd ..
