/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguageCodesDlg.h
/// \author			Bill Martin
/// \date_created	5 May 2010
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CLanguageCodesDlg class. 
/// The CLanguageCodesDlg class provides a dialog in which the user can enter
/// the ISO639-3 3-letter language codes for the source and target languages.
/// The dialog allows the user to search for the codes by language name.
/// \derivation		The CLanguageCodesDlg class is derived from AIModalDialog.
/// BEW 23Jul12, extended to include support for free translation's language & lang code
////////////////////////////////////////////////////////////////////////////////

#ifndef LanguageCodesDlg_h
#define LanguageCodesDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "LanguageCodesDlg.h"
#endif

class CLanguageCodesDlg : public AIModalDialog
{
public:
	CLanguageCodesDlg(wxWindow* parent); // constructor
	virtual ~CLanguageCodesDlg(void); // destructor
	// other methods
	wxString m_sourceLangCode;
	wxString m_targetLangCode;
	wxString m_glossLangCode;
	wxString m_freeTransLangCode; // BEW added 23Jul12

	wxString m_glossesLangName; // BEW added 23Jul12
	wxString m_freeTransLangName; // BEW added 23Jul12

	wxString m_searchString;
	int m_curSel;
	bool m_bISO639ListFileFound;
	bool m_bFirstCodeSearch;
    bool m_bFirstNameSearch;

	wxListBox* pListBox;
	wxTextCtrl* pEditSearchForLangName;
	wxTextCtrl* pEditSourceLangCode;
	wxTextCtrl* pEditTargetLangCode;
	wxTextCtrl* pEditGlossLangCode;
	wxTextCtrl* pEditFreeTransLangCode; // BEW added 23Jul12

	wxButton* pBtnFindCode;
	wxButton* pBtnFindLanguage;
	wxButton* pBtnUseSelectionAsSource;
	wxButton* pBtnUseSelectionAsTarget;
	wxButton* pBtnUseSelectionAsGloss;
	wxButton* pBtnUseSelectionAsFreeTrans;

	wxStaticText* pStaticScrollList;
	wxStaticText* pStaticSearchForLangName;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	
	void OnFindCode(wxCommandEvent& WXUNUSED(event));
	void OnFindLanguage(wxCommandEvent& WXUNUSED(event));
	void OnUseSelectedCodeForSrcLanguage(wxCommandEvent& WXUNUSED(event));
	void OnUseSelectedCodeForTgtLanguage(wxCommandEvent& WXUNUSED(event));
	void OnUseSelectedCodeForGlsLanguage(wxCommandEvent& WXUNUSED(event));
	void OnUseSelectedCodeForFreeTransLanguage(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListboxLanguageCodes(wxCommandEvent& WXUNUSED(event));
	void OnEnterInSearchBox(wxCommandEvent& WXUNUSED(event));
	wxString Get3LetterCodeFromLBItem(); // helper function

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	wxString m_associatedLanguageName; // set for each call of Get3LetterCodeFromLBItem()
			// but use the value only for a gloss language choice, or free translation
			// language choice (the source and target languages have to be kept more
			// potentially independent, to allow for multiple AI projects with the same
			// pair of src & tgt language codes)
	// The following two booleans govern when m_associatedLanguageName's contents are used
	// (one has to be true for it to be used) and to which member variable it's value will
	// be assigned to (if m_bGlossBtnChosen is TRUE, then assign it to m_glossesLangName,
	// else the latter must be empty; if m_bFreeTransBtnChosen is TRUE, assign it to
	// m_freeTransLangName, else the latter must be empty). Do the tests and assignment as
	// late as possible (therefore, within OnOK()) because the user may fidddle about in
	// the dialog choosing several times before setting on one choice as the correct one -
	// so only retain the knowledge of the last choice).
	bool m_bGlossBtnChosen;
	bool m_bFreeTransBtnChosen;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* LanguageCodesDlg_h */
