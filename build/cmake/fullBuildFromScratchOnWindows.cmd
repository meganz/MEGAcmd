
REM Run this file from its current folder, with triplet as a parameter. 
REM It will set up and build 3rdParty in a folder next to the MEGAcmd repo
REM And also build MEGAcmd against those third party libraries.
REM At the moment, you have to supply the pdfium library code, eg by cut and paste after starting the script.

REM If anything goes wrong, typically you will need to adjust and manually execute the rest of the script, 
REM as it's not designed to be re-run over the old output.

REM However, you can run it multiple times in the same repo, with different triplets.  For example,
REM (and, take into account that the script may need some small tweaks depending on your VS version etc)
REM    fullBuildFromScratchOnWindows.cmd x64-windows-mega
REM    fullBuildFromScratchOnWindows.cmd x86-windows-mega
REM    fullBuildFromScratchOnWindows.cmd x64-windows-mega-staticdev
REM    fullBuildFromScratchOnWindows.cmd x86-windows-mega-staticdev

set TRIPLET=%1
set MEGACMD_DIR=%CD%\..\..

if "%TRIPLET%a" == "a" echo "Please supply a triplet string parameter"
if "%TRIPLET%a" == "a" goto ErrorHandler

REM Prepare 3rdParty folder and build the prep tool

if not exist %MEGACMD_DIR%\..\3rdParty_megacmd mkdir %MEGACMD_DIR%\..\3rdParty_megacmd
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

cd %MEGACMD_DIR%\..\3rdParty_megacmd
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

cmake %MEGACMD_DIR%\sdk\contrib\cmake\build3rdParty
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

cmake --build . --config Release
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

REM use the prep tool to set up just our dependencies and no others

.\Release\build3rdParty.exe --setup --removeunusedports --nopkgconfig --ports "%MEGACMD_DIR%\build\cmake\preferred-ports-megacmd.txt" --triplet %TRIPLET% --sdkroot "%MEGACMD_DIR%\sdk"
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

REM use the prep tool to invoke vcpkg to build all the dependencies

.\Release\build3rdParty.exe --build --ports "%MEGACMD_DIR%\build\cmake\preferred-ports-megacmd.txt" --triplet %TRIPLET% --sdkroot "%MEGACMD_DIR%\sdk"
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

REM Now set up to build this repo

if not exist "%MEGACMD_DIR%\%TRIPLET%" mkdir "%MEGACMD_DIR%\%TRIPLET%"
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler
cd "%MEGACMD_DIR%\%TRIPLET%"
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

set x86orx64=x64
if "%TRIPLET%zz"=="x86-windows-megazz" set x86orx64=Win32
if "%TRIPLET%zz"=="x86-windows-mega-staticdevzz" set x86orx64=Win32

cmake -G "Visual Studio 16 2019" -A %x86orx64% -DMega3rdPartyDir="%MEGACMD_DIR%\..\3rdParty_megacmd" -DVCPKG_TRIPLET=%TRIPLET% -S "%MEGACMD_DIR%\build\cmake" -B .
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

cmake --build . --config Debug
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

cmake --build . --config Release
IF %ERRORLEVEL% NEQ 0 goto ErrorHandler

ECHO ****************************** COMPLETE ********************************
exit /B %ERRORLEVEL%

:ErrorHandler
ECHO ****************************** ERROR %ERRORLEVEL% ********************************
exit /B %ERRORLEVEL%







