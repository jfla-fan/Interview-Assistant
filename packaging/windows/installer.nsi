; -----------------------------------------------------------------------------
; Script for Interview Assistant Installer
; -----------------------------------------------------------------------------

; --- Basic Information ---
!define APP_NAME "Interview Assistant"
!define APP_ICON "app_icon.ico"
!define COMPANY_NAME "Edward"
!define EXE_NAME "InterviewAssistant.exe"
!define UNINSTALLER_NAME "uninstall.exe"

!ifndef APP_VERSION
  !error "Specify app version"
!endif

!ifndef STAGING_DIR
  !error "STAGING_DIR must be defined"
!endif

!ifndef OUTPUT_DIR
  !define OUTPUT_DIR "."
!endif

; --- Installation Name ---
Name "${APP_NAME}"

; --- Text Below Any Page Content ---
BrandingText "${COMPANY_NAME}"

; --- Installer Path ---
OutFile "${OUTPUT_DIR}\IA_Setup_${APP_VERSION}.exe"

; --- Default Installation Directory ---
InstallDir "$PROGRAMFILES64\${APP_NAME}"

; --- Request admin privileges ---
RequestExecutionLevel admin


; =============================================================================
; MODERN UI 2 CONFIGURATION
; =============================================================================

!include "x64.nsh"

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

; --- General UI Settings ---
!define MUI_ABORTWARNING
!define MUI_ICON "${APP_ICON}"
!define MUI_UNICON "${APP_ICON}"

; --- Installer Pages ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; --- Custom Shortcuts Page ---
Var g_StartMenuCheckbox      ; temporary: control handle
Var g_DesktopCheckbox        ; temporary: control handle
Var g_CreateStartMenu        ; persistent: 1 = checked, 0 = unchecked
Var g_CreateDesktop          ; persistent: 1 = checked, 0 = unchecked

Page custom CreateShortcutsPage CreateShortcutsPageLeave "Shortcuts"

!insertmacro MUI_PAGE_FINISH

; --- Uninstaller Pages ---
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --- Language ---
!insertmacro MUI_LANGUAGE "English"


; =============================================================================
; INITIALIZATION
; =============================================================================

Function .onInit
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "This application requires a 64-bit version of Windows."
        Quit
    ${EndIf}
FunctionEnd


; =============================================================================
; CUSTOM SHORTCUTS PAGE
; =============================================================================

Function CreateShortcutsPage
    !insertmacro MUI_HEADER_TEXT "Create Shortcuts" "Choose where to create shortcuts."

    nsDialogs::Create 1018
    Pop $0

    ${NSD_CreateCheckbox} 0 13u 100% 12u "Create a Start Menu shortcut"
    Pop $g_StartMenuCheckbox
    ${NSD_SetState} $g_StartMenuCheckbox ${BST_CHECKED}

    ${NSD_CreateCheckbox} 0 30u 100% 12u "Create a Desktop shortcut"
    Pop $g_DesktopCheckbox
    ${NSD_SetState} $g_DesktopCheckbox ${BST_CHECKED}

    nsDialogs::Show
FunctionEnd

Function CreateShortcutsPageLeave
    ${NSD_GetState} $g_StartMenuCheckbox $0
    StrCpy $g_CreateStartMenu $0

    ${NSD_GetState} $g_DesktopCheckbox $0
    StrCpy $g_CreateDesktop $0
FunctionEnd


; =============================================================================
; INSTALL SECTIONS
; =============================================================================

Section "Interview Assistant (Required)" SEC_CORE
    SectionIn RO

    SetOutPath $INSTDIR
    File /r "${STAGING_DIR}\*.*"

    WriteUninstaller "$INSTDIR\${UNINSTALLER_NAME}"

    ; --- Add/Remove Programs entry (per-user: use HKCU) ---
    WriteRegStr HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${COMPANY_NAME}"
    WriteRegStr HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${EXE_NAME}"
    WriteRegStr HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\${UNINSTALLER_NAME}"'
    WriteRegDWORD HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
SectionEnd


; =============================================================================
; POST-INSTALL: CREATE SHORTCUTS
; =============================================================================

Function .onInstSuccess
    ; Shortcuts for all users
    SetShellVarContext all

    ; --- Start Menu ---
    ${If} $g_CreateStartMenu == ${BST_CHECKED}
        CreateDirectory "$SMPROGRAMS\${APP_NAME}"
        CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
        CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\${UNINSTALLER_NAME}"
    ${EndIf}

    ; --- Desktop ---
    ${If} $g_CreateDesktop == ${BST_CHECKED}
        CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
    ${EndIf}
FunctionEnd


; =============================================================================
; UNINSTALLER
; =============================================================================

Section "Uninstall"
    SetOutPath "$TEMP"

    Delete "$INSTDIR\${EXE_NAME}"
    Delete "$INSTDIR\${UNINSTALLER_NAME}"
    RMDir /r "$INSTDIR"

    ; Remove shortcuts
    SetShellVarContext all

    Delete "$SMPROGRAMS\${APP_NAME}\*.*"
    RMDir "$SMPROGRAMS\${APP_NAME}"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    SetShellVarContext current

    ; Remove registry
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
