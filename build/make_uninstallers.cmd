SET SUFFIX_DEF=
if not "%MEGA_VERSION_SUFFIX%" == "" (
	SET SUFFIX_DEF=/DVERSION_SUFFIX=%MEGA_VERSION_SUFFIX%
)

"C:\Program Files (x86)\NSIS\makensis.exe" /DWINKITVER=10.0.22621.0 /DBUILD_UNINSTALLER /DBUILD_X64_VERSION %SUFFIX_DEF% installer_win.nsi || exit 1 /b
UninstallerGenerator.exe
erase UninstallerGenerator.exe
copy uninst.exe built64
move uninst.exe sign64

IF "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	GOTO :EOF
)

"C:\Program Files (x86)\NSIS\makensis.exe" /DWINKITVER=10.0.22621.0 /DBUILD_UNINSTALLER %SUFFIX_DEF% installer_win.nsi || exit 1 /b
UninstallerGenerator.exe
erase UninstallerGenerator.exe
copy uninst.exe built32
move uninst.exe sign32
