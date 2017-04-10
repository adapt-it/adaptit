/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			InstallGitOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	24 March 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CInstallGitOptionsDlg class. 
/// The CInstallGitOptionsDlg class creates an AIModal dialog that allows the user to select from
/// 3 options - represented by 3 radio buttons that indicate how the Git program should be installed
/// by Adapt It.
/// \derivation		The CInstallGitOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "InstallGitOptionsDlg.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes

//#include <wx/msw/registry.h> // for wxRegKey

#include "Adapt_It.h"
#include "InstallGitOptionsDlg.h"
#include "BString.h"

// event handler table
BEGIN_EVENT_TABLE(CInstallGitOptionsDlg, AIModalDialog)
EVT_INIT_DIALOG(CInstallGitOptionsDlg::InitDialog)
EVT_BUTTON(wxID_OK, CInstallGitOptionsDlg::OnOK)
EVT_BUTTON(wxID_CANCEL, CInstallGitOptionsDlg::OnCancel)
EVT_RADIOBUTTON(ID_RADIOBUTTON_DO_NOT_INSTALL_GIT, CInstallGitOptionsDlg::OnRadioDoNotInstallGit)
EVT_RADIOBUTTON(ID_RADIOBUTTON_INSTALL_GIT_FROM_INTERNET, CInstallGitOptionsDlg::OnRadioDownloadGitAndInstall)
EVT_RADIOBUTTON(ID_RADIOBUTTON_BROWSE_FOR_GIT_INSTALLER, CInstallGitOptionsDlg::OnRadioBrowseForGitAndInstall)
END_EVENT_TABLE()

CInstallGitOptionsDlg::CInstallGitOptionsDlg(wxWindow* parent) // dialog constructor
    : AIModalDialog(parent, -1, _("Git Installation Options"),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and sizers
    // for the dialog. The first parameter is the parent which should normally be "this".
    // The second and third parameters should both be TRUE to utilize the sizers and create the right
    // size dialog.
    GitInstallOptionsDlgFunc(this, TRUE, TRUE);
    // The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    pRadioBtnDoNotInstallGitNow = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_DO_NOT_INSTALL_GIT);
    wxASSERT(pRadioBtnDoNotInstallGitNow != NULL);
    pRadioBtnDownloadAndInstallGitFromInternet = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_INSTALL_GIT_FROM_INTERNET);
    wxASSERT(pRadioBtnDownloadAndInstallGitFromInternet != NULL);
    pRadioBtnBrowseForGitInstaller = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_BROWSE_FOR_GIT_INSTALLER);
    wxASSERT(pRadioBtnBrowseForGitInstaller != NULL);
    pStaticTextTop = (wxStaticText*)FindWindowById(ID_TEXT_PREAMBLE);
    wxASSERT(pStaticTextTop != NULL);
    pStaticDescTopBtn = (wxStaticText*)FindWindowById(ID_TEXT_TOP_BTN_DESC);
    wxASSERT(pStaticDescTopBtn != NULL);

    // Set radio button defaults 
    pRadioBtnDoNotInstallGitNow->SetValue(TRUE);
    pRadioBtnDownloadAndInstallGitFromInternet->SetValue(FALSE);
    pRadioBtnBrowseForGitInstaller->SetValue(FALSE);

    m_pApp = &wxGetApp();
    // other attribute initializations
}

CInstallGitOptionsDlg::~CInstallGitOptionsDlg() // destructor
{

}

void CInstallGitOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
    //InitDialog() is not virtual, no call needed to a base class
    GitInstallerFileName = _T("Git-2.12.1-32-bit.exe");
    GitDownloadInstallerFileName = _T("Git_Downloader_2_12_1_4AI.exe");
    needsRestartMsg = _("The computer will need to restart before Git will be activated and the document histories can be managed. After closing this dialog, quit Adapt It, and then restart your computer. The next time you run Adapt It the document history items will work on the Adapt It File menu.");
    bGitInstalled = FALSE;
    GitSetupURL = _T("http://www.adapt-it.org/") + GitInstallerFileName;
    PathToAIInstallation = m_pApp->m_appInstallPathOnly;
    GitInstallerPathAndName = PathToAIInstallation + m_pApp->PathSeparator + GitInstallerFileName;
    GitDownloadInstallerPathAndName = PathToAIInstallation + m_pApp->PathSeparator + GitDownloadInstallerFileName;
    bGitInstallerExistsLocally = ::wxFileExists(GitInstallerPathAndName);

    if (bGitInstallerExistsLocally)
    {
        pRadioBtnDoNotInstallGitNow->SetValue(FALSE);
        pRadioBtnDownloadAndInstallGitFromInternet->SetValue(FALSE);
        pRadioBtnBrowseForGitInstaller->SetValue(TRUE);
    }
    else
    {
        // Make the initial selection be to Download and install Git from the Internet"
        pRadioBtnDoNotInstallGitNow->SetValue(FALSE);
        pRadioBtnDownloadAndInstallGitFromInternet->SetValue(TRUE);
        pRadioBtnBrowseForGitInstaller->SetValue(FALSE);
    }

    bGitInstalled = m_pApp->IsGitInstalled();
    if (bGitInstalled)
    {
        // Remove the top dialog text which doesn't apply when Git is already installed
        pStaticTextTop->SetLabel(_T(""));
        pStaticDescTopBtn->SetLabel(_T(""));
        // Make the initial selection be 'Do not try to install Git at this time"
        pRadioBtnDoNotInstallGitNow->SetValue(TRUE);
        pRadioBtnDownloadAndInstallGitFromInternet->SetValue(FALSE);
        pRadioBtnBrowseForGitInstaller->SetValue(FALSE);
    }
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CInstallGitOptionsDlg::OnOK(wxCommandEvent& event)
{
    if (pRadioBtnDoNotInstallGitNow->GetValue())
    {
        // If Git is not already installed provide information screen about Git and Adapt It
        if (!bGitInstalled)
        {
            // Make these statements context sensitive. The administrator may select the "Do not try
            // to install Git at this time" button even before s/he realizes that there is a local copy of 
            // the installer available. 
            msg = _("You chose not to download and install the Git program as part of the Adapt It installation. Adapt It will still run, but it will not be able to track changes in your translated documents (nor be able to restore a previous version) until you install the Git program on this computer. ");
            if (bGitInstallerExistsLocally)
            {
                // We remind the administrator/user that a local Git installer is available and we recommend they
                // install Git now while they are in the Adapt It installer.
                msg = msg + _("A Git installer is available on this computer at:\n     %s\nThat installer could be used to install Git without accessing the Internet. We recommend that you click on OK below, then click on the Back button and select the option \"Browse this computer to find a Git installer...\". The installer will help you install Git before you finish using this installer.\n\n");
                msg = msg.Format(msg, GitInstallerPathAndName.c_str());
            }
            else
            {
                // 
                msg = msg + _("If you have not previously downloaded the Git installer (36MB) and you have Internet access, the recommended way to obtain the Git program is to run the Adapt It installer again and choose to have it automatically download and install the Git program with the correct settings.\n\n");
            }
            // Remind the administrator/user that once Git has been downloaded once, that
            // installer will be preserved in the Adapt It installation folder where it could be
            // copied to a USB drive and made available when Adapt It and Git are installed on other
            // computers - making it possible to install Adapt It and Git on multiple computers once
            // the first installation has been done.
            msg = msg + _("You should know that the Adapt It installer now saves a copy of the Git installer the first time it is downloaded from the Internet. That copy of the Git installer is preserved in the Adapt It installation folder located at:\n     %s\nYou can use the local copy of the Git installer to install Git on this computer by running this installer again, or by transfering this installer and the Git installer via thumb drive to the other computer, and running this installer on that other computer.\n\n");
            msg = msg.Format(msg, PathToAIInstallation.c_str());

            // Tell the administrator that if they want to install Git apart from using this installer,
            // they can do so from a cmd terminal call, supplying the /SILENT option on the command line, 
            // which will cause it to be installed without any prompts - accepting all the default options 
            // required during the installation - as this installer does.
            msg = msg + _("The Git installer can be downloaded separately (from: http://git-scm.com/downloads) and installed at a later time apart from using this installer. It you decide to install Git that way, you should run the Git installer in a cmd window using this command:\n     %s /SILENT");
            msg = msg.Format(msg, GitInstallerFileName.c_str());
            wxMessageBox(msg, _("Git Installation Information"), wxICON_INFORMATION | wxOK);
        }
    }
    else if (pRadioBtnDownloadAndInstallGitFromInternet->GetValue())
    {
        // Note: Since Git_Downloader_2_12_1_4AI.exe is designed so that it can be used as a standalone
        // downloader/installer is handles checking to see if Git is already installed, and it will
        // issue the following Yes/No confirmation prompt if Git is already installed:
        // "The Git program is already installed. If Git is not working properly, you can download 
        // a fresh copy (36MB) and reinstall it. Would you like to download and reinstall Git?" [Yes] [No]
        // If the user responds 'Yes' the download and installation proceed. If the user responds 'No'
        // another message says:
        // The installer will now quit without trying to reinstall Git. The recommended way to install 
        // Git for use by Adapt It is by using this installer. If you prefer, the Git installer can be 
        // downloaded separately (from: http://git-scm.com/downloads) and installed at a later time 
        // after this installer has finished." [OK]

        // Call our small Git_Downloader_2_12_1_4AI.exe Git program to download and install the 
        // the actual Git installer. The local path of our small Git_Downloader_2_12_1_4AI.exe
        // should be stored in GitDownloadInstallerPathAndName.
        wxString commandLine;
        // Our small installer doesn't need any command-line arguments. It internally uses the 
        // "/SILENT" and "/NORESTART" options.
        // First it downloads the actual Git installer called "Git-2.12.1-32-bit.exe" saving it in the 
        // C:\Program Files (x86)\Adapt It WX Unicode\ folder, then it executed the downloaded Git installer
        // which has a nice progress bar, but suppresses the usual wizard pages. The Git installer is supposed
        // to disappear automatically when installation is finished but may present a "Completed" page with
        // "Finish" button for the user to click.
        commandLine = GitDownloadInstallerPathAndName; // typically: "C:\Program Files (x86)\Adapt It WX Unicode\Git_Downloader_2_12_1_4AI.exe"
        long result = -1;
        wxArrayString outputMsg;
        wxArrayString errorsMsg;
        // We use the wxExecute() override that takes the two wxStringArray parameters which
        // redirects the output and suppresses the dos console window during execution.
        result = ::wxExecute(commandLine, outputMsg, errorsMsg);
        result = result;
        // TODO: Error checking
        if (!bGitInstalled)
        {
            // Show message only if Adapt It detected that Git was already installed when this option
            // was selected
            // Since AI is always running when this CInstallGotOptionsDlg is invoked, we've suppressed
            // the "Needs Restart" wizard page notice in the installer in order to provide our own 
            // more appropriate message here that tells the user to first Quit AI before attempting a
            // system restart.
            //needsRestartMsg = 'The computer will need to restart before Git will be activated and the document histories can be managed. After closing this dialog, quit Adapt It, and then restart your computer. The next time you run Adapt It the document history items will work on the Adapt It File menu.';
            wxMessageBox(needsRestartMsg, _("Git Installation completed"), wxICON_INFORMATION | wxOK);
        }
    }
    else if (pRadioBtnBrowseForGitInstaller->GetValue())
    {
        bUseFileDialog = FALSE;
        // Check if a suitable Git installer exists in the AI installation folder. If so we 
        // can offer to use it since the user has chosen to use a previously downloaded copy 
        // of the Git installer.
        if (bGitInstallerExistsLocally)
        {
            int result;
            wxString msg = _("A Git installer is available on this computer at: %s.\nThat installer could be used to install Git without accessing the Internet. Do you want Adapt It to install Git now using that installer?");
            msg = msg.Format(msg, GitInstallerPathAndName.c_str());
            result = wxMessageBox(msg, _("Found a Git installer"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
            if (result == wxYES)
            {
                wxString commandLine;
                // use of "/SILENT" option suppresses the console output and just shows the Git installer GUI 
                // with a nice progress bar. The Git installer disappears automatically when installation is finished.
                // The use of the /NORESTART option will suppress the "Needs Restart" wizard page from appearing with
                // its "Restart Now or Later" options. Instead of allowing a sudden restart while Adapt It is running,
                // we'll just issue a message after the installer completes below, telling the user what to do now in 
                // order to get the document history functions to work.
                commandLine = GitInstallerPathAndName + _T(" ") + _T("/SILENT /NORESTART");
                long result = -1;
                wxArrayString outputMsg;
                wxArrayString errorsMsg;
                // We use the wxExecute() override that takes the two wxStringArray parameters which
                // redirects the output and suppresses the dos console window during execution.
                result = ::wxExecute(commandLine, outputMsg, errorsMsg);
                result = result;
                // TODO: Error checking
                if (!bGitInstalled)
                {
                    // Since AI is always running when this CInstallGotOptionsDlg is invoked, we've suppressed
                    // the "Needs Restart" wizard page notice in the installer in order to provide our own
                    // more appropriate message here that tells the user to first Quit AI before attempting a
                    // system restart.
                    //needsRestartMsg = _("The computer will need to restart before Git will be activated and the document histories can be managed. First close this message, then quit Adapt It, and restart your computer. The next time you run Adapt It the document history items will work on the Adapt It File menu.");
                    wxMessageBox(needsRestartMsg, _("Git Installation completed"), wxICON_INFORMATION | wxOK);
                }
            }
            else
            {
                bUseFileDialog = TRUE;
                // File dialog routine called below
            }
                             
        }
        else
        {
            // GitInstallerExistsLocally was False
            bUseFileDialog = TRUE;
        }
        if (bUseFileDialog)
        {
            // User opted to not use the existing Git installer that we found, perhaps because
            // the user wants to use a different (updated) one from a thumb drive, so present 
            // the file dialog and allow navigating to the desired installer.
            wxString Prompt = _("Select a Git installer file, for example Git-2.12.1-32-bit.exe [version 2.12.1 numbers may vary]");
            wxString FileName = _T("Git-*.exe");
            wxString InitialDirectory = PathToAIInstallation;
            wxString Filter = _("Git Installer (Git*.exe)|*.exe|All Files (*.*)|*.*||");
            wxFileDialog fileDlg(
                (wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
                Prompt,
                InitialDirectory,	// default dir
                FileName,		// default filename
                Filter,
                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            fileDlg.Centre();
            // open as modal dialog
            int returnValue = fileDlg.ShowModal(); // MFC has DoModal()
            if (returnValue == wxID_OK)
            {
                // Get the path and file
                GitInstallerPathAndName = fileDlg.GetPath();
                // Note: The user picked file could be anything, but hopefully with the help of the file dialog's
                // specified InitialDirectory, Filter, and the supplied FileName pattern above, the choice would 
                // be the normal Git installer, i.e., Git-2.12.1-32-bit.exe. If the user tries, it is possible for her/him to 
                // select our small installer named Git_Downloader_2_12_1_4AI.exe instead. That would still work, but
                // might lead to additional/unnecessary prompts to the user.
                
                wxString commandLine;
                // use of "/SILENT" option suppresses the console output and just shows the Git installer GUI 
                // with a nice progress bar. The Git installer disappears automatically when installation is finished.
                // The use of the /NORESTART option will suppress the "Needs Restart" wizard page from appearing with
                // its "Restart Now or Later" options. Instead of allowing a sudden restart while Adapt It is running,
                // we'll just issue a message after the installer completes below, telling the user what to do now in 
                // order to get the document history functions to work.
                commandLine = GitInstallerPathAndName + _T(" ") + _T("/SILENT /NORESTART");
                long result = -1;
                wxArrayString outputMsg;
                wxArrayString errorsMsg;
                // We use the wxExecute() override that takes the two wxStringArray parameters which
                // redirects the output and suppresses the dos console window during execution.
                result = ::wxExecute(commandLine, outputMsg, errorsMsg);
                result = result;
                // TODO: Error checking
                if (!bGitInstalled)
                {
                    // Since AI is always running when this CInstallGotOptionsDlg is invoked, we've suppressed
                    // the "Needs Restart" wizard page notice in the installer in order to provide our own
                    // more appropriate message here that tells the user to first Quit AI before attempting a
                    // system restart.
                    //needsRestartMsg = _("The computer will need to restart before Git will be activated and the document histories can be managed. First close this message, then quit Adapt It, and restart your computer. The next time you run Adapt It the document history items will work on the Adapt It File menu.");
                    wxMessageBox(needsRestartMsg, _("Git Installation completed"), wxICON_INFORMATION | wxOK);
                }
            }
            else // returnValue == wxID_CANCEL
            {
                // User canceled from the GetOpenFileName dialog
                wxString msg = _("No Git Installer was selected. Git will not be installed if you continue.\nIf you still want to install Git, after you click OK, you can try one of the Git install options page, and try again, or click Cancel to quit the options page.");
                wxMessageBox(msg, _T("No Git installer selected"), wxICON_INFORMATION | wxOK);
                return;
            }
            bUseFileDialog = FALSE;
        }
    }

    event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CInstallGitOptionsDlg::OnCancel(wxCommandEvent& event)
{
    event.Skip();
}



void CInstallGitOptionsDlg::OnRadioDoNotInstallGit(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnDoNotInstallGitNow->SetValue(TRUE);
    pRadioBtnDownloadAndInstallGitFromInternet->SetValue(FALSE);
    pRadioBtnBrowseForGitInstaller->SetValue(FALSE);
}



void CInstallGitOptionsDlg::OnRadioDownloadGitAndInstall(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnDoNotInstallGitNow->SetValue(FALSE);
    pRadioBtnDownloadAndInstallGitFromInternet->SetValue(TRUE);
    pRadioBtnBrowseForGitInstaller->SetValue(FALSE);
}

void CInstallGitOptionsDlg::OnRadioBrowseForGitAndInstall(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnDoNotInstallGitNow->SetValue(FALSE);
    pRadioBtnDownloadAndInstallGitFromInternet->SetValue(FALSE);
    pRadioBtnBrowseForGitInstaller->SetValue(TRUE);
}

