IF "%1%" EQU "-help" (
	goto Usage
)

IF [%MEGA_VCPKGPATH%]==[] (
	SET MEGA_VCPKGPATH="C:\mega\dev\3rdparty_megacmd"
)

SET MEGA_ARCH=%1

:: CHECK ARCHITECTURE
IF "%MEGA_ARCH%" EQU "64" (
	echo "Info: Building x64 only"
	SET MEGA_SKIP_32_BIT_BUILD=true
) ELSE (
	IF "%MEGA_ARCH%" EQU "32/64" (
		echo "Info: Building both x64 and x86"
	) ELSE (
		echo "Please add the architecture as first parameter: 64 or 32/64"
		goto Usage
	)
)

cd cmake

cmake -G "Visual Studio 16 2019" -D3RDPARTY_DIR=%MEGA_VCPKGPATH% -DSKIP_PROJECT_BUILD=1 -DTRIPLET=x64-windows-mega -P build_from_scratch.cmake || exit 1 /b

IF NOT "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
cmake -G "Visual Studio 16 2019" -D3RDPARTY_DIR=%MEGA_VCPKGPATH% -DSKIP_PROJECT_BUILD=1 -DTRIPLET=x86-windows-mega -P build_from_scratch.cmake || exit 1 /b
)

cd ..

exit /b

:Usage
echo "Usage: %~0 [-help] [32|32/64|64]"
echo Script for building vcpkg 3rd party dependencies"
exit 2 /b
