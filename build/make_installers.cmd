IF "%1%" EQU "-help" (
	goto Usage
)

SET SUFFIX_DEF=
if not "%MEGA_VERSION_SUFFIX%" == "" (
	SET SUFFIX_DEF=/DVERSION_SUFFIX=%MEGA_VERSION_SUFFIX%
)

SET MEGA_SIGN=%1

SET KITVER=10.0.22621.0

:: DOUBLE CHECK SIGN
IF "%MEGA_SIGN%" EQU "sign" (
	echo "Info: Signed installer(s) will be generated. Checking if sources are signed"
	
	"C:\Program Files (x86)\Windows Kits\10\bin\%KITVER%\x64\signtool.exe" verify /pa built64/*.exe || exit 1 /b
	IF NOT "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
		"C:\Program Files (x86)\Windows Kits\10\bin\%KITVER%\x64\signtool.exe" verify /pa built32/*.exe || exit 1 /b
	)
) ELSE (
	IF "%MEGA_SIGN%" EQU "nosign" (
	echo "Info: Unsigned installer(s) will be generated"
	SET MEGA_THIRD_PARTY_DLL_DIR=bin
	) ELSE (
		echo "Please add a correct sign argument: sign or nosign"
		goto Usage
	)
)

erase MEGAcmdSetup64.exe 2>nul
erase MEGAcmdSetup64_unsigned.exe 2>nul
"C:\Program Files (x86)\NSIS\makensis.exe" /DWINKITVER=%KITVER% /DBUILD_X64_VERSION %SUFFIX_DEF% installer_win.nsi || exit 1 /b
IF "%MEGA_SIGN%" EQU "nosign" (
ren MEGAcmdSetup64.exe MEGAcmdSetup64_unsigned.exe
)

IF "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	GOTO :EOF
)

erase MEGAcmdSetup32.exe >nul
erase MEGAcmdSetup32_unsigned.exe >nul
"C:\Program Files (x86)\NSIS\makensis.exe" /DWINKITVER=%KITVER%  %SUFFIX_DEF% installer_win.nsi || exit 1 /b
IF "%MEGA_SIGN%" EQU "nosign" (
ren MEGAcmdSetup32.exe MEGAcmdSetup32_unsigned.exe
)


exit /B

:Usage
echo "Usage: %~0 [-help] [sign|nosign]"
echo Script for making the installer, expecting files in built folders"
exit 2 /b
