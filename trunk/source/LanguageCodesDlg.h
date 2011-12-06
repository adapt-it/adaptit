/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguageCodesDlg.h
/// \author			Bill Martin
/// \date_created	5 May 2010
/// \date_revised	5 May 2010
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CLanguageCodesDlg class. 
/// The CLanguageCodesDlg class provides a dialog in which the user can enter
/// the ISO639-3 3-letter language codes for the source and target languages.
/// The dialog allows the user to search for the codes by language name.
/// \derivation		The CLanguageCodesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

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
	wxString m_searchString;
	int m_curSel;
	bool m_bISO639ListFileFound;

	wxListBox* pListBox;
	wxTextCtrl* pEditSearchForLangName;
	wxTextCtrl* pEditSourceLangCode;
	wxTextCtrl* pEditTargetLangCode;
	wxTextCtrl* pEditGlossLangCode;
	wxButton* pBtnFindCode;
	wxButton* pBtnFindLanguage;
	wxButton* pBtnUseSelectionAsSource;
	wxButton* pBtnUseSelectionAsTarget;
	wxButton* pBtnUseSelectionAsGloss;
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
	void OnSelchangeListboxLanguageCodes(wxCommandEvent& WXUNUSED(event));
	void OnEnterInSearchBox(wxCommandEvent& WXUNUSED(event));
	wxString Get3LetterCodeFromLBItem(); // helper function

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* LanguageCodesDlg_h */
