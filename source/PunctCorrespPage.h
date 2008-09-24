/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PunctCorrespPage.h
/// \author			Bill Martin
/// \date_created	8 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CPunctCorrespPageWiz and CPunctCorrespPagePrefs classes.
/// It also defines a CPunctCorrespPageCommon class that
/// contains the common elements and methods of the two above classes.
/// These classes create a wizard page in the StartWorkingWizard and/or a panel in the
/// EditPreferencesDlg that allow the user to edit the punctuation correspondences for a 
/// project. 
/// The interface resources for CPunctCorrespPageWiz and CPunctCorrespPagePrefs are 
/// defined in PunctCorrespPageFunc() which was developed and is maintained by wxDesigner.
/// \derivation		CPunctCorrespPageWiz is derived from wxWizardPage, CPunctCorrespPagePrefs from wxPanel and CPunctCorrespPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef PunctCorrespPage_h
#define PunctCorrespPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PunctCorrespPage.h"
#endif

#include "AdaptitConstants.h"

// forward declarations

/// The CPunctCorrespPageCommon class contains data and methods which are 
/// common to the CPunctCorrespPageWiz and CPunctCorrespPagePrefs classes.
/// \derivation The CPunctCorrespPageCommon class is derived from wxPanel
class CPunctCorrespPageCommon : public wxPanel
// CPunctCorrespPageCommon needs to be derived from wxPanel in order to use built-in 
// functions like FindWindowById()
{
public:
	// The following variables are common to both CFontPageWiz and CFontPagePrefs
	wxSizer* pPunctCorrespPageSizer;

	wxTextCtrl*	m_editTgtPunct[MAXPUNCTPAIRS]; // MFC vers had 24, now 26
	wxTextCtrl*	m_editSrcPunct[MAXPUNCTPAIRS]; // MFC vers had 24, now 26
	wxString	m_srcPunctStr[MAXPUNCTPAIRS]; // MFC vers had 24, now 26
	wxString	m_tgtPunctStr[MAXPUNCTPAIRS]; // MFC vers had 24, now 26
	wxTextCtrl*	m_editSrcTwoPunct[MAXTWOPUNCTPAIRS]; // MFC vers had 8, now 10
	wxTextCtrl*	m_editTgtTwoPunct[MAXTWOPUNCTPAIRS]; // MFC vers had 8, now 10
	wxString	m_srcTwoPunctStr[MAXTWOPUNCTPAIRS]; // MFC vers had 8, now 10
	wxString	m_tgtTwoPunctStr[MAXTWOPUNCTPAIRS]; // MFC vers had 8, now 10

	wxButton*	pToggleUnnnnBtn;
	wxTextCtrl* pTextCtrlAsStaticText;

	wxString strSavePhraseBox;
	int activeSequNum;

	// The following stuff is for version 2.3.0 and onwards; m_punctuation[0] and [1]
	// are set entirely from the contents of the punctuation correspondences defined in
	// this dialog when the user dismisses the dialog. If the user cancells, the earlier
	// contents are retained.
	wxString m_srcPunctuation; // to accept what will be placed in m_punctuation[0]
	wxString m_tgtPunctuation; // to accept what will be placed in m_punctuation[1]
	wxString m_punctuationBeforeEdit[2]; // whm moved here from the Doc where it was called m_savePunctuation[2]
#ifdef _UNICODE
	bool m_bShowingChars;
	void OnBnClickedToggleUnnnn(wxCommandEvent& WXUNUSED(event));
#endif

public:
	// The following functions are common to both CPunctCorrespPageWiz and CPunctCorrespPagePrefs
	void DoSetDataAndPointers();
	void DoInit();

	void SetupForGlyphs();
	void PopulateWithGlyphs();
	void UpdateAppValues(bool bFromGlyphs);
	void GetPunctuationSets();
	void GetDataFromEdits();
	void StoreDataIntoEdits();
#ifdef _UNICODE
	wxString MakeUNNNN(wxString& chStr);
	wxString UnMakeUNNNN(wxString& nnnnStr);
	void SetupForUnicodeNumbers();
	void PopulateWithUnicodeNumbers();
	int HexToInt(const wxChar hexDigit);
	bool ExtractSubstrings(wxString& dataStr,wxString& s1,wxString& s2);
#endif
};

/// The CPunctCorrespPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the punctuation correspondences settings used for the project during 
/// initial setup.
/// The interface resources for the page/panel are defined in PunctCorrespPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CPunctCorrespPageWiz is derived from wxWizardPage.
class CPunctCorrespPageWiz : public wxWizardPage
{
public:
	CPunctCorrespPageWiz();
	CPunctCorrespPageWiz(wxWizard* parent); // constructor
	virtual ~CPunctCorrespPageWiz(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_DLG_PUNCT_MAP };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();
	
	/// an instance of the CPunctCorrespPageCommon class for use in CPunctCorrespPageWiz
	CPunctCorrespPageCommon punctPgCommon;

public:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event)); // needs to be public because it's called from the App

	// functions unique to wxWizardPage
	void OnWizardPageChanging(wxWizardEvent& event); // make it public
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

private:
	// class attributes

    DECLARE_DYNAMIC_CLASS( CPunctCorrespPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CPunctCorrespPagePrefs class creates a page for the Punctuation tab in the Edit Preferences property sheet
/// which allows the user to change the punctuation correspondences used for the document and/or project. 
/// The interface resources for the page/panel are defined in PunctCorrespPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CPunctCorrespPagePrefs is derived from wxPanel.
class CPunctCorrespPagePrefs : public wxPanel
{
public:
	CPunctCorrespPagePrefs();
	CPunctCorrespPagePrefs(wxWindow* parent); // constructor
	virtual ~CPunctCorrespPagePrefs(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_DLG_PUNCT_MAP };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CPunctCorrespPageCommon class for use in CPunctCorrespPagePrefs
	CPunctCorrespPageCommon punctPgCommon;

public:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event)); // needs to be public because it's called from the App
	
	// function unique to EditPreferencesDlg panel
	void OnOK(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes

    DECLARE_DYNAMIC_CLASS( CPunctCorrespPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* PunctCorrespPage_h */
