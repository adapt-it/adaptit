/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GoToDlg.h
/// \author			Bill Martin
/// \date_created	1 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CGoToDlg class. 
/// The CGoToDlg class provides a simple dialog in which the user can enter a reference
/// and jump to it in the document.
/// \derivation		The CGoToDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef GoToDlg_h
#define GoToDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GoToDlg.h"
#endif

/// The CGoToDlg class provides a simple dialog in which the user can enter a reference
/// and jump to it in the document.
/// \derivation		The CGoToDlg class is derived from AIModalDialog.
class CGoToDlg : public AIModalDialog
{
public:
	// Expose all overloads of InitDialog from the base class (wxWindowBase)
	using wxWindowBase::InitDialog; // whm 6Dec2025 added to avoid gcc warning
	CGoToDlg(wxWindow* parent); // constructor
	virtual ~CGoToDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_GO_TO };
	wxSpinCtrl* m_pSpinCtrlChapter;
	wxSpinCtrl* m_pSpinCtrlVerse;
	wxButton* m_pButtonGoBackTo;
	wxComboBox* m_pComboBoxGoBackTo;
	int			m_nChapter;
	int			m_nVerse;
	wxString	m_chapterVerse;
	wxString	m_verse;
	bool		m_bComboTextChanged;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnButtonGoBackTo(wxCommandEvent& WXUNUSED(event)); // whm 25Oct2022 added for revised Go To dialog
	void OnComboBox(wxCommandEvent& WXUNUSED(event)); // whm 25Oct2022 added for revised Go To dialog
	void OnSetFocus(wxFocusEvent& event);
	void OnComboTextChanged(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* GoToDlg_h */
