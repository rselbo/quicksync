;Product Info
Name "Quicksync" ;Define your own software name here
!define PRODUCT "Quicksync" ;Define your own software name here
!define VERSION "1.0" ;Define your own software version here

CRCCheck On
; Script create for version 2.0rc1/final (from 12.jan.04) with GUI NSIS (c) by Dirk Paehl. Thank you for use my program

 !include "MUI.nsh"

 
;--------------------------------
;Configuration
 
   OutFile "setup.exe"

  ;Folder selection page
   InstallDir "$PROGRAMFILES\${PRODUCT}"

;Remember install folder
InstallDirRegKey HKCU "Software\${PRODUCT}" ""

;--------------------------------
;Pages
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_TEXT "Start quicksync"
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchQuicksync"
  !insertmacro MUI_PAGE_FINISH

 !define MUI_ABORTWARNING

 
;--------------------------------
 ;Language
 
  !insertmacro MUI_LANGUAGE "English"
;--------------------------------

     
Section "section_1" section_1
SetOutPath "$INSTDIR"
FILE "..\pcre\lib\pcre3.dll"
FILE "QtCore4.dll"
FILE "QtGui4.dll"
FILE "QtNetwork4.dll"
FILE "..\client\Release\syncclient.exe"
WriteRegStr HKCU "Software\jesperhansen\syncclient\patcher\" "usepatcher" "false"
SectionEnd

Section Shortcuts
CreateDirectory "$SMPROGRAMS\Quicksync"
CreateShortCut "$SMPROGRAMS\Quicksync\Quicksync.lnk" $INSTDIR\syncclient.exe"
SectionEnd

Section Uninstaller
  CreateShortCut "$SMPROGRAMS\Quicksync\Uninstall.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync" "DisplayName" "${PRODUCT} ${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync" "URLInfoAbout" ""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync" "Publisher" "Roar Selbo"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync" "UninstallString" "$INSTDIR\Uninst.exe"
  WriteRegStr HKCU "Software\${PRODUCT}" "" $INSTDIR
  WriteUninstaller "$INSTDIR\Uninst.exe"
 
 
SectionEnd
 
 
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer.."
FunctionEnd
  
Function un.onInit 
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Function LaunchQuicksync
  ExecShell "" "$SMPROGRAMS\Quicksync\Quicksync.lnk"
FunctionEnd
 
Section "Uninstall" 
 
  Delete "$INSTDIR\*.*" 
   
  Delete "$SMPROGRAMS\Quicksync\*.*"
  RmDir "$SMPROGRAMS\Quicksync"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Quicksync"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Quicksync"
  RMDir "$INSTDIR"
             
SectionEnd
               
   
;eof
