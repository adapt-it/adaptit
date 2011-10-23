; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Adapt It WX"
#define MyAppVersion "6.0.1"
#define MyAppURL "http://www.adapt-it.org/"
#define MyAppExeName "Adapt_It.exe"
#define MyAppShortName "Adapt It"

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
LicenseFile=C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\LICENSING.txt
InfoBeforeFile=C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Readme.txt
OutputBaseFilename=Adapt_It_WX_Regular_Localizations_Only
SetupIconFile=C:\C++ Programming\Adapt It\adaptit\res\ai_32.ico
Compression=lzma/Max
SolidCompression=true
OutputDir=C:\Users\Bill Martin\Desktop\AIWX Installers
VersionInfoCopyright=2011 by Bruce Waters, Bill Martin, SIL International
VersionInfoProductName=Adapt It WX
VersionInfoProductVersion=6.0.1
WizardImageFile="C:\C++ Programming\Adapt It\adaptit\res\AIWX.bmp"
WizardSmallImageFile="C:\C++ Programming\Adapt It\adaptit\res\AILogo32x32.bmp"
WizardImageStretch=false
AppCopyright=2011 Bruce Waters, Bill Martin, SIL International
PrivilegesRequired=poweruser
DirExistsWarning=no
VersionInfoVersion=6.0.1
VersionInfoCompany=SIL
VersionInfoDescription=Adapt It WX
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
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Localization_Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\es\Adapt_It.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\es\es.po"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\es\wxstd.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\fr\Adapt_It.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\fr\fr.po"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\fr\wxstd.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\id\Adapt_It.mo"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\id\id.po"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\pt\Adapt_It.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\pt\pt.po"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\pt\wxstd.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\ru\Adapt_It.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\ru\ru.po"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\ru\wxstd.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\tpi\Adapt_It.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\tpi\tpi.po"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\tpi\books_tpi.xml"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\tpi\tpi_readme.txt"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "C:\C++ Programming\Adapt It\adaptit\setup Regular Localizations Only\Languages\tpi\wxstd.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 

[Icons]

[Run]
