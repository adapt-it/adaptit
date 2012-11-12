/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CaseEquivPage.h
/// \author			Bill Martin
/// \date_created	29 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCaseEquivPageWiz class and the CCaseEquivPagePrefs class. 
/// A third class CCaseEquivPageCommon handles the 
/// routines that are common to the two classes above. These classes manage a page
/// that allows the user to enter and/or manage the lower and upper case character 
/// equivalences for the source, target and gloss languages. It is designed so that 
/// the page reveals progressively more controls as needed in response how the user 
/// responds to the first two check boxes on the page. The first check box that 
/// appears is:
/// [ ] Check here if the source text contains both capital letters (upper case) and 
///     small letters (lower case)
/// If the user checks this box, then the following checkbox appears:
/// [ ] Check here if you want Adapt It to automatically distinguish between upper 
///     case and lower case letters
/// If the user checks this second box, then all the remaining static text,
/// buttons, and three edit boxes become visible on the dialog/pane allowing
/// the user to actually edit/define the lower to upper case equivalences for
/// the language project. The interface resources for the page are defined in 
/// the CaseEquivDlgFunc() function which was developed and is maintained by 
/// wxDesigner.
/// \derivation	CCaseEquivPageWiz is derived from wxWizardPage, CCaseEquivPagePrefs from wxPanel, and CCaseEquivPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef LowerToUpperCaseEquivPage_h
#define LowerToUpperCaseEquivPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CaseEquivPage.h"
#endif

/// The CCaseEquivPageCommon class contains data and methods which are 
/// common to the CCaseEquivPageWiz and CCaseEquivPagePrefs classes.
/// \derivation The CCaseEquivPageCommon class is derived from wxPanel
class CCaseEquivPageCommon : public wxPanel
// CCaseEquivPageCommon needs to be derived from wxPanel in order to use built-in 
// functions like FindWindowById()
{
public:

	wxSizer* pCaseEquivSizer;

	// for the Src wxTextCtrl multiline box
	wxTextCtrl* m_pEditSrcEquivalences;
	wxString m_strSrcEquivalences;

	// for the Tgt wxTextCtrl multiline box
	wxTextCtrl* m_pEditTgtEquivalences;
	wxString m_strTgtEquivalences;

	// for the Gloss wxTextCtrl multiline box
	wxTextCtrl* m_pEditGlossEquivalences;
	wxString m_strGlossEquivalences;

	//enum { IDD = IDD_LOWER_TO_UPPER_EQUIVALENCES };
	void DoSetDataAndPointers();
	void DoInit();

	void DoBnClickedClearSrcList();
	void DoBnClickedSrcSetEnglish();
	void DoBnClickedSrcCopyToNext();
	void DoBnClickedSrcCopyToGloss();

	void DoBnClickedClearTgtList();
	void DoBnClickedTgtSetEnglish();
	void DoBnClickedTgtCopyToNext();

	void DoBnClickedClearGlossList();
	void DoBnClickedGlossSetEnglish();
	void DoBnClickedGlossCopyToNext();

	void DoBnCheckedSrcHasCaps(); // added by whm 11Aug04
	void DoBnCheckedUseAutoCaps(); // added by whm 11Aug04

	// helpers
	void BuildListString(wxString& lwrCase, wxString& upprCase, wxString& strList);
	bool BuildUcLcStrings(wxString& strList, wxString& lwrCase, wxString& upprCase);
	void ToggleControlsVisibility(bool visible);

};

/// The CCaseEquivPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the case equivalents settings used for the project during 
/// initial setup.
/// The interface resources for the page/panel are defined in CaseEquivDlgFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CCaseEquivPageWiz is derived from wxWizardPage.
class CCaseEquivPageWiz : public wxWizardPage
{
public:
	CCaseEquivPageWiz();
	CCaseEquivPageWiz(wxWizard* parent); // constructor
	virtual ~CCaseEquivPageWiz(void); // destructor // whm make all destructors virtual

	//enum { IDD = IDD_LOWER_TO_UPPER_EQUIVALENCES };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxScrolledWindow* m_scrolledWindow;

	/// an instance of the CCaseEquivPageCommon class for use in CCaseEquivPageWiz
	CCaseEquivPageCommon casePgCommon;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// implement wxWizardPage functions
	void OnWizardPageChanging(wxWizardEvent& event);
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	virtual wxWizardPage *GetPrev() const;
	virtual wxWizardPage *GetNext() const;

	void OnBnClickedClearSrcList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcCopyToNext(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcCopyToGloss(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedClearTgtList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedTgtSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedTgtCopyToNext(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedClearGlossList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedGlossSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedGlossCopyToNext(wxCommandEvent& WXUNUSED(event));

	void OnBnCheckedSrcHasCaps(wxCommandEvent& WXUNUSED(event)); // added by whm 11Aug04
	void OnBnCheckedUseAutoCaps(wxCommandEvent& WXUNUSED(event)); // added by whm 11Aug04


private:
	// class attributes

    DECLARE_DYNAMIC_CLASS( CCaseEquivPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CCaseEquivPagePrefs class creates a page for the Case tab in the Edit Preferences property sheet
/// which allows the user to change the case equivalences used for the document and/or project. 
/// The interface resources for the page/panel are defined in CaseEquivDlgFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CCaseEquivPagePrefs is derived from wxPanel.
class CCaseEquivPagePrefs : public wxPanel
{
public:
	CCaseEquivPagePrefs();
	CCaseEquivPagePrefs(wxWindow* parent); // constructor
	virtual ~CCaseEquivPagePrefs(void); // destructor // whm make all destructors virtual
   
	//enum { IDD = IDD_LOWER_TO_UPPER_EQUIVALENCES };
	
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CCaseEquivPageCommon class for use in CCaseEquivPagePrefs
	CCaseEquivPageCommon casePgCommon;
	
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	
	// function unique to EditPreferencesDlg panel
	void OnOK(wxCommandEvent& event);

	void OnBnClickedClearSrcList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcCopyToNext(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcCopyToGloss(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedClearTgtList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedTgtSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedTgtCopyToNext(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedClearGlossList(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedGlossSetEnglish(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedGlossCopyToNext(wxCommandEvent& WXUNUSED(event));

	void OnBnCheckedSrcHasCaps(wxCommandEvent& WXUNUSED(event)); // added by whm 11Aug04
	void OnBnCheckedUseAutoCaps(wxCommandEvent& WXUNUSED(event)); // added by whm 11Aug04

private:
	// class attributes

    DECLARE_DYNAMIC_CLASS( CCaseEquivPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* LowerToUpperCaseEquivPage_h */
