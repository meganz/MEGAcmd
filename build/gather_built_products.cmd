
REM Build with (from MEGAcmd folder)
REM
REM mkdir x64-windows-mega
REM cd x64-windows-mega
REM cmake -G "Visual Studio 15 2017" -A x64 -DMega3rdPartyDir=C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20210302\3rdParty_megacmd -DVCPKG_TRIPLET=x64-windows-mega -S "..\build\cmake" -B .
REM cmake --build . --config Release
REM 
REM mkdir x86-windows-mega
REM cd x86-windows-mega
REM cmake -G "Visual Studio 15 2017" -A Win32 -DMega3rdPartyDir=C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20210302\3rdParty_megacmd -DVCPKG_TRIPLET=x86-windows-mega -S "..\build\cmake" -B .
REM cmake --build . --config Release


mkdir built32
mkdir built64

copy ..\x64-windows-mega\Release\MEGAcmdServer.exe built64
copy ..\x64-windows-mega\Release\MEGAcmdShell.exe built64
copy ..\x64-windows-mega\Release\MEGAclient.exe built64
copy ..\x64-windows-mega\Release\MEGAcmdUpdater.exe built64

copy ..\x86-windows-mega\Release\MEGAcmdServer.exe built32
copy ..\x86-windows-mega\Release\MEGAcmdShell.exe built32
copy ..\x86-windows-mega\Release\MEGAclient.exe built32
copy ..\x86-windows-mega\Release\MEGAcmdUpdater.exe built32

copy C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20210315\3rdParty_megacmd\vcpkg\installed\x64-windows-mega\bin_signed\*.* built64
copy C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20210315\3rdParty_megacmd\vcpkg\installed\x86-windows-mega\bin_signed\*.* built32

