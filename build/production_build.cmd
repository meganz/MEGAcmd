IF [%MEGA_VCPKGPATH%]==[] (
	SET MEGA_VCPKGPATH=C:\mega\dev\3rdparty_megacmd
)


IF [%MEGA_CORES%]==[] (
	FOR /f "tokens=2 delims==" %%f IN ('wmic cpu get NumberOfLogicalProcessors /value ^| find "="') DO SET MEGA_CORES=%%f
)

REM Clean up any previous leftovers
IF EXIST build-x64-windows-mega (
    rmdir /s /q build-x64-windows-mega
)

mkdir build-x64-windows-mega
cd build-x64-windows-mega
cmake -G "Visual Studio 16 2019" -A x64 -DMega3rdPartyDir=%MEGA_VCPKGPATH% -DVCPKG_TRIPLET=x64-windows-mega -S "..\cmake" -B .
cmake --build . --config Release --target MEGAcmd --target MEGAcmdServer --target MEGAcmdShell --target MEGAcmdUpdater -j%MEGA_CORES%
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
cmake -G "Visual Studio 16 2019" -A Win32 -DMega3rdPartyDir=%MEGA_VCPKGPATH% -DVCPKG_TRIPLET=x86-windows-mega -S "..\cmake" -B .
cmake --build . --config Release --target MEGAcmd --target MEGAcmdServer --target MEGAcmdShell --target MEGAcmdUpdater -j%MEGA_CORES%
cd ..
