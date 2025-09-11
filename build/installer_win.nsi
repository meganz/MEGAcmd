
ManifestDPIAware true

RequestExecutionLevel user
Unicode true

#!define BUILD_UNINSTALLER
#!define BUILD_X64_VERSION
#!define ENABLE_DEBUG_MESSAGES

#!define WINKITVER "${WINKITVER}"

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
VIAddVersionKey "LegalCopyright" "MEGA Limited 2019"
VIAddVersionKey "ProductName" "MEGAcmd"

!define PRODUCT_PUBLISHER "Mega Limited"
!define PRODUCT_WEB_SITE "http://www.mega.nz"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\MEGAcmdShell.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"
!define CSIDL_STARTUP '0x7' ;Startup path
!define CSIDL_LOCALAPPDATA '0x1C' ;Local Application Data path
!define CSIDL_COMMON_APPDATA '0x23'

; To be defined depending on your working environment

!define BUILDPATH_X64 "built64"
!ifndef BUILD_X64_VERSION
!define BUILDPATH_X86 "built32"
!endif

!ifdef BUILD_X64_VERSION
!define SRCDIR_MEGACMD "${BUILDPATH_X64}"
!else
!define SRCDIR_MEGACMD "${BUILDPATH_X86}"
!endif

!define SRCDIR_BATFILES "..\src\client\win"

; Version info: get version directly from the binary
!getdllversion "${SRCDIR_MEGACMD}/MEGAcmdServer.exe" Expv_
VIAddVersionKey "FileVersion" "${Expv_1}.${Expv_2}.${Expv_3}.${Expv_4}"
!ifdef VERSION_SUFFIX
VIProductVersion "${Expv_1}.${Expv_2}.${Expv_3}.${Expv_4}-${VERSION_SUFFIX}"
VIAddVersionKey "ProductVersion" "${Expv_1}.${Expv_2}.${Expv_3}.${Expv_4}-${VERSION_SUFFIX}"
!define PRODUCT_VERSION "${Expv_1}.${Expv_2}.${Expv_3}-${VERSION_SUFFIX}"
!else
VIProductVersion "${Expv_1}.${Expv_2}.${Expv_3}.${Expv_4}"
VIAddVersionKey "ProductVersion" "${Expv_1}.${Expv_2}.${Expv_3}.${Expv_4}"
!define PRODUCT_VERSION "${Expv_1}.${Expv_2}.${Expv_3}"
!endif

!define VcRedistBasePath "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.42.34433"
!define VcRedist32BasePath "${VcRedistBasePath}\x86"
!define VcRedist32Path "${VcRedist32BasePath}\Microsoft.VC143.CRT"
!define VcRedist64BasePath "${VcRedistBasePath}\x64"
!define VcRedist64Path "${VcRedist64BasePath}\Microsoft.VC143.CRT"

!define WinRedistBasePath "C:\Program Files (x86)\Windows Kits\10\Redist\${WINKITVER}\ucrt\DLLs"
!define WinRedist32BasePath "${winRedistBasePath}\x86"
!define WinRedist64BasePath "${winRedistBasePath}\x64"

!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_EXECUTIONLEVEL Standard
!define MULTIUSER_EXECUTIONLEVEL_ALLUSERS
!define MULTIUSER_INSTALLMODE_DEFAULT_CURRENTUSER

!define WinFspInstaller "winfsp-installer.msi"

!define MEGA_DATA "mega.ini"
!define UNINSTALLER_NAME "uninst.exe"

!include "MUI2.nsh"
!include "Library.nsh"
!include "UAC.nsh"
!include "MultiUser.nsh"
!include "x64.nsh"
!include "WinVer.nsh"


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
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
Page Custom WinFspPage WinFspPageLeave
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
UninstPage Custom un.WinFspUninstall un.WinFspUninstallLeave
!insertmacro MUI_UNPAGE_FINISH

; Language files (the ones available in MEGAsync (not MEGAcmd), with locale code)
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
#!insertmacro MUI_LANGUAGE "Cibemba"
!insertmacro MUI_LANGUAGE "Croatian"        ;hr
!insertmacro MUI_LANGUAGE "Czech"           ;cs
!insertmacro MUI_LANGUAGE "Danish"          ;da
!insertmacro MUI_LANGUAGE "Dutch"           ;nl
#!insertmacro MUI_LANGUAGE "Efik" 			;locale code not found
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
#!insertmacro MUI_LANGUAGE "Hindi"             ;hi
!insertmacro MUI_LANGUAGE "Hungarian"         ;hu
!insertmacro MUI_LANGUAGE "Icelandic"
#!insertmacro MUI_LANGUAGE "Igbo"
!insertmacro MUI_LANGUAGE "Indonesian"        ;in
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Italian"           ;it
!insertmacro MUI_LANGUAGE "Japanese"          ;ja
#!insertmacro MUI_LANGUAGE "Khmer"
!insertmacro MUI_LANGUAGE "Korean"            ;ko
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Latvian"           ;lv
!insertmacro MUI_LANGUAGE "Lithuanian"        ;lt
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Macedonian"        ;mk
#!insertmacro MUI_LANGUAGE "Malagasy"
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
#!insertmacro MUI_LANGUAGE "Sesotho"
!insertmacro MUI_LANGUAGE "SimpChinese"       ;zh_CN
!insertmacro MUI_LANGUAGE "Slovak"            ;sk
!insertmacro MUI_LANGUAGE "Slovenian"         ;sl
!insertmacro MUI_LANGUAGE "Spanish"           ;es
!insertmacro MUI_LANGUAGE "SpanishInternational"
#!insertmacro MUI_LANGUAGE "Swahili"
!insertmacro MUI_LANGUAGE "Swedish"          ;sv
#!insertmacro MUI_LANGUAGE "Tamil"
!insertmacro MUI_LANGUAGE "Thai"             ;th
!insertmacro MUI_LANGUAGE "TradChinese"      ;zh_TW
!insertmacro MUI_LANGUAGE "Turkish"          ;tr
#!insertmacro MUI_LANGUAGE "Twi"
!insertmacro MUI_LANGUAGE "Ukrainian"        ;uk
#!insertmacro MUI_LANGUAGE "Uyghur"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Vietnamese"       ;vn
!insertmacro MUI_LANGUAGE "Welsh"            ;cy
#!insertmacro MUI_LANGUAGE "Yoruba"
#!insertmacro MUI_LANGUAGE "Zulu"

; MUI end ------

Name $APP_NAME
!ifdef BUILD_UNINSTALLER
OutFile "UninstallerGenerator.exe"
!else
!pragma warning disable 6020 ; Disable warning: Uninstaller script code found but WriteUninstaller never used - no uninstaller will be created
!ifdef BUILD_X64_VERSION
OutFile "MEGAcmdSetup64.exe"
!else
OutFile "MEGAcmdSetup32.exe"
!endif
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

Var Dialog
Var WinFspCheckbox
Var WinFspCheckboxState
Var UninstallWinFspCheckbox
Var WinFspAlreadyInstalled
Var KeepWinFspAtUninstall
Var WinFspGuid
Var WinFspVer

Function WinFspPage
    ${If} $WinFspAlreadyInstalled == "True"
        WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "KeepWinFspAtUninstall" "True"
        Abort
    ${EndIf}

    nsDialogs::Create 1018
    Pop $Dialog

    ${If} $Dialog == error
            Abort
    ${EndIf}

    ${NSD_CreateLabel} 0 10u 100% 12u "${PRODUCT_NAME} includes a feature feature called FUSE."
    ${NSD_CreateLabel} 0 22u 100% 24u "FUSE allows you to view the content of your MEGA folders on your computer without the need to download them. In order for FUSE to work, you need to install WinFSP."

    ${NSD_CreateCheckbox} 0 54u 100% 12u "Install WinFSP"
    Pop $WinFspCheckbox

    ${NSD_CreateLabel} 0 74u 100% 24u "If you don't install WinFSP now, you won't be able to establish FUSE mounts."
    ${NSD_CreateLabel} 0 98u 100% 24u "Please, tick above checkbox to install it in your system."

    ${NSD_SetState} $WinFspCheckbox $WinFspCheckboxState

    nsDialogs::Show
FunctionEnd

Var PREVIOUS_OUTPATH

Function WinFspPageLeave
    ${NSD_GetState} $WinFspCheckbox $WinFspCheckboxState

    ReadRegStr $0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "KeepWinFspAtUninstall"
    ${If} $0 == ""
        ${If} $WinFspCheckboxState == ${BST_CHECKED}
          StrCpy $KeepWinFspAtUninstall "False"
        ${Else}
          StrCpy $KeepWinFspAtUninstall "True"
        ${EndIf}
    ${Else}
      ; In case MEGAcmd was already installed, don't override WinFSP initial installation value
      StrCpy $KeepWinFspAtUninstall "$0"
    ${EndIf}

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "KeepWinFspAtUninstall" "$KeepWinFspAtUninstall"

    ${If} $WinFspCheckboxState == ${BST_CHECKED}
      strCpy $PREVIOUS_OUTPATH GetOutPath
      SetOutPath "$TEMP"

      File "${WinFspInstaller}"
      SetOutPath $PREVIOUS_OUTPATH

      ExecWait 'msiexec /i "$TEMP\\${WinFspInstaller}" /qb'

      Delete "$TEMP\\${WinFspInstaller}"
    ${EndIf}
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

  ${NSD_SetState} $WinFspCheckbox ${BST_CHECKED}
  StrCpy $WinFspCheckboxState ${BST_CHECKED}
  ReadRegStr $0 HKLM "SOFTWARE\Classes\Installer\Dependencies\WinFsp" ""
  ${If} $0 == ""
    StrCpy $WinFspAlreadyInstalled "False"
  ${Else}
    StrCpy $WinFspAlreadyInstalled "True"
  ${EndIf}


!ifdef BUILD_X64_VERSION
  ${If} ${RunningX64}
  ${Else}
    MessageBox MB_OK "This is an installer for 64-bit MEGAcmd, but you are using a 32-bit Windows. Please, download the 32-bit MEGAcmd version from https://mega.nz/cmd."
    Quit
  ${EndIf}
!endif


${IfNot} ${AtLeastWin8}
  MessageBox MB_OK "This MEGAcmd installer is for Windows 8 or above"
  Quit
${EndIf}

UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights at least in win10; TODO: only ask for this if that's the case
MessageBox MB_YESNO "If you would like ${PRODUCT_NAME} to be listed in the installed applications, Admin Privileges are needed.  Press Yes to grant that, or No for a plain install." /SD IDNO IDYES elevate IDNO next
${EndIf}
  
elevate:
  UAC::RunElevated
  ${Switch} $0
  ${Case} 0
    ${IfThen} $1 = 1 ${|} Quit ${|} ;User process. The installer has finished. Quit.
    ${IfThen} $3 <> 0 ${|} ${Break} ${|} ;Admin process, continue the installation
    ${If} $1 = 3 ;RunAs completed successfully, but with a non-admin user
      ;MessageBox mb_YesNo|mb_IconExclamation|mb_TopMost|mb_SetForeground "This requires admin privileges, try again" /SD IDNO IDYES uac_tryagain IDNO 0
      Quit
    ${EndIf}
  ${Case} 1223
    ;MessageBox mb_IconStop|mb_TopMost|mb_SetForeground "This requires admin privileges, aborting!"
    Quit
  ${Default}
    MessageBox mb_IconStop|mb_TopMost|mb_SetForeground "This installer requires Administrator privileges. Error $0"
    Quit
  ${EndSwitch} 

  Goto next
next:
    !insertmacro DEBUG_MSG "exiting oninit"

FunctionEnd


Function GetPaths
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_COMMON_APPDATA}, i0)i.r0'
  strCpy $ALL_USERS_INSTDIR $1
  
  System::Call "advapi32::GetUserName(t .r0, *i ${NSIS_MAX_STRLEN} r1) i.r2"
  strCpy $USERNAME $0
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_LOCALAPPDATA}, i0)i.r0'
  strCpy $CURRENT_USER_INSTDIR $1
  
  ClearErrors
  FileOpen $0 "$ALL_USERS_INSTDIR\megatmp.M1.txt" w
  IfErrors done
  FileWriteUTF16LE $0 "$CURRENT_USER_INSTDIR"
  FileClose $0
  FileOpen $0 "$ALL_USERS_INSTDIR\megatmp.M2.txt" w
  IfErrors done
  FileWriteUTF16LE $0 "$USERNAME"
  FileClose $0
done:
FunctionEnd

!macro Install3264DLL  source target
  File "${source}"
  AccessControl::SetFileOwner ${target} "$USERNAME"
  AccessControl::GrantOnFile ${target} "$USERNAME" "GenericRead + GenericWrite"
!macroend


Section "Principal" SEC01

  !insertmacro DEBUG_MSG "Getting needed information"
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_COMMON_APPDATA}, i0)i.r0'
  strCpy $ALL_USERS_INSTDIR $1
  
  ${UAC.CallFunctionAsUser} GetPaths

readpaths:
  Sleep 1000

  FileOpen $R0 "$ALL_USERS_INSTDIR\megatmp.M1.txt" r
  IfErrors done
  FileReadUTF16LE $R0 "$CURRENT_USER_INSTDIR"
  FileOpen $R1 "$ALL_USERS_INSTDIR\megatmp.M2.txt" r
  IfErrors done
  FileReadUTF16LE $R1 "$USERNAME"
  IfErrors done
done:
  FileClose $R0
  FileClose $R1

  StrCmp $CURRENT_USER_INSTDIR "" readpaths 0
  StrCmp $USERNAME "" readpaths 0

  !insertmacro DEBUG_MSG "Checking install mode"
  Delete "$ALL_USERS_INSTDIR\megatmp.M1.txt"
  Delete "$ALL_USERS_INSTDIR\megatmp.M2.txt"
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


  !insertmacro DEBUG_MSG "Closing MEGAcmd"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmdShell.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmd.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmdServer.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAclient.exe"
  ExecDos::exec /DETAILED /DISABLEFSR "taskkill /f /IM MEGAcmdUpdater.exe"
  Sleep 1000


  !insertmacro DEBUG_MSG "Installing files"  

  ;SetRebootFlag true
  SetOverwrite on

  SetOutPath "$INSTDIR"

  !ifdef BUILD_X64_VERSION
    !insertmacro Install3264DLL "${VcRedist64Path}\vcruntime140.dll" "$INSTDIR\vcruntime140.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\vcruntime140_1.dll" "$INSTDIR\vcruntime140_1.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\msvcp140.dll" "$INSTDIR\msvcp140.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\msvcp140_1.dll" "$INSTDIR\msvcp140_1.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\msvcp140_2.dll" "$INSTDIR\msvcp140_2.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\msvcp140_atomic_wait.dll" "$INSTDIR\msvcp140_atomic_wait.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\msvcp140_codecvt_ids.dll" "$INSTDIR\msvcp140_codecvt_ids.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\concrt140.dll"  "$INSTDIR\concrt140.dll"
    !insertmacro Install3264DLL "${VcRedist64Path}\vccorlib140.dll" "$INSTDIR\vccorlib140.dll"
    !insertmacro Install3264DLL "${VcRedist64BasePath}\Microsoft.VC143.OpenMP\vcomp140.dll"  "$INSTDIR\vcomp140.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\ucrtbase.dll"  "$INSTDIR\ucrtbase.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-utility-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-utility-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-time-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-time-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-string-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-string-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-stdio-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-stdio-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-runtime-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-runtime-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-process-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-process-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-private-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-private-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-multibyte-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-multibyte-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-math-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-math-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-locale-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-locale-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-heap-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-heap-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-filesystem-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-filesystem-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-environment-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-environment-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-convert-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-convert-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-crt-conio-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-conio-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-util-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-util-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-timezone-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-timezone-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-sysinfo-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-sysinfo-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-synch-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-synch-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-synch-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-synch-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-string-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-string-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-rtlsupport-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-rtlsupport-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-profile-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-profile-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-processthreads-l1-1-1.dll"  "$INSTDIR\api-ms-win-core-processthreads-l1-1-1.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-processthreads-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-processthreads-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-processenvironment-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-processenvironment-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-namedpipe-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-namedpipe-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-memory-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-memory-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-localization-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-localization-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-libraryloader-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-libraryloader-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-interlocked-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-interlocked-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-heap-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-heap-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-handle-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-handle-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-file-l2-1-0.dll"  "$INSTDIR\api-ms-win-core-file-l2-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-file-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-file-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-file-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-file-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-errorhandling-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-errorhandling-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-debug-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-debug-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-datetime-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-datetime-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist64BasePath}\api-ms-win-core-console-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-console-l1-1-0.dll"
  !else
    !insertmacro Install3264DLL "${VcRedist32Path}\vcruntime140.dll" "$INSTDIR\vcruntime140.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\msvcp140.dll" "$INSTDIR\msvcp140.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\msvcp140_1.dll" "$INSTDIR\msvcp140_1.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\msvcp140_2.dll" "$INSTDIR\msvcp140_2.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\msvcp140_atomic_wait.dll" "$INSTDIR\msvcp140_atomic_wait.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\msvcp140_codecvt_ids.dll" "$INSTDIR\msvcp140_codecvt_ids.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\concrt140.dll"  "$INSTDIR\concrt140.dll"
    !insertmacro Install3264DLL "${VcRedist32Path}\vccorlib140.dll" "$INSTDIR\vccorlib140.dll"
    !insertmacro Install3264DLL "${VcRedist32BasePath}\Microsoft.VC143.OpenMP\vcomp140.dll"  "$INSTDIR\vcomp140.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\ucrtbase.dll"  "$INSTDIR\ucrtbase.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-utility-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-utility-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-time-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-time-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-string-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-string-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-stdio-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-stdio-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-runtime-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-runtime-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-process-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-process-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-private-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-private-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-multibyte-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-multibyte-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-math-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-math-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-locale-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-locale-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-heap-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-heap-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-filesystem-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-filesystem-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-environment-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-environment-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-convert-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-convert-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-crt-conio-l1-1-0.dll"  "$INSTDIR\api-ms-win-crt-conio-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-util-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-util-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-timezone-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-timezone-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-sysinfo-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-sysinfo-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-synch-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-synch-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-synch-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-synch-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-string-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-string-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-rtlsupport-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-rtlsupport-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-profile-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-profile-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-processthreads-l1-1-1.dll"  "$INSTDIR\api-ms-win-core-processthreads-l1-1-1.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-processthreads-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-processthreads-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-processenvironment-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-processenvironment-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-namedpipe-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-namedpipe-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-memory-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-memory-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-localization-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-localization-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-libraryloader-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-libraryloader-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-interlocked-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-interlocked-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-heap-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-heap-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-handle-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-handle-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-file-l2-1-0.dll"  "$INSTDIR\api-ms-win-core-file-l2-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-file-l1-2-0.dll"  "$INSTDIR\api-ms-win-core-file-l1-2-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-file-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-file-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-errorhandling-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-errorhandling-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-debug-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-debug-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-datetime-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-datetime-l1-1-0.dll"
    !insertmacro Install3264DLL "${WinRedist32BasePath}\api-ms-win-core-console-l1-1-0.dll"  "$INSTDIR\api-ms-win-core-console-l1-1-0.dll"
  !endif

!ifndef BUILD_UNINSTALLER  ; if building uninstaller, skip files below        

  ;x86_32 files

  Delete "$INSTDIR\MEGAcmd.exe" ; delete older name of server
  Delete "$INSTDIR\mega-history.bat" ; delete older bat

  SetOutPath "$INSTDIR"
  SetOverwrite on
  AllowSkipFiles off

  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\MEGAcmdServer.exe" "$INSTDIR\MEGAcmdServer.exe"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\MEGAclient.exe" "$INSTDIR\MEGAclient.exe"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\MEGAcmdShell.exe" "$INSTDIR\MEGAcmdShell.exe"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\MEGAcmdUpdater.exe" "$INSTDIR\MEGAcmdUpdater.exe" 

  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\avcodec-61.dll" "$INSTDIR\avcodec-61.dll"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\avformat-61.dll" "$INSTDIR\avformat-61.dll"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\avutil-59.dll" "$INSTDIR\avutil-59.dll"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\swscale-8.dll" "$INSTDIR\swscale-8.dll"
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\swresample-5.dll" "$INSTDIR\swresample-5.dll"

  ;remove old DLLs that we no longer use (some became static; some have later version number)
  Delete "$INSTDIR\avcodec-57.dll"
  Delete "$INSTDIR\avcodec-59.dll"
  Delete "$INSTDIR\avformat-57.dll"
  Delete "$INSTDIR\avformat-59.dll"
  Delete "$INSTDIR\avutil-55.dll"
  Delete "$INSTDIR\avutil-57.dll"
  Delete "$INSTDIR\swscale-4.dll"
  Delete "$INSTDIR\swscale-6.dll"
  Delete "$INSTDIR\swresample-2.dll"
  Delete "$INSTDIR\swresample-4.dll"
  Delete "$INSTDIR\libsodium.dll"
  Delete "$INSTDIR\pdfium.dll"
  Delete "$INSTDIR\FreeImage.dll"
  Delete "$INSTDIR\avcodec-58.dll"
  Delete "$INSTDIR\avformat-58.dll"
  Delete "$INSTDIR\avutil-56.dll"
  Delete "$INSTDIR\swscale-5.dll"
  Delete "$INSTDIR\swresample-3.dll"

  Delete "$INSTDIR\libcrypto-1_1-x64.dll"
  Delete "$INSTDIR\libssl-1_1-x64.dll"

  Delete "$INSTDIR\libcrypto-1_1.dll"
  Delete "$INSTDIR\libssl-1_1.dll"

; BAT files
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-attr.bat" "$INSTDIR\mega-attr.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-cd.bat" "$INSTDIR\mega-cd.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-confirm.bat" "$INSTDIR\mega-confirm.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-cp.bat" "$INSTDIR\mega-cp.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-debug.bat" "$INSTDIR\mega-debug.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-du.bat" "$INSTDIR\mega-du.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-df.bat" "$INSTDIR\mega-df.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-proxy.bat" "$INSTDIR\mega-proxy.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-export.bat" "$INSTDIR\mega-export.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-find.bat" "$INSTDIR\mega-find.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-get.bat" "$INSTDIR\mega-get.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-help.bat" "$INSTDIR\mega-help.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-https.bat" "$INSTDIR\mega-https.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-webdav.bat" "$INSTDIR\mega-webdav.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-deleteversions.bat" "$INSTDIR\mega-deleteversions.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-transfers.bat" "$INSTDIR\mega-transfers.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-import.bat" "$INSTDIR\mega-import.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-invite.bat" "$INSTDIR\mega-invite.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-ipc.bat" "$INSTDIR\mega-ipc.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-killsession.bat" "$INSTDIR\mega-killsession.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-lcd.bat" "$INSTDIR\mega-lcd.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-log.bat" "$INSTDIR\mega-log.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-login.bat" "$INSTDIR\mega-login.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-logout.bat" "$INSTDIR\mega-logout.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-lpwd.bat" "$INSTDIR\mega-lpwd.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-ls.bat" "$INSTDIR\mega-ls.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-backup.bat" "$INSTDIR\mega-backup.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-mkdir.bat" "$INSTDIR\mega-mkdir.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-mount.bat" "$INSTDIR\mega-mount.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-mv.bat" "$INSTDIR\mega-mv.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-passwd.bat" "$INSTDIR\mega-passwd.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-preview.bat" "$INSTDIR\mega-preview.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-put.bat" "$INSTDIR\mega-put.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-pwd.bat" "$INSTDIR\mega-pwd.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-quit.bat" "$INSTDIR\mega-quit.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-reload.bat" "$INSTDIR\mega-reload.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-rm.bat" "$INSTDIR\mega-rm.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-session.bat" "$INSTDIR\mega-session.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-share.bat" "$INSTDIR\mega-share.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-showpcr.bat" "$INSTDIR\mega-showpcr.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-signup.bat" "$INSTDIR\mega-signup.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-speedlimit.bat" "$INSTDIR\mega-speedlimit.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-sync.bat" "$INSTDIR\mega-sync.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-exclude.bat" "$INSTDIR\mega-exclude.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-thumbnail.bat" "$INSTDIR\mega-thumbnail.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-userattr.bat" "$INSTDIR\mega-userattr.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-users.bat" "$INSTDIR\mega-users.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-version.bat" "$INSTDIR\mega-version.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-whoami.bat" "$INSTDIR\mega-whoami.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-cat.bat" "$INSTDIR\mega-cat.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-tree.bat" "$INSTDIR\mega-tree.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-mediainfo.bat" "$INSTDIR\mega-mediainfo.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-graphics.bat" "$INSTDIR\mega-graphics.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-ftp.bat" "$INSTDIR\mega-ftp.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-cancel.bat" "$INSTDIR\mega-cancel.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-confirmcancel.bat" "$INSTDIR\mega-confirmcancel.bat"
  !insertmacro Install3264DLL "${SRCDIR_BATFILES}\mega-errorcode.bat" "$INSTDIR\mega-errorcode.bat"

; Uninstaller
;!ifndef BUILD_UNINSTALLER  ; if building uninstaller, skip this check
  !insertmacro Install3264DLL "${SRCDIR_MEGACMD}\${UNINSTALLER_NAME}" "$INSTDIR\${UNINSTALLER_NAME}"
!endif
   
  !insertmacro DEBUG_MSG "Creating shortcuts"
  SetRebootFlag false
  StrCmp "CurrentUser" $MultiUser.InstallMode currentuser2
  SetShellVarContext all
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk" "$INSTDIR\MEGAcmdShell.exe"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGAcmdServer.lnk" "$INSTDIR\MEGAcmdServer.exe"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Update MEGAcmd.lnk" "$INSTDIR\MEGAcmdUpdater.exe"
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
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\MEGAcmdServer.lnk" "$INSTDIR\MEGAcmdServer.exe"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Update MEGAcmd.lnk" "$INSTDIR\MEGAcmdUpdater.exe"
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

Function un.UninstallCmd
  ExecDos::exec "$INSTDIR\MEGAcmdServer.exe --uninstall"
FunctionEnd

Function un.WinFspUninstall
    !insertmacro MUI_HEADER_TEXT "Uninstallation complete" "${PRODUCT_NAME} was uninstalled successfully"

    ${If} $KeepWinFspAtUninstall == "True"
            Abort
    ${EndIf}

    ReadRegStr $WinFspGuid HKLM "SOFTWARE\Classes\Installer\Dependencies\WinFsp" ""
    ${If} $WinFspGuid == ""
            Abort
    ${EndIf}
    ReadRegStr $WinFspVer HKLM "SOFTWARE\Classes\Installer\Dependencies\WinFsp" "Version"
    ; TODO: improvement abort in case Version differs from the one installed by MEGAcmd
    nsDialogs::Create 1018
    Pop $Dialog

    ${If} $Dialog == error
            Abort
    ${EndIf}

    ${NSD_CreateLabel} 0 10u 100% 12u "WinFsp $WinFspVer was installed by ${PRODUCT_NAME} to support some features."
    ${NSD_CreateLabel} 0 22u 100% 24u "Do you also want to uninstall WinFsp?"

    ${NSD_CreateCheckbox} 0 54u 100% 12u "Uninstall WinFsp"
    Pop $UninstallWinFspCheckbox

    ${NSD_SetState} $UninstallWinFspCheckbox ${BST_CHECKED}

    nsDialogs::Show
FunctionEnd

Function un.WinFspUninstallLeave
    ${NSD_GetState} $UninstallWinFspCheckbox $WinFspCheckboxState
    ${If} $WinFspCheckboxState == ${BST_CHECKED}
      ExecWait 'msiexec /uninstall $WinFspGuid /qb'
    ${EndIf}
FunctionEnd


Section Uninstall
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmdShell.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAclient.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmd.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmdServer.exe"
  ExecDos::exec /DETAILED "taskkill /f /IM MEGAcmdUpdater.exe"
  Sleep 1000
  ${UAC.CallFunctionAsUser} un.UninstallCmd
  Sleep 1000

  
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\${UNINSTALLER_NAME}"

  ;VC++ Redistributable
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\msvcp140_1.dll"
  Delete "$INSTDIR\msvcp140_2.dll"
  Delete "$INSTDIR\msvcp140_codecvt_ids.dll"
  Delete "$INSTDIR\msvcp140_atomic_wait.dll"
  Delete "$INSTDIR\concrt140.dll"
  Delete "$INSTDIR\vccorlib140.dll"
  Delete "$INSTDIR\vcomp140.dll"
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
  Delete "$INSTDIR\MEGAcmdUpdater.exe"
  Delete "$INSTDIR\MEGAcmdServer.exe"
  Delete "$INSTDIR\MEGAcmd.exe"
  Delete "$INSTDIR\MEGAcmdShell.exe"
  Delete "$INSTDIR\MEGAclient.exe"
  Delete "$INSTDIR\libeay32.dll"
  Delete "$INSTDIR\ssleay32.dll"
  Delete "$INSTDIR\libcurl.dll"
  Delete "$INSTDIR\cares.dll"
  Delete "$INSTDIR\libuv.dll"
  Delete "$INSTDIR\NSIS.Library.RegTool*.exe"
  Delete "$INSTDIR\avcodec-61.dll"
  Delete "$INSTDIR\avformat-61.dll"
  Delete "$INSTDIR\avutil-59.dll"
  Delete "$INSTDIR\swscale-8.dll"
  Delete "$INSTDIR\swresample-5.dll"
;!ifdef BUILD_X64_VERSION
  Delete "$INSTDIR\libssl-3-x64.dll"
  Delete "$INSTDIR\libcrypto-3-x64.dll"
;!else
  Delete "$INSTDIR\libcrypto-3.dll"
  Delete "$INSTDIR\libssl-3.dll"
;!endif

  ;Still remove old DLLs though we no longer produce them (non-VCPKG may still produce them)
  Delete "$INSTDIR\avcodec-57.dll"
  Delete "$INSTDIR\avformat-57.dll"
  Delete "$INSTDIR\avutil-55.dll"
  Delete "$INSTDIR\swscale-4.dll"
  Delete "$INSTDIR\swresample-2.dll"
  Delete "$INSTDIR\libsodium.dll"
  Delete "$INSTDIR\pdfium.dll"
  Delete "$INSTDIR\FreeImage.dll"
  Delete "$INSTDIR\libcrypto-1_1-x64.dll"
  Delete "$INSTDIR\libssl-1_1-x64.dll"
  Delete "$INSTDIR\libcrypto-1_1.dll"
  Delete "$INSTDIR\libssl-1_1.dll"
  Delete "$INSTDIR\avcodec-58.dll"
  Delete "$INSTDIR\avformat-58.dll"
  Delete "$INSTDIR\avutil-56.dll"
  Delete "$INSTDIR\swscale-5.dll"
  Delete "$INSTDIR\swresample-3.dll"
  Delete "$INSTDIR\avcodec-59.dll"
  Delete "$INSTDIR\avformat-59.dll"
  Delete "$INSTDIR\avutil-57.dll"
  Delete "$INSTDIR\swscale-6.dll"
  Delete "$INSTDIR\swresample-4.dll"


  ; BAT files
  Delete "$INSTDIR\mega-attr.bat"
  Delete "$INSTDIR\mega-cd.bat"
  Delete "$INSTDIR\mega-confirm.bat"
  Delete "$INSTDIR\mega-cp.bat"
  Delete "$INSTDIR\mega-debug.bat"
  Delete "$INSTDIR\mega-du.bat"
  Delete "$INSTDIR\mega-df.bat"
  Delete "$INSTDIR\mega-proxy.bat"
  Delete "$INSTDIR\mega-export.bat"
  Delete "$INSTDIR\mega-find.bat"
  Delete "$INSTDIR\mega-get.bat"
  Delete "$INSTDIR\mega-help.bat"
  Delete "$INSTDIR\mega-history.bat"
  Delete "$INSTDIR\mega-https.bat"
  Delete "$INSTDIR\mega-webdav.bat"
  Delete "$INSTDIR\mega-deleteversions.bat"
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
  Delete "$INSTDIR\mega-backup.bat"
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
  Delete "$INSTDIR\mega-exclude.bat"
  Delete "$INSTDIR\mega-thumbnail.bat"
  Delete "$INSTDIR\mega-userattr.bat"
  Delete "$INSTDIR\mega-users.bat"
  Delete "$INSTDIR\mega-version.bat"
  Delete "$INSTDIR\mega-whoami.bat"
  Delete "$INSTDIR\mega-cat.bat"
  Delete "$INSTDIR\mega-tree.bat"
  Delete "$INSTDIR\mega-mediainfo.bat"
  Delete "$INSTDIR\mega-graphics.bat"
  Delete "$INSTDIR\mega-ftp.bat"
  Delete "$INSTDIR\mega-cancel.bat"
  Delete "$INSTDIR\mega-confirmcancel.bat"
  Delete "$INSTDIR\mega-errorcode.bat"

  ; Cache
  RMDir /r "$INSTDIR\.megaCmd"

  ReadRegStr $KeepWinFspAtUninstall ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "KeepWinFspAtUninstall"
  
  SetShellVarContext current
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGA Website.lnk"
  Delete "$INSTDIR\MEGA Website.url"
  Delete "$DESKTOP\MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGAcmd.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGAcmdServer.lnk"
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
  Delete "$SMPROGRAMS\$ICONS_GROUP\MEGAcmdServer.lnk"
  System::Call 'shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r1, i ${CSIDL_STARTUP}, i0)i.r0'
  Delete "$1\MEGAcmd.lnk"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
  SetRebootFlag false
SectionEnd
