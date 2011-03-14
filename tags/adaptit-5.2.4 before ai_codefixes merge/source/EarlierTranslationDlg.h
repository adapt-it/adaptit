/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EarlierTranslationDlg.h
/// \author			Bill Martin
/// \date_created	23 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CEarlierTranslationDlg class. 
/// The CEarlierTranslationDlg class allows the user to view an earlier translation made
/// within the same document (choosing its location by reference), and optionally jump 
/// there if desired.
/// The CEarlierTranslationDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CEarlierTranslationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef EarlierTranslationDlg_h
#define EarlierTranslationDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "EarlierTranslationDlg.h"
#endif

// forward references
class CAdapt_ItView;
class CSourcePhrase;

/// The CEarlierTranslationDlg class allows the user to view an earlier translation made
/// within the same document (choosing its location by reference), and optionally jump 
/// there if desired.
/// The CEarlierTranslationDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CEarlierTranslationDlg class is derived from AIModalDialog.
class CEarlierTranslationDlg : public AIModalDialog
{
public:
	CEarlierTranslationDlg(wxWindow* parent); // constructor
	virtual ~CEarlierTranslationDlg(void); // destructor // whm make all destructors virtual

	//enum { IDD = IDD_EARLIER_TRANSLATION };
	wxSizer*	pEarlierTransSizer;
	wxString	m_srcText;
	wxString	m_tgtText;
	int			m_nChapter;
	int			m_nVerse;
	wxString	m_strBeginChVerse;
	wxString	m_strEndChVerse;
	wxTextCtrl*	m_pSrcTextBox;
	wxTextCtrl*	m_pTgtTextBox;
	wxSpinCtrl* m_pChapterSpinCtrl;
	wxSpinCtrl* m_pVerseSpinCtrl;
	wxStaticText* m_pBeginChVerseStaticText;
	wxStaticText* m_pEndChVerseStaticText;
	bool		m_bIsVerseRange;
	int			m_nVerseRangeEnd;
	CAdapt_ItView* m_pView;
	wxString m_chapterVerse;
	wxString m_verse;
	int		m_nFirstSequNumBasic;
	int		m_nLastSequNumBasic;
	int		m_nExpansionIndex; // index into the next 2 arrays
	int		m_preContext[10]; // first expansion will be index = 0; ie. an extra 2 verses next to basic one
	int		m_follContext[10];
	int		m_nCurPrecChapter; // chapter number for text in preceding context
	int		m_nCurFollChapter; // chapter number for text in following context
	int		m_nCurPrecVerse;	// ditto for preceding context verse
	int		m_nCurFollVerse;	// ditto for following context
	int		m_nCurLastSequNum;  // sequence number of last src phrase in the current (possibly expanded)
								// state -- we need this to check if we are at the end of the list

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& WXUNUSED(event)); // necessary since EarlierTranslationDlg is modeless - must call Destroy
	void OnClose(wxCloseEvent& WXUNUSED(event));
	bool IsMarkedForVerse(CSourcePhrase* pSrcPhrase);
	void ScanVerse(SPList::Node*& pos, CSourcePhrase* pSrcPhrase, SPList* WXUNUSED(pList));
	void EnableMoreButton(bool bEnableFlag);
	void EnableLessButton(bool bEnableFlag);
	void EnableJumpButton(bool bEnableFlag);

	void OnGetChapterVerseText(wxCommandEvent& WXUNUSED(event));
	void OnCloseAndJump(wxCommandEvent& event);
	void OnShowMoreContext(wxCommandEvent& WXUNUSED(event));
	void OnShowLessContext(wxCommandEvent& event);

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* EarlierTranslationDlg_h */
