;--------------------------------
;Includes

  !include "MUI2.nsh"

;--------------------------------
;Defines

  !define SHCNE_ASSOCCHANGED 0x8000000
  !define SHCNF_IDLIST 0

  !define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Euclideon udStream"
  !define /ifndef BUILD_SUFFIX ""

  !getdllversion "..\..\..\builds\udStream${BUILD_SUFFIX}.exe" VersionExpv_
  !define /ifndef VERSION '${VersionExpv_1}.${VersionExpv_2}.${VersionExpv_3}.${VersionExpv_4}'
  !define /ifndef VER_MAJOR ${VersionExpv_1}
  !define /ifndef VER_MINOR ${VersionExpv_2}
  !define /ifndef VER_REVISION ${VersionExpv_3}
  !define /ifndef VER_BUILD ${VersionExpv_4}

;--------------------------------
;General

  ;Name and file
  Name "udStream"
  OutFile "udStreamSetup.exe"
  Unicode True

  ;Default installation folder
  InstallDir "$PROGRAMFILES64\Euclideon\udStream"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Euclideon\udStream" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Header Files

  ;!include "WordFunc.nsh"

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

  !define MUI_ICON "..\..\..\icons\EuclideonClient.ico"
  !define MUI_UNICON "..\..\..\icons\EuclideonClient.ico"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "..\License.rtf"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Version information

  !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD
  VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}.${VER_BUILD}
  VIFileVersion ${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}.${VER_BUILD}
  VIAddVersionKey "ProductName" "udStream Setup"
  VIAddVersionKey "Company" "Euclideon Pty Ltd"
  VIAddVersionKey "FileVersion" "${VERSION}"
  VIAddVersionKey "ProductVersion" "${VERSION}"
  VIAddVersionKey "FileDescription" "udStream Setup"
  VIAddVersionKey "LegalCopyright" "© Euclideon Pty Ltd"
  !endif

;--------------------------------
;Init function
Function .onInit
  System::Call 'kernel32::CreateMutex(i 0, i 0, t "udStreamInstallerMutex") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
    Abort
FunctionEnd

;--------------------------------
;Installer Sections

Section "udStream" SecCore
  SectionInstType RO

  SetOutPath "$INSTDIR"
  
  ;Add files
  File ..\..\..\builds\udStream${BUILD_SUFFIX}.exe
  File ..\..\..\builds\udStreamConvertCMD.exe
  File ..\..\..\builds\vaultSDK.dll
  File ..\..\..\builds\SDL2.dll
  File /nonfatal ..\..\..\builds\libfbxsdk.dll
  File ..\..\..\builds\releasenotes.md
  File ..\..\..\builds\defaultsettings.json
  
  SetOutPath "$INSTDIR\assets"
  File /r ..\..\..\builds\assets\*.*
  SetOutPath "$INSTDIR"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Euclideon\udStream" "" $INSTDIR
  
  ;Write uninstall keys
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "Euclideon udStream"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" "$INSTDIR\uninstall.exe,0"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${VERSION}"
!ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "VersionMajor" "${VER_MAJOR}" ; Required by WACK
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "VersionMinor" "${VER_MINOR}" ; Required by WACK
!endif
  WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "Euclideon Pty Ltd" ; Required by WACK
  WriteRegStr HKLM "${REG_UNINST_KEY}" "URLInfoAbout" "https://www.euclideon.com/vault/"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "HelpLink" "https://www.euclideon.com/customerresourcepage/"
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair" 1
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

Section "File and Protocol Handlers" SecHandlers
  ;Register file extensions
  WriteRegStr HKCR ".uds" "" "udStream.Dataset"
  WriteRegStr HKCR ".udg" "" "udStream.Dataset"
  WriteRegStr HKCR ".ssf" "" "udStream.Dataset"
  WriteRegStr HKCR ".udp" "" "udStream.Project"
  WriteRegStr HKCR ".json\OpenWithProgids" "VisualStudio.json.68f8a8bc" ""
  WriteRegStr HKCR ".json\OpenWithProgids" "udStream.Project" ""
  
  ;Register udStream.Dataset handler
  WriteRegStr HKCR "udStream.Dataset" "" "udStream Dataset File"
  WriteRegStr HKCR "udStream.Dataset\DefaultIcon" "" "$INSTDIR\udStream${BUILD_SUFFIX}.exe,0"
  ReadRegStr $R0 HKCR "udStream.Dataset\shell\open\command" ""
  ${If} $R0 == ""
    WriteRegStr HKCR "udStream.Dataset\shell" "" "open"
    WriteRegStr HKCR "udStream.Dataset\shell\open\command" "" '$INSTDIR\udStream${BUILD_SUFFIX}.exe "%1"'
  ${EndIf}
  
  ;Register udStream.Project handler
  WriteRegStr HKCR "udStream.Project" "" "udStream Project File"
  WriteRegStr HKCR "udStream.Project\DefaultIcon" "" "$INSTDIR\udStream${BUILD_SUFFIX}.exe,0"
  ReadRegStr $R0 HKCR "udStream.Project\shell\open\command" ""
  ${If} $R0 == ""
    WriteRegStr HKCR "udStream.Project\shell" "" "open"
    WriteRegStr HKCR "udStream.Project\shell\open\command" "" '$INSTDIR\udStream${BUILD_SUFFIX}.exe "%1"'
  ${EndIf}
  
  ;Register euclideon protocol handler
  WriteRegStr HKCR "euclideon" "" "URL:euclideon Protocol"
  WriteRegStr HKCR "euclideon" "URL Protocol" ""
  WriteRegStr HKCR "euclideon\DefaultIcon" "" "$INSTDIR\udStream${BUILD_SUFFIX}.exe,0"
  WriteRegStr HKCR "euclideon\shell" "" "open"
  WriteRegStr HKCR "euclideon\shell\open\command" "" '$INSTDIR\udStream${BUILD_SUFFIX}.exe "%1"'
  
  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p0, p0)'
SectionEnd

Section "Start Menu Shortcut" SecStartMenu
  CreateShortcut "$SMPROGRAMS\udStream.lnk" "$INSTDIR\udStream${BUILD_SUFFIX}.exe"
SectionEnd

Section "Desktop Shortcut" SecDesktop
  CreateShortcut "$DESKTOP\udStream.lnk" "$INSTDIR\udStream${BUILD_SUFFIX}.exe"
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCore ${LANG_ENGLISH} "Core components of udStream."
  LangString DESC_SecHandlers ${LANG_ENGLISH} "Register file and protocol handlers for udStream."
  LangString DESC_SecStartMenu ${LANG_ENGLISH} "Create Start Menu item for udStream."
  LangString DESC_SecDesktop ${LANG_ENGLISH} "Create Desktop shortcut for udStream."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHandlers} $(DESC_SecHandlers)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} $(DESC_SecStartMenu)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;Delete files
  Delete "$INSTDIR\udStream${BUILD_SUFFIX}.exe"
  Delete "$INSTDIR\udStreamConvertCMD.exe"
  Delete "$INSTDIR\vaultSDK.dll"
  Delete "$INSTDIR\SDL2.dll"
  Delete "$INSTDIR\libfbxsdk.dll"
  Delete "$INSTDIR\releasenotes.md"
  Delete "$INSTDIR\defaultsettings.json"
  RMDir /r "$INSTDIR\assets"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES64\Euclideon"

  ;Clean up shortcuts
  Delete "$SMPROGRAMS\udStream.lnk"
  Delete "$DESKTOP\udStream.lnk"

  ;Clean up Registry
  !macro AssocDeleteFileExtAndProgId _hkey _dotext _pid
  ReadRegStr $R0 ${_hkey} "Software\Classes\${_dotext}" ""
  StrCmp $R0 "${_pid}" 0 +2
    DeleteRegKey ${_hkey} "Software\Classes\${_dotext}"

  DeleteRegKey ${_hkey} "Software\Classes\${_pid}"
  !macroend

  !macro DeleteRegKeyIfEmpty _hkey _registryKey
  ;Delete Registry Key if there are no more values
  ClearErrors
  EnumRegValue $R0 ${_hkey} "${_registryKey}" 0
  StrCmp $R0 "" +1 +2
    DeleteRegKey /ifempty ${_hkey} "${_registryKey}"
  !macroend

  ;Delete file handlers
  !insertmacro AssocDeleteFileExtAndProgId HKLM ".uds" "udStream.Dataset"
  !insertmacro AssocDeleteFileExtAndProgId HKLM ".udg" "udStream.Dataset"
  !insertmacro AssocDeleteFileExtAndProgId HKLM ".ssf" "udStream.Dataset"
  !insertmacro AssocDeleteFileExtAndProgId HKLM ".udp" "udStream.Project"
  DeleteRegValue HKCR ".json\OpenWithProgids" "udStream.Project"
  !insertmacro DeleteRegKeyIfEmpty HKCR ".json\OpenWithProgids"
  DeleteRegKey /ifempty HKCR ".json"
  
  ;Delete protocol handler
  DeleteRegKey HKCR "euclideon"

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p0, p0)'

  DeleteRegKey HKLM "${REG_UNINST_KEY}"
  DeleteRegKey HKCU "Software\Euclideon\udStream"
  DeleteRegKey /ifempty HKCU "Software\Euclideon"

SectionEnd