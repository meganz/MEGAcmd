@echo off
setlocal

set "WINFSP_VERSION=2.1.25156"
set "WINFSP_VERSION_SHORT=2.1"
set "EXPECTED_HASH=073a70e00f77423e34bed98b86e600def93393ba5822204fac57a29324db9f7a"
set "FILENAME=winfsp-installer.msi"
set "URL=https://github.com/winfsp/winfsp/releases/download/v%WINFSP_VERSION_SHORT%/winfsp-%WINFSP_VERSION%.msi"

echo Downloading: %URL%
curl -L -o "%FILENAME%" "%URL%"
if errorlevel 1 (
    echo ERROR: Download failed
    exit /b 1
)
for /f %%H in ('sha256sum "%FILENAME%"') do set "ACTUAL_HASH=%%H"

if /i "%ACTUAL_HASH%"=="%EXPECTED_HASH%" (
    echo Downloaded %FILENAME% correctly
) else (
    echo ERROR: Checksum mismatch!
    echo Expected: %EXPECTED_HASH%
    echo Actual  : %ACTUAL_HASH%
    exit /b 1
)

endlocal

exit /B
