/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguageCodesDlg_Single.h
/// \author			Bill Martin, Bruce Waters
/// \date_created	14 April 2015
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CLanguageCodesDlg_Single class. 
/// The CLanguageCodesDlg_Single class provides a dialog in which the user can enter
/// the ISO639-3 2- and 3-letter language code for creating the start of a custom language code.
/// The dialog allows the user to search for the code by language name or the code.
/// \derivation		The CLanguageCodesDlg_Single class is derived from AIModalDialog.
/// This is a cut-down class copied from CLanguageCodesDlg and simplified
////////////////////////////////////////////////////////////////////////////////

#ifndef LanguageCodesDlg_Single_h
#define LanguageCodesDlg_Single_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "LanguageCodesDlg_Single.h"
#endif

class CLanguageCodesDlg_Single : public AIModalDialog
{
public:
	CLanguageCodesDlg_Single(wxWindow* parent); // constructor
	CLanguageCodesDlg_Single(wxWindow* parent, enum LangCodesChoice choice); // constructor for use in KB Sharing Manager GUI
	virtual ~CLanguageCodesDlg_Single(void); // destructor
	// other methods
	wxString m_langCode; // was m_sourceLangCode;

	wxString m_searchString;
	int m_curSel;
	bool m_bISO639ListFileFound;
	bool m_bFirstCodeSearch;
    bool m_bFirstNameSearch;

	wxListBox* pListBox;
	wxTextCtrl* pEditSearchForLangName;
	wxTextCtrl* pEditLangCode; // was pEditSourceLangCode;

	wxButton* pBtnFindCode;
	wxButton* pBtnFindLanguage;
	wxButton* pBtnUseSelectionAsCode; // was pBtnUseSelectionAsSource;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void SetPointers();
	
	void OnFindCode(wxCommandEvent& WXUNUSED(event));
	void OnFindLanguage(wxCommandEvent& WXUNUSED(event));
	void OnUseSelectedCodeForCode(wxCommandEvent& WXUNUSED(event)); // was OnUseSelectedCodeForSrcLanguage(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListboxLanguageCodes(wxCommandEvent& WXUNUSED(event));
	void OnEnterInSearchBox(wxCommandEvent& WXUNUSED(event));
	wxString Get3LetterCodeFromLBItem(); // helper function

private:
	
	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* LanguageCodesDlg_Single_h */
