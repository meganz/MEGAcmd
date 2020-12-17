mkdir built32
mkdir built64

copy cmake\64\Release\*.exe built64
copy cmake\32\Release\*.exe built32

copy C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20201215\vcpkg\installed\x64-windows-mega\bin\*.* built64
copy C:\Users\build\MEGA\build-MEGAcmd\3rdParty_MSVC2017_20201215\vcpkg\installed\x86-windows-mega\bin\*.* built32

