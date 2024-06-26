; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

; edb 11 Oct 2013: Include InnoTools Downloader for installing Git
; (this is the local version in the Inno_Setup_Scripts directory). This needs
; the following:
; - itdownload.dll    // DLL that allows us to download 3rd party apps
; - it_download.iss   // ITD script to connect the DLL
#include "it_download.iss"

#define MyAppName "Adapt It WX"
#define MyAppVersion "6.6.1"
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
LicenseFile={#SvnBase}\setup Regular\LICENSING.txt
InfoBeforeFile={#SvnBase}\setup Regular\Readme.txt
OutputBaseFilename=Adapt_It_WX_6_6_1_Regular
SetupIconFile={#SvnBase}\res\ai_32.ico
Compression=lzma/Max
SolidCompression=true
OutputDir={#SvnBase}\AIWX Installers
VersionInfoCopyright=2015 by Bruce Waters, Bill Martin, SIL International
VersionInfoProductName=Adapt It WX
VersionInfoProductVersion=6.6.1
WizardImageFile="{#SvnBase}\res\ai_wiz_bg.bmp"
WizardSmallImageFile="{#SvnBase}\res\AILogo32x32.bmp"
WizardImageStretch=false
AppCopyright=2015 Bruce Waters, Bill Martin, SIL International
PrivilegesRequired=poweruser
DirExistsWarning=no
VersionInfoVersion=6.6.1
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
Source: "{#SvnBase}\setup Regular\Adapt_It.exe"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#SvnBase}\setup Regular\Adapt It changes.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Adapt_It_Quick_Start.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Help_for_Administrators.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\RFC5646message.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\GuesserExplanation.htm"; DestDir: "{app}"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Adapt It Reference.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Adapt It Tutorial.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Adapt_It.htb"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\AI_UserProfiles.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\AI_USFM.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\books.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\curl-ca-bundle.crt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\iso639-3codes.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\aiDefault.css"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\KJV 1Jn 2.12-17.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Known Issues and Limitations.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\LICENSING.txt"; DestDir: "{app}"; Flags: ignoreversion; 
Source: "{#SvnBase}\setup Regular\License_CPLv05.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\License_GPLv2.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\License_LGPLv21.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Localization_Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\rdwrtp7.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\ParatextShared.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular\ICSharpCode.SharpZipLib.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular\Interop.XceedZipLib.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular\NetLoc.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular\Utilities.dll"; DestDir: "{app}"; Flags: IgnoreVersion
Source: "{#SvnBase}\setup Regular\Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#SvnBase}\setup Regular\SILConverters in AdaptIt.doc"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Tok Pisin fragment 1John.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SvnBase}\setup Regular\Images\Admin_help\*.gif"; DestDir: "{app}\Images\Admin_help\"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Images\Adapt_It_Quick_Start\*.gif"; DestDir: "{app}\Images\Adapt_It_Quick_Start\"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\es\Adapt_It.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\es\es.po"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\es\wxstd.mo"; DestDir: "{app}\Languages\es"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\fr\Adapt_It.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\fr\fr.po"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\fr\wxstd.mo"; DestDir: "{app}\Languages\fr"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\id\Adapt_It.mo"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\id\id.po"; DestDir: "{app}\Languages\id"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\pt\Adapt_It.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\pt\pt.po"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\pt\wxstd.mo"; DestDir: "{app}\Languages\pt"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\ru\Adapt_It.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\ru\ru.po"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\ru\wxstd.mo"; DestDir: "{app}\Languages\ru"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\tpi\Adapt_It.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\tpi\tpi.po"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\tpi\books_tpi.xml"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\tpi\tpi_readme.txt"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\Languages\tpi\wxstd.mo"; DestDir: "{app}\Languages\tpi"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\Ansi2Utf8.exe"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CC.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CC.hlp"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\cc32.dll"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CCDebug.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CCFiles.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CCW32.exe"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\CCW32.INI"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\reverse_lx_ge.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\FwdSlashInsertAtPuncts.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\FwdSlashRemoveAtPuncts.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\Summary.doc"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 
Source: "{#SvnBase}\setup Regular\CC\table series as one.cct"; DestDir: "{app}\CC"; Flags: IgnoreVersion; 

[Registry]
Root: HKCU; Subkey: "Environment"; ValueName: "Path"; ValueType: "string"; ValueData: "{pf}\Git\bin;\{pf}\Git\cmd;{olddata}"; Check: NotOnPathAlready(); Flags: preservestringtype;
;Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{pf}\Git\bin;{pf}\Git\cmd;{olddata}"; Check: NotOnPathAlready(); Flags: preservestringtype;

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; WorkingDir: {app}; Comment: "Launch Adapt It"; 
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppShortName}}"; Filename: {#MyAppURL}; Comment: "Go to the Adapt It website at http://adapt-it.org"; 
Name: "{group}\Adapt It Quick Start"; Filename: "{app}\Adapt_It_Quick_Start.htm"; WorkingDir: "{app}"; Comment: "Launch Adapt It Quick Start in browser"; 
Name: "{group}\Help for Administrators (HTML)"; Filename: "{app}\Help_for_Administrators.htm"; WorkingDir: "{app}"; Comment: "Launch Help for Administrators"; 
Name: "{group}\Adapt It Tutorial"; Filename: "{app}\Adapt It Tutorial.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Tutorial.doc in word processor"; 
Name: "{group}\Adapt It Reference"; Filename: "{app}\Adapt It Reference.doc"; WorkingDir: "{app}"; Comment: "Launch Adapt It Reference.doc in word processor"; 
Name: "{group}\Adapt It Changes"; Filename: "{app}\Adapt It changes.txt"; WorkingDir: "{app}"; Comment: "Launch Adapt It changes.txt in Notepad"; 
Name: {group}\Uninstall; Filename: {uninstallexe}; WorkingDir: {app}; Comment: "Uninstall Adapt It from this computer"; 
Name: {commondesktop}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon; 
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Adapt It"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\CC\Consistent Changes for Windows"; Filename: "{app}\CC\CCW32.exe"; WorkingDir: "{app}\CC"; IconFilename: "{app}\CC\CCW32.exe"; Comment: "Launch Consistent Changes GUI program"; 
Name: "{group}\Consistent Changes (standalone)"; Filename: "{app}\CC\CCW32.exe"; WorkingDir: "{app}\CC"; IconFilename: "{app}\CC\CCW32.exe"; Comment: "Launch Consistent Changes GUI program"; 
Name: "{group}\CC\CC Summary Document"; Filename: {app}\CC\Summary.doc; WorkingDir: {app}\CC; 
Name: "{group}\CC\Consistent Changes Documentation"; Filename: {app}\CC\CC.doc; WorkingDir: {app}\CC; 
Name: "{group}\CC\CC Files Document"; Filename: {app}\CC\CCFiles.doc; WorkingDir: {app}\CC; 
Name: "{group}\CC\CC Debug Document"; Filename: {app}\CC\CCDebug.doc; WorkingDir: {app}\CC; 

; edb 11 Oct 2013: Code changes to download / install Git
[Code]
const GitSetupURL = 'http://msysgit.googlecode.com/files/Git-1.8.4-preview20130916.exe';
var GitInstalled: Boolean;  // Is Git installed?
var ShouldInstallGit: Boolean; // should the installer download and run the Git installer?
var tmpResult: Integer;     
var GitName: string;
var msg: string;

procedure InitializeWizard();
begin
    GitInstalled := False;
    ShouldInstallGit := False;
    GitName := expandconstant('{tmp}\GitInstaller.exe');
    ITD_Init; // initialize the InnoTools Downloader
    itd_downloadafter(wpReady);

    // Test for Git by looking for its uninstaller in the registry
    // check for 64-bit Windows
    if (RegKeyExists(HKLM, 'SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1')) then
        GitInstalled := True;
    // check for 32-bit Windows
    if (RegKeyExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Git_is1')) then
        GitInstalled := True;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID=wpReady then begin //Lets install those files that were downloaded for us
    if (GitInstalled = False) then
      if (MsgBox('Adapt It needs to install a program called Git in order to track changes in your translated documents. Would you like to download and install this program?', mbConfirmation, MB_YESNO) = IDYES) then
        // user clicked YES -- install Git from the internet.
        begin
          // download the Git installer after the "ready to install" screen is shown
          ShouldInstallGit := True;
          ITD_AddFile(GitSetupURL, GitName);
        end
      else begin
        msg := 'You chose not to download and install the Git program as part of the Adapt It installation. Adapt It will still run, but it will not be able to track changes in your translated documents (nor be able to restore a previous version) until you install the Git program on this computer. If you have not previously downloaded the Git installer (15MB) and you have Internet access, the recommended way to obtain the Git program is to run the Adapt It installer again and choose to have it automatically download and install the Git program with the correct settings.' + Chr(13) + Chr(13);
        msg := msg + 'If you have no Internet access (or it is too slow/expensive) at the time you wish to Install Adapt It, the Git installer can be downloaded separately (from: http://git-scm.com/downloads) and installed at a later time after this installer has finished.';
        MsgBox(msg, mbInformation, MB_OK);
      end
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep=ssPostInstall then
    // Silently run the Git installer as a post-install step
    if (ShouldInstallGit = True) then begin
      // run the git installer silently, with the options loaded from the file
      // ai_git.inf. The git installer for Windows is made in Inno as well;
      // more info on the command line options for Inno installers can be
      // found here: http://www.jrsoftware.org/ishelp/index.php?topic=setupcmdline
      Exec(GitName, '/SILENT', '', SW_SHOW, ewWaitUntilTerminated, tmpResult);
    end;
end;

function NotOnPathAlready(): Boolean;
var
  BinDir, Path: String;
begin
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', Path) then
  begin // Successfully read the value
    Log('HKCU\Environment\PATH = ' + Path);
    BinDir := ExpandConstant('{pf}\Git\bin');
    Log('Looking for Git\bin dir in %PATH%: ' + BinDir + ' in ' + Path);
    if Pos(LowerCase(BinDir), Lowercase(Path)) = 0 then
    begin
      Log('Did not find Git\bin dir in %PATH% so will add it');
      Result := True;
    end
    else
    begin
      Log('Found Git\bin dir in %PATH% so will not add it again');
      Result := False;
    end
  end
  else // The key probably doesn't exist
  begin
    Log('Could not access HKCU\Environment\PATH so assume it is ok to add it');
    Result := True;
  end;
end;

function NeedRestart(): Boolean;
begin
  // If Git was installed, prompt the user to reboot after everything's done
  Result := ShouldInstallGit;
end;
// end EDB Oct 2013

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, "&", "&&")}}"; Flags: nowait postinstall skipifsilent
