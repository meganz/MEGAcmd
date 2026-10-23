@echo off
setlocal

set "WINFSP_VERSION=2.2.26215"
set "WINFSP_TAG=v2.2B4"
set "EXPECTED_HASH=2ecb5c89405488a95bbd8a01875e02c48534fd37bbdfd84488f7590464d65944"
set "FILENAME=winfsp-installer.msi"
set "URL=https://github.com/winfsp/winfsp/releases/download/%WINFSP_TAG%/winfsp-%WINFSP_VERSION%.msi"

echo Downloading: %URL%
curl -L -o "%FILENAME%" "%URL%"
if errorlevel 1 (
    echo ERROR: Download failed
    exit /b 1
)
set "ACTUAL_HASH="
for /f %%H in ('sha256sum "%FILENAME%" 2^>nul') do set "ACTUAL_HASH=%%H"
if not defined ACTUAL_HASH echo sha256sum unavailable, computing the checksum with PowerShell
if not defined ACTUAL_HASH for /f %%H in ('powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 -Path '%FILENAME%').Hash.ToLowerInvariant()"') do set "ACTUAL_HASH=%%H"
if not defined ACTUAL_HASH (
    echo ERROR: Failed to compute SHA256 checksum
    exit /b 1
)

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
