/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConflictResActionDlg.h
/// \author			Bruce Waters
/// \date_created	15 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CConflictResActionDlg class. 
/// The CConflictResActionDlg class puts up a dialog for the user to indicate
/// whether the conflict resolution should be done the legacy way (favouring keeping
/// the PT or BE verse version unchanged), or to force the AI verse version to be
/// transferred to the external editor, or to request a further dialog for the visual
/// comparison of verse versions to be shown for the user to select which is best for
/// each listed conflict. The user's response will set one of three app variables,
/// which then guide what happens subsequently.
/// \derivation		The CConflictResActionDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ConflictResActionDlg_h
#define ConflictResActionDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ConflictResActionDlg.h"
#endif

class CConflictResActionDlg : public AIModalDialog
{
public:
	CConflictResActionDlg(wxWindow* parent); // constructor
	virtual ~CConflictResActionDlg(void);    // destructor
	// other methods

	void OnBnClickedRadioRetainPTorBE(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioForceAI(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioConflictResDlg(wxCommandEvent& WXUNUSED(event));
	bool m_bLegacy_retain_PTorBE_version;
	bool m_bForce_AI_version_transfer;
	bool m_bUserWantsVisualConflictResolution;
	wxRadioButton* pRadioRetainPT;
	wxRadioButton* pRadioForceAI;
	wxRadioButton* pRadioConflictResDlg;
	wxString m_collabEditorName;
	wxStaticBoxSizer* pTopStaticSizer;
	wxStaticBoxSizer* pMiddleStaticSizer;
	wxStaticBoxSizer* pBottomStaticSizer;
	wxBoxSizer* pWholeDlgSizer;
	wxString appNameStr;
	wxTextCtrl* topBox;
	wxTextCtrl* middleBox;
	wxTextCtrl* bottomBox;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

private:
	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* ConflictResActionDlg_h */
