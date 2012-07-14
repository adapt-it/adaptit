/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseCollabOptionsDlg.h
/// \author			Bill Martin
/// \date_created	18 February 2012
/// \date_revised	18 February 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CChooseCollabOptionsDlg class. 
/// The CChooseCollabOptionsDlg class implements a 3-radio button dialog that allows the user to
/// choose to Turn Collaboration ON, Turn Collaboration OFF, or Turn Read-Only Mode ON. This
/// dialog is called from the ProjectPage of the Start Working wizard if the project just
/// opened is one that has previously been setup to collaborate with Paratext/Bibledit.
/// \derivation		The CChooseCollabOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ChooseCollabOptionsDlg_h
#define ChooseCollabOptionsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ChooseCollabOptionsDlg.h"
#endif

class CChooseCollabOptionsDlg : public AIModalDialog
{
public:
	CChooseCollabOptionsDlg(wxWindow* parent); // constructor
	virtual ~CChooseCollabOptionsDlg(void); // destructor
	// other methods
	wxRadioButton* pRadioTurnCollabON;
	wxRadioButton* pRadioTurnCollabOFF;
	wxRadioButton* pRadioTurnReadOnlyON;
	wxStaticText* pStaticTextAIProjName;
	wxTextCtrl* pStaticAsTextCtrlNotInstalledErrorMsg;
	wxButton* pBtnTellMeMore;
	wxButton* pBtnOK;
	wxString m_aiProjName;
	wxArrayString projList;
	bool m_bRadioSelectCollabON;
	bool m_bRadioSelectCollabOFF;
	bool m_bRadioSelectReadOnlyON;
	bool m_bEditorIsAvailable;
	wxSizer* pChooseCollabOptionsDlgSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnBtnTellMeMore(wxCommandEvent& WXUNUSED(event));
	void OnRadioTurnCollabON(wxCommandEvent& WXUNUSED(event));
	void OnRadioTurnCollabOFF(wxCommandEvent& WXUNUSED(event));
	void OnRadioReadOnlyON(wxCommandEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& event);

private:
	// class attributes

	CAdapt_ItApp* m_pApp;
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseCollabOptionsDlg_h */
