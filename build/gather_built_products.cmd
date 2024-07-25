mkdir built64
mkdir sign64

SET BASEMEGACMDPATH=.

copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\*.dll built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAclient.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdServer.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdShell.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdUpdater.exe built64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\*.pdb built64

copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\*.dll sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAclient.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdServer.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdShell.exe sign64
copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\MEGAcmdUpdater.exe sign64

REM copy debug symbols of 3rd parties. dlls were already copied
copy %BASEMEGACMDPATH%\build-x64-windows-mega\vcpkg_installed\x64-windows-mega\bin\*.pdb built64

IF "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	GOTO :EOF
)

mkdir built32
mkdir sign32

copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\*.dll built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAclient.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdServer.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdShell.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdUpdater.exe built32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\*.pdb built32

copy %BASEMEGACMDPATH%\build-x64-windows-mega\RelWithDebInfo\*.dll sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAclient.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdServer.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdShell.exe sign32
copy %BASEMEGACMDPATH%\build-x86-windows-mega\RelWithDebInfo\MEGAcmdUpdater.exe sign32

REM copy debug symbols of 3rd parties. dlls were already copied
copy %BASEMEGACMDPATH%\build-x64-windows-mega\vcpkg_installed\x86-windows-mega\bin\*.pdb built32

REM copy scritps??

