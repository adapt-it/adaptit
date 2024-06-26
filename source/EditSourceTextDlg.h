/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EditSourceTextDlg.h
/// \author			Bill Martin
/// \date_created	13 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
	//enum { IDD = IDD_EDIT_SOURCE };

	// pointers for the dialog's text ctrls
	wxTextCtrl* pSrcTextEdit;
	wxTextCtrl* pPreContextEdit;
	wxTextCtrl* pFollContextEdit;
	wxTextCtrl* pOldSrcTextEdit;
    // TODO: Remove following two info text ctrls after Help button is implemented and tested.
    //wxTextCtrl* pTextCtrlEditAsStatic1;
	//wxTextCtrl* pTextCtrlEditAsStatic2;
    wxButton* pBtnOK;
    wxButton* pBtnCancel;

	wxString	m_strNewSourceText;
	wxString	m_preContext;
	wxString	m_follContext;
	//bool		m_bEditMarkersWanted;
	//wxString	m_strOldTranslationText;
	wxString	m_strOldSourceText;
	//wxString	m_chapterMarker;
	
	wxSizer* pEditSourceTextDlgSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void ReinterpretEnterKeyPress(wxCommandEvent& WXUNUSED(event));
    void OnHelpOnEditingSourceText(wxCommandEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* EditSourceTextDlg_h */
