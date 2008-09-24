/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			GoToDlg.h
/// \author			Bill Martin
/// \date_created	1 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
	CGoToDlg(wxWindow* parent); // constructor
	virtual ~CGoToDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_GO_TO };
	wxSpinCtrl* m_pSpinCtrlChapter;
	wxSpinCtrl* m_pSpinCtrlVerse;
	int			m_nChapter;
	int			m_nVerse;
	wxString	m_chapterVerse;
	wxString	m_verse;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnSetFocus(wxFocusEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* GoToDlg_h */
