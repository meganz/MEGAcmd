RequestExecutionLevel user
Unicode true

#!define BUILD_UNINSTALLER
#!define BUILD_X64_VERSION
#!define ENABLE_DEBUG_MESSAGES

!macro DEBUG_MSG message
!ifdef ENABLE_DEBUG_MESSAGES
  MessageBox MB_OK "${message}"
!endif
!macroend

; HM NIS Edit Wizard helper defines
BrandingText "MEGA Limited"
!define PRODUCT_NAME "MEGAcmd"

VIAddVersionKey "CompanyName" "MEGA Limited"
VIAddVersionKey "FileDescription" "MEGAcmd"
VIAddVersionKey "LegalCopyright" "MEGA Limited 2017"
VIAddVersionKey "ProductName" "MEGAcmd"

; Version info
VIProductVersion "0.9.4.0"
VIAddVersionKey "FileVersion" "0.9.4.0"
VIAddVersionKey "ProductVersion" "0.9.4.0"
!define PRODUCT_VERSION "0.9.4"

!define PRODUCT_PUBLISHER "Mega Limited"
!define PRODUCT_WEB_SITE "http://www.mega.nz"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\MEGAcmdShell.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"
!define CSIDL_STARTUP '0x7' ;Startup path
!define CSIDL_LOCALAPPDATA '0x1C' ;Local Application Data path
!define CSIDL_COMMON_APPDATA '0x23'

!define BUILDFOLDER_X86 "Release_x32"
!define BUILDFOLDER_X64 "Release_x64"

!ifdef BUILD_X64_VERSION
!define SRCDIR_MEGACMD "..\..\..\contrib\QtCreator\${BUILDFOLDER_X64}\MEGAcmdServer\release"
!else
!define SRCDIR_MEGACMD "..\..\..\contrib\QtCreator\${BUILDFOLDER_X86}\MEGAcmdServer\release"
!endif

!define SRCDIR_BATFILES "..\client\win"

!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_EXECUTIONLEVEL Standard
!define MULTIUSER_EXECUTIONLEVEL_ALLUSERS
!define MULTIUSER_INSTALLMODE_DEFAULT_CURRENTUSER

!define MEGA_DATA "mega.ini"
!define UNINSTALLER_NAME "uninst.exe"

!include "MUI2.nsh"
!include "Library.nsh"
!include "UAC.nsh"
!include "MultiUser.nsh"
!include "x64.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "installer\app_ico.ico"
!define MUI_UNICON "installer\uninstall_ico.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Settings
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "MEGAcmd"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!define MUI_FINISHPAGE_RUN ;"$INSTDIR\MEGAcmdShell.exe"
!define MUI_FINISHPAGE_RUN_FUNCTION RunFunction

!define MUI_WELCOMEFINISHPAGE_BITMAP "installer\left_banner.bmp"
;!define MUI_FINISHPAGE_NOAUTOCLOSE

var APP_NAME
var ICONS_GROUP
var USERNAME
var CURRENT_USER_INSTDIR
var ALL_USERS_INSTDIR

; Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "installer\terms.txt"
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "Afrikaans"       ;af
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Arabic"          ;ar
!insertmacro MUI_LANGUAGE "Armenian"
!insertmacro MUI_LANGUAGE "Basque"          ;eu
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Bosnian"         ;bs
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Bulgarian"       ;bg
!insertmacro MUI_LANGUAGE "Catalan"         ;ca
!insertmacro MUI_LANGUAGE "Cibemba"
!insertmacro MUI_LANGUAGE "Croatian"        ;hr
!insertmacro MUI_LANGUAGE "Czech"           ;cs
!insertmacro MUI_LANGUAGE "Danish"          ;da
!insertmacro MUI_LANGUAGE "Dutch"           ;nl
!insertmacro MUI_LANGUAGE "Efik" 			;locale code not found
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Esperanto"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Farsi" 			;locale code not found
!insertmacro MUI_LANGUAGE "Finnish"         ;fi
!insertmacro MUI_LANGUAGE "French"          ;fr
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Georgian"
!insertmacro MUI_LANGUAGE "German"            ;de
!insertmacro MUI_LANGUAGE "Greek"             ;el
!insertmacro MUI_LANGUAGE "Hebrew"            ;he
!insertmacro MUI_LANGUAGE "Hindi"             ;hi
!insertmacro MUI_LANGUAGE "Hungarian"         ;hu
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Igbo"
!insertmacro MUI_LANGUAGE "Indonesian"        ;in
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Italian"           ;it
!insertmacro MUI_LANGUAGE "Japanese"          ;ja
!insertmacro MUI_LANGUAGE "Khmer"
!insertmacro MUI_LANGUAGE "Korean"            ;ko
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Latvian"           ;lv
!insertmacro MUI_LANGUAGE "Lithuanian"        ;lt
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Macedonian"        ;mk
!insertmacro MUI_LANGUAGE "Malagasy"
!insertmacro MUI_LANGUAGE "Malay"             ;ms
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Norwegian"         ;no
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Polish"            ;pl
!insertmacro MUI_LANGUAGE "Portuguese"        ;pt
!insertmacro MUI_LANGUAGE "PortugueseBR"      ;pt_BR
!insertmacro MUI_LANGUAGE "Romanian"          ;ro
!insertmacro MUI_LANGUAGE "Russian"           ;ru
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Sesotho"
!insertmacro MUI_LANGUAGE "SimpChinese"       ;zh_CN
!insertmacro MUI_LANGUAGE "Slovak"            ;sk
!insertmacro MUI_LANGUAGE "Slovenian"         ;sl
!insertmacro MUI_LANGUAGE "Spanish"           ;es
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "Swahili"
!insertmacro MUI_LANGUAGE "Swedish"          ;sv
!insertmacro MUI_LANGUAGE "Tamil"
!insertmacro MUI_LANGUAGE "Thai"             ;th
!insertmacro MUI_LANGUAGE "TradChinese"      ;zh_TW
!insertmacro MUI_LANGUAGE "Turkish"          ;tr
!insertmacro MUI_LANGUAGE "Twi"
!insertmacro MUI_LANGUAGE "Ukrainian"        ;uk
!insertmacro MUI_LANGUAGE "Uyghur"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Vietnamese"       ;vn
!insertmacro MUI_LANGUAGE "Welsh"            ;cy
!insertmacro MUI_LANGUAGE "Yoruba"
!insertmacro MUI_LANGUAGE "Zulu"

; MUI end ------

Name $APP_NAME
!ifdef BUILD_UNINSTALLER
OutFile "UninstallerGenerator.exe"
!else
OutFile "MEGAcmdSetup.exe"
!endif

InstallDir "$PROGRAMFILES\MEGAcmd"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails nevershow
ShowUnInstDetails nevershow

Function RunFunction
  ${UAC.CallFunctionAsUser} RunMegaCmd
FunctionEnd

Function RunMegaCmd
  Exec "$INSTDIR\MEGAcmdShell.exe"
  Sleep 2000
FunctionEnd

Function .onInit
  !insertmacro MULTIUSER_INIT
  StrCpy $APP_NAME "${PRODUCT_NAME} ${PRODUCT_VERSION}"
  
  !ifdef BUILD_UNINSTALLER
         WriteUninstaller "$EXEDIR\${UNINSTALLER_NAME}"
         Quit
  !endif
 
  ;!insertmacro MUI_UNGETLANGUAGE
  !insertmacro MUI_LANGDLL_DISPLAY
  
FunctionEnd

Section "Principal" SEC01
  !insertmacro DEBUG_MSG "Getting needed information"
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_COMMON_APPDATA}, i0)i.r0'
  strCpy $ALL_USERS_INSTDIR $1
  
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_COMMON_APPDATA}, i0)i.r0'
  strCpy $ALL_USERS_INSTDIR $1
  
  System::Call "advapi32::GetUserName(t .r0, *i ${NSIS_MAX_STRLEN} r1) i.r2"
  strCpy $USERNAME $0
  
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_LOCALAPPDATA}, i0)i.r0'
  strCpy $CURRENT_USER_INSTDIR $1

  StrCmp "CurrentUser" $MultiUser.InstallMode currentuser
  !insertmacro DEBUG_MSG "Install for all"
  SetShellVarContext all
  StrCpy $INSTDIR "$ALL_USERS_INSTDIR\MEGAcmd"
  goto modeselected
currentuser:
 !insertmacro DEBUG_MSG "Install for current user"
  SetShellVarContext current
  StrCpy $INSTDIR "$CURRENT_USER_INSTDIR\MEGAcmd"
modeselected:

  ;SetRebootFlag true
  SetOverwrite try
  SetOutPath "$INSTDIR"
  
  File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vcruntime140.dll"
  AccessControl::SetFileOwner "$INSTDIR\vcruntime140.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\vcruntime140.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvcp140.dll"
  AccessControl::SetFileOwner "$INSTDIR\msvcp140.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\msvcp140.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\concrt140.dll"
  AccessControl::SetFileOwner "$INSTDIR\concrt140.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\concrt140.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vccorlib140.dll"
  AccessControl::SetFileOwner "$INSTDIR\vccorlib140.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\vccorlib140.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\ucrtbase.dll"
  AccessControl::SetFileOwner "$INSTDIR\ucrtbase.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\ucrtbase.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-utility-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-utility-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-utility-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite" 

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-time-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-time-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-time-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-string-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-string-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-string-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-stdio-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-stdio-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-stdio-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-runtime-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-runtime-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-runtime-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-process-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-process-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-process-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-private-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-private-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-private-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-multibyte-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-multibyte-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-multibyte-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-math-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-math-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-math-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-locale-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-locale-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-locale-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-heap-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-heap-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-heap-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-filesystem-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-filesystem-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-filesystem-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-environment-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-environment-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-environment-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-convert-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-convert-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-convert-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-crt-conio-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-crt-conio-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-crt-conio-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-util-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-util-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-util-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-timezone-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-timezone-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-timezone-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-sysinfo-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-sysinfo-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-sysinfo-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-synch-l1-2-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-synch-l1-2-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-synch-l1-2-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-synch-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-synch-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-synch-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-string-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-string-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-string-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-rtlsupport-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-rtlsupport-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-rtlsupport-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-profile-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-profile-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-profile-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  
  
  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-processthreads-l1-1-1.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-processthreads-l1-1-1.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-processthreads-l1-1-1.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-processthreads-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-processthreads-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-processthreads-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-processenvironment-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-processenvironment-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-processenvironment-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-namedpipe-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-namedpipe-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-namedpipe-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-memory-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-memory-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-memory-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-localization-l1-2-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-localization-l1-2-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-localization-l1-2-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-libraryloader-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-libraryloader-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-libraryloader-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-interlocked-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-interlocked-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-interlocked-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-heap-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-heap-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-heap-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-handle-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-handle-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-handle-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-file-l2-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-file-l2-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-file-l2-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-file-l1-2-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-file-l1-2-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-file-l1-2-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-file-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-file-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-file-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-errorhandling-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-errorhandling-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-errorhandling-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-debug-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-debug-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-debug-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-datetime-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-datetime-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-datetime-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  File "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api-ms-win-core-console-l1-1-0.dll"
  AccessControl::SetFileOwner "$INSTDIR\api-ms-win-core-console-l1-1-0.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\api-ms-win-core-console-l1-1-0.dll" "$USERNAME" "GenericRead + GenericWrite"  

  !insertmacro DEBUG_MSG "Closing MEGAcmd"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmdShell.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmd.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAclient.exe"
  
  !insertmacro DEBUG_MSG "Installing files"  

!ifndef BUILD_UNINSTALLER  ; if building uninstaller, skip files below        
  SetOutPath "$INSTDIR"
  SetOverwrite on
  AllowSkipFiles off
  File "${SRCDIR_MEGACMD}\MEGAcmd.exe"
  AccessControl::SetFileOwner "$INSTDIR\MEGAcmd.exe" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\MEGAcmd.exe" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\..\..\MEGAcmdClient\release\MEGAclient.exe"
  AccessControl::SetFileOwner "$INSTDIR\MEGAclient.exe" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\MEGAclient.exe" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\..\..\MEGAcmdShell\release\MEGAcmdShell.exe"
  AccessControl::SetFileOwner "$INSTDIR\MEGAcmdShell.exe" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\MEGAcmdShell.exe" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\libeay32.dll"
  AccessControl::SetFileOwner "$INSTDIR\libeay32.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\libeay32.dll" "$USERNAME" "GenericRead + GenericWrite"
  
  File "${SRCDIR_MEGACMD}\ssleay32.dll"
  AccessControl::SetFileOwner "$INSTDIR\ssleay32.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\ssleay32.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\libcurl.dll"
  AccessControl::SetFileOwner "$INSTDIR\libcurl.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\libcurl.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\cares.dll"
  AccessControl::SetFileOwner "$INSTDIR\cares.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\cares.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\libsodium.dll"
  AccessControl::SetFileOwner "$INSTDIR\libsodium.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\libsodium.dll" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_MEGACMD}\FreeImage.dll"
  AccessControl::SetFileOwner "$INSTDIR\FreeImage.dll" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\FreeImage.dll" "$USERNAME" "GenericRead + GenericWrite"

; BAT files

  File "${SRCDIR_BATFILES}\mega-attr.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-attr.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-attr.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-cd.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-cd.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-cd.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-confirm.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-confirm.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-confirm.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-cp.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-cp.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-cp.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-debug.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-debug.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-debug.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-du.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-du.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-du.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-export.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-export.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-export.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-find.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-find.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-find.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-get.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-get.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-get.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-help.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-help.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-help.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-https.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-https.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-https.bat" "$USERNAME" "GenericRead + GenericWrite"
 
  File "${SRCDIR_BATFILES}\mega-transfers.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-transfers.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-transfers.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-import.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-import.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-import.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-invite.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-invite.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-invite.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-ipc.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-ipc.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-ipc.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-killsession.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-killsession.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-killsession.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-lcd.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-lcd.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-lcd.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-log.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-log.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-log.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-login.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-login.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-login.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-logout.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-logout.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-logout.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-lpwd.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-lpwd.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-lpwd.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-ls.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-ls.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-ls.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-mkdir.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-mkdir.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-mkdir.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-mount.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-mount.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-mount.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-mv.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-mv.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-mv.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-passwd.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-passwd.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-passwd.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-preview.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-preview.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-preview.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-put.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-put.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-put.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-pwd.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-pwd.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-pwd.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-quit.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-quit.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-quit.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-reload.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-reload.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-reload.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-rm.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-rm.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-rm.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-session.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-session.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-session.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-share.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-share.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-share.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-showpcr.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-showpcr.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-showpcr.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-signup.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-signup.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-signup.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-speedlimit.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-speedlimit.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-speedlimit.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-sync.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-sync.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-sync.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-thumbnail.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-thumbnail.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-thumbnail.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-userattr.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-userattr.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-userattr.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-users.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-users.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-users.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-version.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-version.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-version.bat" "$USERNAME" "GenericRead + GenericWrite"

  File "${SRCDIR_BATFILES}\mega-whoami.bat"
  AccessControl::SetFileOwner "$INSTDIR\mega-whoami.bat" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\mega-whoami.bat" "$USERNAME" "GenericRead + GenericWrite"

; Uninstaller

  File "${UNINSTALLER_NAME}"
  AccessControl::SetFileOwner "$INSTDIR\${UNINSTALLER_NAME}" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\${UNINSTALLER_NAME}" "$USERNAME" "GenericRead + GenericWrite"
!endif
   
  !insertmacro DEBUG_MSG "Creating shortcuts"
  SetRebootFlag false
  StrCmp "CurrentUser" $MultiUser.InstallMode currentuser2
  SetShellVarContext all
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk" "$INSTDIR\MEGAcmdShell.exe"
  CreateShortCut "$DESKTOP\MEGAcmd.lnk" "$INSTDIR\MEGAcmdShell.exe"
  WriteIniStr "$INSTDIR\MEGA Website.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGA Website.lnk" "$INSTDIR\MEGA Website.url" "" "$INSTDIR\MEGAcmdShell.exe" 1
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall MEGAcmd.lnk" "$INSTDIR\${UNINSTALLER_NAME}"
  !insertmacro MUI_STARTMENU_WRITE_END
  goto modeselected2
currentuser2:
  SetShellVarContext current
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk" "$INSTDIR\MEGAcmdShell.exe"
  CreateShortCut "$DESKTOP\MEGAcmd.lnk" "$INSTDIR\MEGAcmdShell.exe"

  WriteIniStr "$INSTDIR\MEGA Website.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGA Website.lnk" "$INSTDIR\MEGA Website.url" "" "$INSTDIR\MEGAcmdShell.exe" 1
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall MEGAcmd.lnk" "$INSTDIR\${UNINSTALLER_NAME}"
  !insertmacro MUI_STARTMENU_WRITE_END
modeselected2:

SectionEnd

Section -AdditionalIcons

SectionEnd

Section -Post
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\MEGAcmdShell.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\${UNINSTALLER_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\MEGAcmdShell.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" ""
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  AccessControl::SetFileOwner "$INSTDIR\MEGA Website.url" "$USERNAME"
  AccessControl::GrantOnFile "$INSTDIR\MEGA Website.url" "$USERNAME" "GenericRead + GenericWrite"  
SectionEnd

Function un.onInit
!insertmacro MULTIUSER_UNINIT
StrCpy $APP_NAME "${PRODUCT_NAME}"

ReadIniStr $0 "$ExeDir\${MEGA_DATA}" UAC first
${IF} $0 <> 1
	;SetSilent silent
	InitPluginsDir
	WriteIniStr "$PluginsDir\${MEGA_DATA}" UAC first 1
	CopyFiles /SILENT "$EXEPATH" "$PluginsDir\${UNINSTALLER_NAME}"
	ExecWait '"$PluginsDir\${UNINSTALLER_NAME}" _?=$INSTDIR' $0
	SetErrorLevel $0
	Quit
${EndIf}

!insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Section Uninstall
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmdShell.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAclient.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmd.exe"

  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\${UNINSTALLER_NAME}"

  ;VC++ Redistributable
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\concrt140.dll"
  Delete "$INSTDIR\vccorlib140.dll"
  Delete "$INSTDIR\ucrtbase.dll"
  Delete "$INSTDIR\api-ms-win-crt-utility-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-time-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-string-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-stdio-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-runtime-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-process-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-private-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-multibyte-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-math-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-locale-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-heap-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-filesystem-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-environment-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-convert-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-crt-conio-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-util-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-timezone-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-sysinfo-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-synch-l1-2-0.dll"
  Delete "$INSTDIR\api-ms-win-core-synch-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-string-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-rtlsupport-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-profile-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-processthreads-l1-1-1.dll"
  Delete "$INSTDIR\api-ms-win-core-processthreads-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-processenvironment-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-namedpipe-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-memory-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-localization-l1-2-0.dll"
  Delete "$INSTDIR\api-ms-win-core-libraryloader-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-interlocked-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-heap-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-handle-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-file-l2-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-file-l1-2-0.dll"
  Delete "$INSTDIR\api-ms-win-core-file-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-errorhandling-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-debug-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-datetime-l1-1-0.dll"
  Delete "$INSTDIR\api-ms-win-core-console-l1-1-0.dll"
  
  ;Common files
  Delete "$INSTDIR\MEGAcmd.exe"
  Delete "$INSTDIR\MEGAcmdShell.exe"
  Delete "$INSTDIR\MEGAclient.exe"
  Delete "$INSTDIR\libeay32.dll"
  Delete "$INSTDIR\ssleay32.dll"
  Delete "$INSTDIR\libcurl.dll"
  Delete "$INSTDIR\cares.dll"
  Delete "$INSTDIR\libsodium.dll"
  Delete "$INSTDIR\FreeImage.dll"
  
  ; BAT files
  Delete "$INSTDIR\mega-attr.bat"
  Delete "$INSTDIR\mega-cd.bat"
  Delete "$INSTDIR\mega-confirm.bat"
  Delete "$INSTDIR\mega-cp.bat"
  Delete "$INSTDIR\mega-debug.bat"
  Delete "$INSTDIR\mega-du.bat"
  Delete "$INSTDIR\mega-export.bat"
  Delete "$INSTDIR\mega-find.bat"
  Delete "$INSTDIR\mega-get.bat"
  Delete "$INSTDIR\mega-help.bat"
  Delete "$INSTDIR\mega-https.bat"
  Delete "$INSTDIR\mega-transfers.bat"
  Delete "$INSTDIR\mega-import.bat"
  Delete "$INSTDIR\mega-invite.bat"
  Delete "$INSTDIR\mega-ipc.bat"
  Delete "$INSTDIR\mega-killsession.bat"
  Delete "$INSTDIR\mega-lcd.bat"
  Delete "$INSTDIR\mega-log.bat"
  Delete "$INSTDIR\mega-login.bat"
  Delete "$INSTDIR\mega-logout.bat"
  Delete "$INSTDIR\mega-lpwd.bat"
  Delete "$INSTDIR\mega-ls.bat"
  Delete "$INSTDIR\mega-mkdir.bat"
  Delete "$INSTDIR\mega-mount.bat"
  Delete "$INSTDIR\mega-mv.bat"
  Delete "$INSTDIR\mega-passwd.bat"
  Delete "$INSTDIR\mega-preview.bat"
  Delete "$INSTDIR\mega-put.bat"
  Delete "$INSTDIR\mega-pwd.bat"
  Delete "$INSTDIR\mega-quit.bat"
  Delete "$INSTDIR\mega-reload.bat"
  Delete "$INSTDIR\mega-rm.bat"
  Delete "$INSTDIR\mega-session.bat"
  Delete "$INSTDIR\mega-share.bat"
  Delete "$INSTDIR\mega-showpcr.bat"
  Delete "$INSTDIR\mega-signup.bat"
  Delete "$INSTDIR\mega-speedlimit.bat"
  Delete "$INSTDIR\mega-sync.bat"
  Delete "$INSTDIR\mega-thumbnail.bat"
  Delete "$INSTDIR\mega-userattr.bat"
  Delete "$INSTDIR\mega-users.bat"
  Delete "$INSTDIR\mega-version.bat"
  Delete "$INSTDIR\mega-whoami.bat"

  ; Cache
  RMDir /r "$INSTDIR\.megaCmd"
  
  SetShellVarContext current
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGA Website.lnk"
  Delete "$INSTDIR\MEGA Website.url"
  Delete "$DESKTOP\MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk"
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_STARTUP}, i0)i.r0'
  Delete "$1\MEGAcmd.lnk"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  RMDir "$INSTDIR"
  
  SetShellVarContext all
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGA Website.lnk"
  Delete "$INSTDIR\MEGA Website.url"
  Delete "$DESKTOP\MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk"
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_STARTUP}, i0)i.r0'
  Delete "$1\MEGAcmd.lnk"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
  SetRebootFlag false
SectionEnd
