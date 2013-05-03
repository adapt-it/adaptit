; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Adapt It WX"
#define MyAppVersion "6.4.2"
#define MyAppURL "http://www.adapt-it.org/"
#define MyAppExeName "Adapt_It.exe"
#define MyAppShortName "Adapt It"
#define SvnBase ".."

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppID={{FEC3801A-14B8-48A6-9749-AAB769CFE01D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\Adapt It WX
DefaultGroupName=Adapt It WX
LicenseFile={#SvnBase}\setup Regular - Minimal\LICENSING.txt
InfoBeforeFile={#SvnBase}\setup Regular - Minimal\Readme.txt
OutputBaseFilename=Adapt_It_WX_6_4_2_Regular_Minimal
SetupIconFile={#SvnBase}\res\ai_32.ico
Compression=lzma/Max
SolidCompression=true
OutputDir={#SvnBase}\AIWX Installers
VersionInfoCopyright=2013 by Bruce Waters, Bill Martin, SIL International
VersionInfoProductName=Adapt It WX
VersionInfoProductVersion=6.4.2
WizardImageFile="{#SvnBase}\res\ai_wiz_bg.bmp"
WizardSmallImageFile="{#SvnBase}\res\AILogo32x32.bmp"
WizardImageStretch=false
AppCopyright=2013 Bruce Waters, Bill Martin, SIL International
PrivilegesRequired=none
DirExistsWarning=no
VersionInfoVersion=6.4.2
VersionInfoCompany=SIL
VersionInfoDescription=Adapt It WX
UsePreviousGroup=false
UsePreviousAppDir=false
DisableWelcomePage=true
WizardImageBackColor=clWhite

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "{#SvnBase}\setup Regular - Minimal\Adapt_It.exe"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#SvnBase}\setup Regular - Minimal\AI_UserProfiles.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\AI_USFM.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\books.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\curl-ca-bundle.crt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\aiDefault.css"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\LICENSING.txt"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#SvnBase}\setup Regular - Minimal\License_CPLv05.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\License_GPLv2.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\License_LGPLv21.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\rdwrtp7.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular - Minimal\ParatextShared.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular - Minimal\ICSharpCode.SharpZipLib.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular - Minimal\Interop.XceedZipLib.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular - Minimal\NetLoc.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular - Minimal\Utilities.dll"; DestDir: "{app}"; Flags: IgnoreVersion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#SvnBase}\setup Regular - Minimal\Readme.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: {group}\_{#MyAppName}; Filename: {app}\{#MyAppExeName}; WorkingDir: {app}; Comment: "Launch Adapt It"; 
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppShortName}}"; Filename: {#MyAppURL}; Comment: "Go to the Adapt It website at http://adapt-it.org"; 
Name: {group}\Uninstall; Filename: {uninstallexe}; WorkingDir: {app}; Comment: "Uninstall Adapt It from this computer"; 
Name: {commondesktop}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon; 

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, "&", "&&")}}"; Flags: nowait postinstall skipifsilent
