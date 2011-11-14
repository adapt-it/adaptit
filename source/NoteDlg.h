/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NoteDlg.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CNoteDlg class. 
/// The CNoteDlg class provides a dialog for creating, finding, editing, deleting,  
/// and navigating through Adapt It notes (those prefixed by \note).
/// The CNoteDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CNoteDlg class is derived from wxScrollingDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef NoteDlg_h
#define NoteDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "NoteDlg.h"
#endif

/// The CNoteDlg class provides a dialog for creating, finding, editing, deleting,  
/// and navigating through Adapt It notes (those prefixed by \note).
/// The CNoteDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CNoteDlg class is derived from wxScrollingDialog.
class CNoteDlg : public wxScrollingDialog
{
public:
	CNoteDlg(wxWindow* parent); // constructor
	virtual ~CNoteDlg(void); // destructor
	// other methods
	wxTextCtrl	m_editNote;
	wxTextCtrl* pEditNote; // whm added

protected:
	wxString	m_strNote;
	wxString	markers;
	int		m_noteOffset; // offset to first character of the note's content,
						  // in the pSrcPhrase's m_markers string
	int		m_noteLen;	  // length of the note content, including the trailing space
	bool	m_bPreExisting; // TRUE if the dialog is opened on preexisting note text, false if
							// it is a virgin location (we use this to govern Cancel button's behaviour)
	wxString m_saveText; // save the original text (for use by Cancel button)

	wxButton* pNextNoteBtn;
	wxButton* pFindNextBtn;
	wxButton* pPrevNoteBtn;
	wxButton* pFirstNoteBtn;
	wxButton* pLastNoteBtn;
	wxButton* pOKButton;
	wxTextCtrl* pEditSearch; // whm added
	wxString  m_searchStr; // the search string itself



// Dialog Data
	//enum { IDD = IDD_NOTE_DLG };

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

public:
	void OnOK(wxCommandEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedNextBtn(wxCommandEvent& event);
	void OnBnClickedDeleteBtn(wxCommandEvent& event);
	void OnBnClickedPrevBtn(wxCommandEvent& event);
	void OnBnClickedFirstBtn(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLastBtn(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedFindNextBtn(wxCommandEvent& event);
	void OnEnChangeEditBoxSearchText(wxCommandEvent& WXUNUSED(event));
	void OnIdle(wxIdleEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* NoteDlg_h */
