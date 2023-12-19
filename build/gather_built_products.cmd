
IF [%MEGA_VCPKGPATH%]==[] (
	SET MEGA_VCPKGPATH="C:\mega\dev\3rdparty_megacmd"
)

IF [%MEGA_THIRD_PARTY_DLL_DIR%]==[] (
	SET MEGA_THIRD_PARTY_DLL_DIR=bin_dlls_signed
)

mkdir built64
mkdir sign64

SET BASEMEGACMDPATH=.

copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\*.dll built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAclient.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdServer.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdShell.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdUpdater.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\*.pdb built64

copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAclient.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdServer.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdShell.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\Release\MEGAcmdUpdater.exe sign64

set "MEGA_VCPKGPATH_COPYABLE=%MEGA_VCPKGPATH:/=\%"

copy %MEGA_VCPKGPATH_COPYABLE%\vcpkg\installed\x64-windows-mega\%MEGA_THIRD_PARTY_DLL_DIR%\*.* built64

IF "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	GOTO :EOF
)

mkdir built32
mkdir sign32

copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\*.dll built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAclient.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdServer.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdShell.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdUpdater.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\*.pdb built32

copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAclient.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdServer.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdShell.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\Release\MEGAcmdUpdater.exe sign32

REM copy scritps??

copy %MEGA_VCPKGPATH_COPYABLE%\vcpkg\installed\x86-windows-mega\%MEGA_THIRD_PARTY_DLL_DIR%\*.* built32
