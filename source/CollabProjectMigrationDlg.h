/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabProjectMigrationDlg.h
/// \author			Bill Martin
/// \date_created	5 April 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCollabProjectMigrationDlg class. 
/// The CCollabProjectMigrationDlg class creates a dialog that allows a user who currently collaborates with
/// Paratext projects in Paratext 7 to migrate the Adapt It project to collaborate with the same Paratext
/// projects once they have been migrated to PT8.
/// \derivation		The CCollabProjectMigrationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CollabProjectMigrationDlg_h
#define CollabProjectMigrationDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CollabProjectMigrationDlg.h"
#endif

class CCollabProjectMigrationDlg : public AIModalDialog
{
public:
    CCollabProjectMigrationDlg(wxWindow* parent, wxString aiProj, wxString srcProject, wxString tgtProject, wxString freeTransProject); // constructor
    virtual ~CCollabProjectMigrationDlg(void); // destructor
    wxRadioButton* pRadioBtnPT8;
    wxRadioButton* pRadioBtnPT7;
    wxStaticText* pStaticTextSrcProj;
    wxStaticText* pStaticTextTgtProj;
    wxStaticText* pStaticTextFreeTransProj;
    wxStaticText* pStaticAICollabProj;
    wxCheckBox* pCheckBoxDoNotShowAgain;
    bool m_bDoNotShowAgain;
    bool m_bPT8BtnSelected;

protected:
    void InitDialog(wxInitDialogEvent& WXUNUSED(event));
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnRadioBtnPT8(wxCommandEvent& WXUNUSED(event));
    void OnRadioBtnPT7(wxCommandEvent& WXUNUSED(event));
    void OnCheckDontShowAgain(wxCommandEvent& WXUNUSED(event));
    wxString aiProject;
    wxString sourceProject;
    wxString targetProject;
    wxString freeTranslationProject;


private:
    // class attributes
    // wxString m_stringVariable;

    // other class attributes

    DECLARE_EVENT_TABLE()
};
#endif /* CollabProjectMigrationDlg_h */
