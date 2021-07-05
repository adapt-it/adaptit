/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			InstallGitOptionsDlg.h
/// \author			Bill Martin
/// \date_created	24 March 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CInstallGitOptionsDlg class. 
/// The CInstallGitOptionsDlg class creates an AIModal dialog that allows the user to select from
/// 3 options - represented by 3 radio buttons that indicate how the Git program should be installed
/// by Adapt It.
/// \derivation		The CInstallGitOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef InstallGitOptionsDlg_h
#define InstallGitOptionsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "InstallGitOptionsDlg.h"
#endif

class CInstallGitOptionsDlg : public AIModalDialog
{
public:
    CInstallGitOptionsDlg(wxWindow* parent); // constructor
    virtual ~CInstallGitOptionsDlg(void); // destructor
                               // other methods
    wxRadioButton* pRadioBtnDoNotInstallGitNow;
    wxRadioButton* pRadioBtnDownloadAndInstallGitFromInternet;
    wxRadioButton* pRadioBtnBrowseForGitInstaller;
    wxTextCtrl* pStaticTextPreamble;
    wxTextCtrl* pStaticDescTopBtn;
    wxTextCtrl* pStaticDescMiddleBtn;
    wxTextCtrl* pStaticDescBottomBtn;

    // the following variables made public for possibl use in CAdapt_ItApp::OnToolsInstallGit()
    
    wxString gitVerNumAlreadyInstalled;
    wxString topPreambleStr;
    wxString verNumWithDots;
    wxString topStr;
    wxString middleStr; 
    wxString bottomStr;

    wxString PathToAIInstallation;
    wxString GitInstallerPathAndName;
    bool bGitInstalled;
    bool bGitInstallerExistsLocally;
    bool bUseFileDialog;
    wxString needsRestartMsg;
    wxString msg;
    wxString GitInstallerFileName;
    wxString GitDownloadInstallerFileName;
    wxString GitDownloadInstallerPathAndName;
    

protected:
    void InitDialog(wxInitDialogEvent& WXUNUSED(event));
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnRadioDoNotInstallGit(wxCommandEvent& event);
    void OnRadioDownloadGitAndInstall(wxCommandEvent& event);
    void OnRadioBrowseForGitAndInstall(wxCommandEvent& event);


private:
    CAdapt_ItApp* m_pApp;

    DECLARE_EVENT_TABLE()
};
#endif /* InstallGitOptionsDlg_h */
