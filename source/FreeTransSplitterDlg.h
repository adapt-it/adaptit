/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTransSplitterDlg.h
/// \author			Bruce Waters
/// \date_created	29 November 2013
/// \rcs_id $Id: FreeTransSplitterDlg.h 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the FreeTransSplitterDlg class.
/// The FreeTransSplitterDlg class provides a handler for the "Split" radio button option in the 
/// FreeTransAdjustDlg dialog. The dialog has four multiline edit boxes. All are "read only".
/// The top one displays the text (typically a translation of a source text) of the
/// current free translation section - and typically that free translation will have text
/// in it which doesn't belong in the current free translation section - hence the need
/// for this dialog and the option which invokes it. 
/// The second edit box displays the
/// current section's free translation. This is the text to be split into two parts. This
/// dialog allows the user to click where he wants the text divided into two parts - the
/// first part will be retained in the current free translation section as its sufficient free
/// translation (although it would be possible to edit it later if the user chooses), and
/// the remainder is put into the start of the next section. If the next section does not
/// exist yet, it is created automatically in order to receive the text. The section which
/// receives the remainder also becomes the new active section.
/// The third text box displays the first part of the split text. Splitting is always done
/// between words. If the click was between words, the text is divided there. If it was in
/// a word, that clicked word becomes the first word of the remainder. If a range of
/// characters is selected, the first character in that range is interpretted as an
/// insertion point and the above two rules then apply. We split whole words, never two
/// parts of a single word.
/// The last text box displays the remainder from the split - it will be put into the next
/// section. If the next section already exists (i.e. it exists and its start abutts the
/// end of the current section) and has a free translation, the remainder text is inserted
/// before it and a space divider is guaranteed to be put there. We preserve the user's
/// typing location, which in most circumstances will move to the next section. However,
/// it the typing location was within the first split of part, then in the new section it
/// will be put at the end of the "remainder" text moved to there.
/// The wxDesigner resource is SplitterDlgFunc
/// \derivation		The FreeTransSplitterDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef FreeTransSplitterDlg_h
#define FreeTransSplitterDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FreeTransSplitterDlg.h"
#endif

// forward declarations
class CFreeTrans;
class CMainFrame; // use this for the dialog's parent

class FreeTransSplitterDlg : public AIModalDialog
{
public:
	FreeTransSplitterDlg(wxWindow*	parent); // constructor
	virtual ~FreeTransSplitterDlg(); // destructor

	// member variables
	wxString m_theText;
	wxString m_theFreeTrans;
	wxString m_FreeTransForCurrent;
	wxString m_FreeTransForNext;

	// pointers to the text controls
	wxTextCtrl* m_pEditText;
	wxTextCtrl* m_pEditFreeTrans;
	wxTextCtrl* m_pEditForCurrent;
	wxTextCtrl* m_pEditForNext;


protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnButtonSplitHere(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	CFreeTrans*			m_pFreeTrans; // pointer to the one and only CFreeTrans instance
	CAdapt_ItApp*		m_pApp; // pointer to the application instance
	wxSizer*			m_pFreeTransSplitterSizer;
	CMainFrame*			m_pMainFrame; // the parent window
	long				m_from;
	long				m_to;
	size_t				m_offset;

	DECLARE_EVENT_TABLE()
};
#endif /* FreeTransSplitterDlg_h */
