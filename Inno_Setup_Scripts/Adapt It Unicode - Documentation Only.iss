; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Adapt It WX Unicode"
#define MyAppVersion "6.10.1"
#define MyAppPublisher "Adapt It"
#define MyAppURL "http://www.adapt-it.org/"
#define MyAppExeName "Adapt_It_Unicode.exe"
#define MyAppShortName "Adapt It"
#define SvnBase ".."

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppID={{7317EA81-BC6E-4A4F-AE2B-44ADE6A2188F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\Adapt It WX Unicode
DefaultGroupName=Adapt It WX Unicode
LicenseFile={#SvnBase}\setup Unicode Documentation Only\LICENSING.txt
InfoBeforeFile={#SvnBase}\setup Unicode Documentation Only\Readme_Unicode_Version.txt
OutputBaseFilename=Adapt_It_WX_Unicode_Documentation_Only
SetupIconFile={#SvnBase}\res\ai_32.ico
Compression=lzma/Max
SolidCompression=true
OutputDir={#SvnBase}\AIWX Installers
VersionInfoCopyright=2020 by Bruce Waters, Bill Martin, SIL International
VersionInfoProductName=Adapt It WX Unicode
VersionInfoProductVersion=6.10.1
WizardImageFile="{#SvnBase}\res\ai_wiz_bg.bmp"
WizardSmallImageFile="{#SvnBase}\res\AILogo32x32.bmp"
WizardImageStretch=false
AppCopyright=2020 Bruce Waters, Bill Martin, SIL International
PrivilegesRequired=poweruser
DirExistsWarning=no
VersionInfoVersion=6.10.1
VersionInfoCompany=SIL
VersionInfoDescription=Adapt It WX Unicode
UsePreviousGroup=false
UsePreviousAppDir=false
DisableWelcomePage=true

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]

[Files]
Source: "{#SvnBase}\setup Unicode Documentation Only\Adapt It changes.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Adapt_It_Quick_Start.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Help_for_Administrators.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Unicode Documentation Only\RFC5646message.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Unicode Documentation Only\GuesserExplanation.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Unicode Documentation Only\Adapt It Reference.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Adapt It Tutorial.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Adapt_It_Unicode.htb"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\KJV 1Jn 2.12-17.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Known Issues and Limitations.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\LICENSING.txt"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#SvnBase}\setup Unicode Documentation Only\License_CPLv05.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\License_GPLv2.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\License_LGPLv21.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Localization_Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Readme_Unicode_Version.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#SvnBase}\setup Unicode Documentation Only\SILConverters in AdaptIt.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Tok Pisin fragment 1John.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Unicode Documentation Only\Images\Admin_help\*.gif"; DestDir: "{app}\Images\Admin_help\"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Unicode Documentation Only\Images\Adapt_It_Quick_Start\*.gif"; DestDir: "{app}\Images\Adapt_It_Quick_Start\"; Flags: IgnoreVersion; 

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppShortName}}"; Filename: {#MyAppURL}; Comment: "Go to the Adapt It website at http://adapt-it.org"; 
Name: "{group}\Adapt It Quick Start"; Filename: "{app}\Adapt_It_Quick_Start.htm"; WorkingDir: "{app}"; Comment: "Launch Adapt It Quick Start in browser"; 
Name: "{group}\Help for Administrators (HTML)"; Filename: "{app}\Help_for_Administrators.htm"; WorkingDir: "{app}"; Comment: "Launch Help for Administrators"; 
Name: "{group}\Adapt It Tutorial"; Filename: "{app}\Adapt It Tutorial.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Tutorial.doc in word processor"; 
Name: "{group}\Adapt It Reference"; Filename: "{app}\Adapt It Reference.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Reference.doc in word processor"; 
Name: "{group}\Adapt It Changes"; Filename: "{app}\Adapt It changes.txt"; WorkingDir: "{app}"; Comment: "Launch Adapt It changes.txt in Notepad"; 

[Run]
