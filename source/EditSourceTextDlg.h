/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			EditSourceTextDlg.h
/// \author			Bill Martin
/// \date_created	13 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CEditSourceTextDlg class. 
/// The CEditSourceTextDlg class provides a dialog in which the user can edit the
/// source text. Restrictions are imposed to prevent such editing while glossing, or 
/// if the source text has disparate text types, is a retranslation or has a free
/// translation or filtered information contained within it.
/// \derivation		The CEditSourceTextDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef EditSourceTextDlg_h
#define EditSourceTextDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "EditSourceTextDlg.h"
#endif

/// The CEditSourceTextDlg class provides a dialog in which the user can edit the
/// source text. Restrictions are imposed to prevent such editing while glossing, or 
/// if the source text has disparate text types, is a retranslation or has a free
/// translation or filtered information contained within it.
/// \derivation		The CEditSourceTextDlg class is derived from AIModalDialog.
class CEditSourceTextDlg : public AIModalDialog
{
public:
	CEditSourceTextDlg(wxWindow* parent); // constructor
	virtual ~CEditSourceTextDlg(void); // destructor

	// pointers for the dialog's text ctrls
	wxTextCtrl* pSrcTextEdit;
	wxTextCtrl* pPreContextEdit;
	wxTextCtrl* pFollContextEdit;
	wxTextCtrl* pTgtEdit;
	wxCheckBox* pCheckForceMkrDlg;

	// other methods
	//enum { IDD = IDD_EDIT_SOURCE };
	wxString	m_strNewSourceText;
	wxString	m_preContext;
	wxString	m_follContext;
	bool		m_bEditMarkersWanted;
	wxString	m_strOldTranslationText;
	wxString	m_strOldSourceText;
	wxString	m_chapterMarker;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* EditSourceTextDlg_h */
