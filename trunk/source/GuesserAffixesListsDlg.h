/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserAffixesListsDlg.h
/// \author			Kevin Bradford & Bruce Waters
/// \date_created	7 March 2014
/// \rcs_id $Id: GuesserAffixesListDlg.h 3296 2013-06-12 04:56:31Z adaptit.bruce@gmail.com $
/// \copyright		2014 Bruce Waters, Bill Martin, Kevin Bradford, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the GuesserAffixesListsDlg class. 
/// The GuesserAffixesListsDlg class provides a dialog in which the user can create a list of
/// prefixes in two columns, each line being a src language prefix in column one, and a
/// target language prefix in column two. Similarly, a separate list of suffix pairs can
/// be created. No hyphens should be typed. Explicitly having these morpheme sets makes
/// the guesser work far more accurately.
/// \derivation		The GuesserAffixesListsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef GuesserAffixesListsDlg_h
#define GuesserAffixesListsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GuesserAffixesListsDlg.h"
#endif

enum PairsListType {
	prefixesListType = 1,
	suffixesListType
};

typedef struct {
	PairsListType pairType; // 1 for prefixes, 2 for suffixes ( ********assumed, change it to 2 & 1 if I got this round the wrong way & fix code accordingly ************)
	wxString srcLangAffix;
	wxString tgtLangAffix;
} AffixPair;

WX_DEFINE_ARRAY(AffixPair*, AffixPairsArray); // this type is unsorted

class GuesserAffixesListsDlg : public AIModalDialog
{
public:
	GuesserAffixesListsDlg(wxWindow* parent); // constructor
	virtual ~GuesserAffixesListsDlg(void); // destructor
	
	// The top button which displays the html message in default browser, or failing that,
	// in an instance of the HTMLViewer class
	wxButton*		m_pBtnHowToUseThisDlg;
	// The two text controls
	wxTextCtrl*		m_pSrcAffix;
	wxTextCtrl*		m_pTgtAffix;
	// The two radio buttons
	wxRadioButton*	m_pRadioSuffixesList;
	wxRadioButton*	m_pRadioPrefixesList;
	// Support for changing the labels from Source Language Affix to either
	// Source Language Suffix      and
	// Target Language Suffix
	// and similarly for when Prefix List is chose, the two labels would be
	// Source Language Prefix      and
	// Target Language Prefix
	wxStaticText*	m_pSrcLangAffix;
	wxStaticText*	m_pTgtLangAffix;
	// Store the above four label strings in the following members
	wxString		m_strSrcLangSuffix;
	wxString		m_strTgtLangSuffix;
	wxString		m_strSrcLangPrefix;
	wxString		m_strTgtLangPrefix;
	// Support for the middle four buttons & OK & Cancel
	wxButton*		m_pBtnAdd;
	wxButton*		m_pBtnUpdate;
	wxButton*		m_pBtnInsert;
	wxButton*		m_pBtnDelete;
	wxButton*		m_pBtnOK;
	wxButton*		m_pBtnCancel;
	// The list control, and support for storing the pairs of prefixes in one list, and pairs
	// of suffixes in another list
	wxListCtrl*		m_pAffixPairsList;

	// Support the 2 radio buttons
	PairsListType	m_pltCurrentAffixPairListType;
	// Support hiding the "hyphens" before or after the checkboxes, depending 
	// on which radio button was chosen by the user
	wxStaticText*	m_pHyphenSrcSuffix; // to left of src affix text box
	wxStaticText*	m_pHyphenSrcPrefix; // to right of src affix text box
	wxStaticText*	m_pHyphenTgtSuffix; // to left of tgt affix text box
	wxStaticText*	m_pHyphenTgtPrefix; // to right of tgt affix text box

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// Button handlers etc
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnExplanationDlgWanted(wxCommandEvent& WXUNUSED(event));
	void OnRadioButtonPrefixesList(wxCommandEvent& WXUNUSED(event));
	void OnRadioButtonSuffixesList(wxCommandEvent& WXUNUSED(event));
	void OnListItemSelected(wxListEvent& event);
	void OnAdd(wxCommandEvent& event);
	void OnUpdate(wxCommandEvent& event);
	void OnInsert(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);


	bool LoadDataForListType(PairsListType myType);

	bool LoadPrefixes();
	bool LoadSuffixes();

	AffixPairsArray* GetPrefixes();
	AffixPairsArray* GetSuffixes();

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	CAdapt_ItApp*		m_pApp;
	wxWindow*			m_pParent; // the instance of GuesserSettingsDlg, wxStaticLine must know its parent
	
	// other class attributes
	AffixPairsArray* m_pSuffixesPairsArray;
	AffixPairsArray* m_pPrefixesPairsArray;
	bool m_bPrefixesLoaded; // TRUE indicates that local affix arrays have been loaded from file / app
	bool m_bSuffixesLoaded; // TRUE indicates that local affix arrays have been loaded from file / app
	bool m_bPrefixesUpdated; // TRUE indicates that updates have been made that have not been written to file
	bool m_bSuffixesUpdated; // TRUE indicates that updates have been made that have not been written to file

	// Get index of selected item from control
	long GetSelectedItemIndex();
	wxString GetCellContentsString( long row_number, int column ); 

	DECLARE_EVENT_TABLE()
};
#endif /* GuesserAffixesListsDlg_h */
