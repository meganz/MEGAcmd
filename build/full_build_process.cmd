@echo off 

IF "%1%" EQU "-help" (
	goto Usage
)

SET MEGA_ARCH=32/64
SET MEGA_SKIP_32_BIT_BUILD=false
SET MEGA_SIGN=sign
SET MEGA_CORES=0
SET MEGA_VERSION_SUFFIX=

IF NOT "%1" == "" (
	SET MEGA_ARCH=%1
	SET MEGA_SIGN=%2
	SET MEGA_CORES=%3
	SET MEGA_VERSION_SUFFIX=%4
	
	IF [%MEGA_VCPKGPATH%]==[] (
		echo "Error: MEGA_VCPKGPATH environment variable is not set. Please set it."
		goto Usage
	)
	
	:: CHECK NUMBER OF ARGUMENTS
	IF "%3" == "" (
		echo "Error: too few arguments"
		goto Usage
	)
	IF NOT "%5" == "" (
		echo "Error: too many arguments"
		goto Usage
	)
) ELSE (
	IF [%MEGA_VCPKGPATH%]==[] (
		SET "SCRIPT_DIR=%~dp0"
		SET "MEGA_VCPKGPATH=%SCRIPT_DIR%..\..\"
	)
)

IF [%MEGA_WIN_KITVER%]==[] (
	SET MEGA_WIN_KITVER=10.0.22621.0
)

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

:: CHECK SIGN
IF "%MEGA_SIGN%" EQU "sign" (
	echo "Info: Signed installer(s) will be generated"
) ELSE (
	IF "%MEGA_SIGN%" EQU "nosign" (
	echo "Info: Unsigned installer(s) will be generated"
	SET MEGA_THIRD_PARTY_DLL_DIR=bin
	) ELSE (
		echo "Please add a correct sign argument: sign or nosign"
		goto Usage
	)
)

:: CHECK CORES
SET "VALID_CORES=1"
IF %MEGA_CORES% LSS 0 (
	SET "VALID_CORES=0"
)
IF %MEGA_CORES% GTR 16 (
	SET "VALID_CORES=0"
)
IF %MEGA_CORES% EQU 0 (
	FOR /f "tokens=2 delims==" %%f IN ('wmic cpu get NumberOfLogicalProcessors /value ^| find "="') DO SET MEGA_CORES=%%f
)
IF %VALID_CORES% EQU 0 (
	echo "Please add a correct core argument: 1 to 16, or 0 for default value"
	goto Usage
)

echo "Info: CORES SET to %MEGA_CORES%"

REM Clean up any previous leftovers
IF EXIST built32 (
    rmdir /s /q built32
)
IF EXIST sign32 (
    rmdir /s /q sign32
)
IF EXIST built64 (
    rmdir /s /q built64
)
IF EXIST sign64 (
    rmdir /s /q sign64
)

IF [%SKIP_BUILD_PRODUCTS%]==[] (
IF EXIST build-x64-windows-mega (
    rmdir /s /q build-x64-windows-mega
)
IF EXIST build-x86-windows-mega (
    rmdir /s /q build-x86-windows-mega
)
)

IF [%SKIP_BUILD_THIRD_PARTIES%]==[] (
echo calling build_3rd_parties.cmd %MEGA_ARCH%
call build_3rd_parties.cmd %MEGA_ARCH% || exit 1 /b
)

IF NOT [%ONLY_BUILD_THIRD_PARTIES%]==[] (
exit /b
)

IF [%SKIP_BUILD_PRODUCTS%]==[] (
echo calling production_build.cmd
call production_build.cmd || exit 1 /b
)

echo calling gather_built_products.cmd
call gather_built_products.cmd || exit 1 /b

echo calling make_uninstallers.cmd
call make_uninstallers.cmd || exit 1 /b

IF "%MEGA_SIGN%" EQU "sign" (
echo time to sign the executables in built32/64 folders
REM TODO: here in case of IF "%MEGA_SIGN%" EQU "sign" , the signing would need to take place, replacing the built .exes with the signed ones

echo gathering signed executables in the built folders
call gather_signed_products.cmd || exit 1 /b
)

echo calling make_installers.cmd
call make_installers.cmd %MEGA_SIGN% || exit 1 /b

REM TODO: Pending signing the installers themselves!

exit /B

:Usage
echo "Usage: %~0 [-help] [64|32/64 sign|nosign <cores number> [<suffix>]]"
echo Script building, signing and creating the installer(s)
echo It can take 0, 1, 3 or 4 arguments:
echo 	- -help: this message
echo 	- 0 arguments: use these settings: 32/64 sign 1
echo 	- Architecture : 64 or 32/64 to build either for 64 bit or both 32 and 64 bit
echo 	- Sign: sign or nosign if the binaries must be signed or not
echo 	- Cores: the number of cores to build the project, or 0 for default value (number of logical cores on the machine)
echo 	- Suffix for installer: The installer will add this suffix to the version. [OPTIONAl]
echo MEGA_VCPKGPATH environment variable should be set to the root of the 3rd party dir.
echo SKIP_BUILD_THIRD_PARTIES environment variable can be used to skip the attempt to build vcpkg 3rd parties
echo ONLY_BUILD_THIRD_PARTIES environment variable can be used to stop after building 3rd parties
echo MEGA_WIN_KITVER environment variable can be used to set the Windows sdk to use. Value defaults to "10.0.22621.0". Set to "." to use the Universal Kit
exit /B
