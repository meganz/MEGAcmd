SET MEGA_SIGN=%1

REM TODO: Have this unified (env variable?)
SET KITVER=10.0.22621.0

:: CHECK SIGN
:Check

echo Info: Checking if sources are signed

"C:\Program Files (x86)\Windows Kits\10\bin\%KITVER%\x64\signtool.exe" verify /pa sign64/*.exe >nul || goto SleepAndRetry
IF NOT "%MEGA_SKIP_32_BIT_BUILD%" == "true" (
	"C:\Program Files (x86)\Windows Kits\10\bin\%KITVER%\x64\signtool.exe" verify /pa sign32/*.exe >nul || goto SleepAndRetry
)
echo Signed executables found. Will copy them to builtXX folders

REM Copying signed installers
copy sign64\* built64
copy sign32\* built32

exit /b


:SleepAndRetry
IF [%MSYSTEM%]==[MINGW64] (
	REM pause or set \p freezed mingw bash and there are no builtin mechanism for Batch to sleep, using python for it:
	echo "you need to place signed executables in sign folders. The signature verification will be retry in 30 seconds"
	python -c "import time; time.sleep(30)"
) else (
	echo You need to place signed executables in sign folders. After that,
	pause
)

goto Check

exit 2 /b

