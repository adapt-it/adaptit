; Script name: Adapt It Unicode - Documentation Only.iss
; Author: Bill Martin, Erik Brommers
; Purpose: This script creates an Adapt It installer that installs ONLY the Documentation that is 
;   left out when a user previously installs the "Miminal" installation. The documentation only 
;   installer has no version number in its name, but is named 'Adapt_It_WX_Unicode_Documentation_Only.exe'.
;   This script draws its files from the 'adaptit/setup Unicode Documentation Only' staging folder of 
;   the developer's adaptit working tree. The execution of this Adapt It Unicode - Documentation Only.iss 
;   script is the last stage in the creation documentation installer for the Windows version of Adapt It 
;   (See "Prerequisites" below.)
;   The installer produced by this script Adapt_It_WX_Unicode_Documentation_Only.exe can be uploaded 
;   from the developer's adaptit/AIWXInstallers folder to the adapt-it.org website using WinSCP.
; Prerequisites: IMPORTANT: BEFORE RUNNING/EXECUTING THIS SCRIPT YOU SHOULD HAVE DONE THE FOLLOWING Steps:
;   1. Complete all documentation related updates for a new release of the Adapt It application. Documents
;      that should be updated are minimally: Adapt It changes.txt, Readme_Unicode_Version.txt, 
;      Known Issues and Limitations.txt, Adapt It Reference.odt, Adapt It Reference.doc. Other documents
;      that might periodically be updated are: Adapt It Tutorial.doc, Adapt_It_Quick_Start.htm,
;      GuesserExplanation.htm, Help_for_Administrators.odt, Help_for_Administrators.htm, SIL Converters
;      in AdaptIt.doc, and possibly others.
;   2. Execute the batch file named 'CallAllBatchSetups.bat' that is located in the top-level
;      adaptit folder of the developer's tree. That batch file populates the various staging
;      folders with the updated files from other folders in the tree, in preparation for the
;      compiling/running of this 'Adapt It Unicode - Documentation Only.iss' Inno Setup script.
;      Note: This installer installs only documentation files. It does not install Git nor the 
;      main Adapt It program/executable.
;   3. Finally, once the above pre-requisites are met, compile this script using the Inno Setup Compiler. 
;      It should compile without error (if not contact Bill Martin!). The installer that this script
;      creates is stored in the 'adaptit/AIWX Installers/' folder where it and the other installers
;      created for an Adapt It release can be uploaded to https://adapt-it.org using WinSCP.
; Source/Credits: 
; This ccript was initially generated by the Inno Setup Script Wizard and informed by
; the example scripts supplied with an installation of Inno Setup.
; See the Inno Setup documentation for details on creating Inoo Setup script files!

; IMPORTANT TODO: designate the version numbers in the following header file within the
; adaptit/source folder. The version numbers in this header file are the only location
; where all the Inno Setup scripts get their version numbers in the form of:
;   #define AI_VERSION_MAJOR 6
;   #define AI_VERSION_MINOR 10
;   #define AI_REVISION 4
; and
;   #define GIT_VERSION_MAJOR 2
;   #define GIT_VERSION_MINOR 32
;   #define GIT_REVISION 0
#include "../source/_AIandGitVersionNumbers.h"

#define AIAppName "Adapt It WX Unicode"
#define AIAppVersion Str(AI_VERSION_MAJOR) + "." +Str(AI_VERSION_MINOR) + "." + Str(AI_REVISION) ; 6.10.3
#define AIAppUnderscoreVersion Str(AI_VERSION_MAJOR) + "_" +Str(AI_VERSION_MINOR) + "_" + Str(AI_REVISION) ; 6_10_3
#define AIInstallerCopyright "2021 by Bruce Waters, Bill Martin, SIL International"
#define AIInstallerPublisher "Adapt It"
#define AIAppURL "https://www.adapt-it.org/"
#define AIAppFullName "Adapt_It_Unicode.exe"
#define AIAppShortName "Adapt It"
#define RepoBase ".."

; RepoStagingFolder differs in the 5 app installers
#define RepoStagingFolder "setup Unicode Documentation Only"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppID={{7317EA81-BC6E-4A4F-AE2B-44ADE6A2188F}
AppName={#AIAppName}
AppVersion={#AIAppVersion}
AppVerName={#AIAppName} {#AIAppVersion}
AppPublisher={#AIInstallerPublisher}
AppPublisherURL={#AIAppURL}
AppSupportURL={#AIAppURL}
AppUpdatesURL={#AIAppURL}
DefaultDirName={commonpf}\{#AIAppName}
DefaultGroupName={#AIAppName}
LicenseFile={#RepoBase}\{#RepoStagingFolder}\LICENSING.txt
InfoBeforeFile={#RepoBase}\{#RepoStagingFolder}\Readme_Unicode_Version.txt

; OutputBaseFilename differs in the 5 app installers
OutputBaseFilename="Adapt_It_WX_Unicode_Documentation_Only" 

SetupIconFile={#RepoBase}\res\ai_32.ico
Compression=lzma/Max
SolidCompression=true
OutputDir={#RepoBase}\AIWX Installers
VersionInfoCopyright={#AIInstallerCopyright}
VersionInfoProductName={#AIAppName}
VersionInfoProductVersion={#AIAppVersion}
WizardImageFile="{#RepoBase}\res\ai_wiz_bg.bmp"
WizardSmallImageFile="{#RepoBase}\res\AILogo32x32.bmp"
WizardImageStretch=false
AppCopyright={#AIInstallerCopyright}
PrivilegesRequired=admin
DirExistsWarning=no
VersionInfoVersion={#AIAppVersion}
VersionInfoCompany=SIL
VersionInfoDescription={#AIAppName}
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
; Note: The files included in this "full" installer are the complete list for the "full" installer.
; If additions or changes are made here you need to make corresponding changes to the appropriate
; batch file(s):
;   _CopyDocs2InstallFolders.bat in docs folder
;   _CopyHelp2InstallFolders.bat in hlp folder
;   _CopyMO2InstallFolders.bat in the po folder
;   _CopyXML2InstallFolders.bat in the xml folder
; The following are the individual files (uncommented) that are included within this "Documentation Only" installer:
;Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt_It_Unicode.exe"; DestDir: "{app}"; Flags: ignoreversion; 
;Source: "{#RepoBase}\setup Git\{#GitDownloaderFullName}"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt It changes.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt_It_Quick_Start.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Help_for_Administrators.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\RFC5646message.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\GuesserExplanation.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt It Reference.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt It Tutorial.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Adapt_It_Unicode.htb"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\AI_UserProfiles.xml"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\AI_USFM.xml"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\books.xml"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\curl-ca-bundle.crt"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\iso639-3codes.txt"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#RepoBase}\{#RepoStagingFolder}\aiDefault.css"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\KJV 1Jn 2.12-17.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Known Issues and Limitations.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\LICENSING.txt"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\License_CPLv05.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\License_GPLv2.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\License_LGPLv21.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Localization_Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Readme_Unicode_Version.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#RepoBase}\{#RepoStagingFolder}\SILConverters in AdaptIt.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Tok Pisin fragment 1John.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoBase}\{#RepoStagingFolder}\Images\Admin_help\AdminMenuSetupBECollab.gif"; DestDir: "{app}\Images\Admin_help\"; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\Images\Admin_help\AdminMenuSetupPTCollab.gif"; DestDir: "{app}\Images\Admin_help\"; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\Images\Admin_help\*.gif"; DestDir: {app}\Images\Admin_help\; Flags: IgnoreVersion; 
Source: "{#RepoBase}\{#RepoStagingFolder}\Images\Adapt_It_Quick_Start\*.gif"; DestDir: "{app}\Images\Adapt_It_Quick_Start\"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\az\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\az"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\az\az.po"; DestDir: "{app}\Languages\az"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\es\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\es\es.po"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\es\wxstd.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\fr\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\fr\fr.po"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\fr\wxstd.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\id\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\id\id.po"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\pt\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\pt\pt.po"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\pt\wxstd.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\ru\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\ru\ru.po"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\ru\wxstd.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\swh\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\swh"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\swh\swh.po"; DestDir: "{app}\Languages\swh"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\tpi\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\tpi\tpi.po"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\tpi\books_tpi.xml"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\tpi\tpi_readme.txt"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\tpi\wxstd.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\zh\Adapt_It_Unicode.mo"; DestDir: "{app}\Languages\zh"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\zh\zh.po"; DestDir: "{app}\Languages\zh"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\Languages\zh\wxstd.mo"; DestDir: "{app}\Languages\zh"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\Ansi2Utf8.exe"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CC.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CC.hlp"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\cc32.dll"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CCDebug.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CCFiles.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CCW32.exe"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\CCW32.INI"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\reverse_lx_ge.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\FwdSlashInsertAtPuncts.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\FwdSlashRemoveAtPuncts.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\Summary.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
;Source: "{#RepoBase}\{#RepoStagingFolder}\CC\table series as one.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,{#AIAppShortName}}"; Filename: {#AIAppURL}; Comment: "Go to the Adapt It website at https://adapt-it.org"; 
Name: "{group}\Adapt It Quick Start"; Filename: "{app}\Adapt_It_Quick_Start.htm"; WorkingDir: "{app}"; Comment: "Launch Adapt It Quick Start in browser"; 
Name: "{group}\Help for Administrators (HTML)"; Filename: "{app}\Help_for_Administrators.htm"; WorkingDir: "{app}"; Comment: "Launch Help for Administrators"; 
Name: "{group}\Adapt It Tutorial"; Filename: "{app}\Adapt It Tutorial.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Tutorial.doc in word processor"; 
Name: "{group}\Adapt It Reference"; Filename: "{app}\Adapt It Reference.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Reference.doc in word processor"; 
Name: "{group}\Adapt It Changes"; Filename: "{app}\Adapt It changes.txt"; WorkingDir: "{app}"; Comment: "Launch Adapt It changes.txt in Notepad"; 

; There is no need for a [Code] section in this script, since the installer it produces only installs documentation 
; files.