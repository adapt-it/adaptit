/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			LanguagesPage.h
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CLanguagesPage class. 
/// The CLanguagesPage class creates a wizard panel that allows the user
/// to enter the names of the languages that Adapt It will use to create a new
/// project. The dialog also allows the user to specify whether sfm markers 
/// start on new lines. The interface resources for the CLanguagesPage are 
/// defined in LanguagesPageFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CLanguagesPage class is derived from wxWizardPage.
/////////////////////////////////////////////////////////////////////////////

#ifndef WizLanguagesPage_h
#define WizLanguagesPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "LanguagesPage.h"
#endif

/// The CLanguagesPage class creates a wizard panel that allows the user
/// to enter the names of the languages that Adapt It will use to create a new
/// project. The dialog also allows the user to specify whether sfm markers 
/// start on new lines. The interface resources for the CLanguagesPage are 
/// defined in LanguagesPageFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CLanguagesPage class is derived from wxWizardPage.
class CLanguagesPage : public  wxWizardPage
{
public:
	CLanguagesPage();
	CLanguagesPage(wxWizard* parent); // constructor
	virtual ~CLanguagesPage(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_DLG_LANGUAGES };
    
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxSizer*	pLangPageSizer;

	wxString	tempSourceName;
	wxString	tempTargetName;
	wxString	tempSfmEscCharStr;

	wxTextCtrl*	pSrcBox;
	wxTextCtrl*	pTgtBox;
	//wxTextCtrl* pDefaultSystemLanguageBox;
	//wxTextCtrl* pInterfaceLanguageBox;
	wxTextCtrl* pTextCtrlAsStaticSFMsAlwasStNewLine;
	wxCheckBox*	pSfmOnlyAfterNLCheckBox;
	//wxButton*	pChangeInterfaceLangBtn;
	bool		tempSfmOnlyAfterNewlines;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnUILanguage(wxCommandEvent& WXUNUSED(event));
    
	//// implement wxWizardPage functions
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	void OnWizardPageChanging(wxWizardEvent& event);
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CLanguagesPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* WizLanguagesPage_h */
