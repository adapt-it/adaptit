/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguagesPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CLanguagesPage class. 
/// The CLanguagesPage class creates a wizard panel that allows the user
/// to enter the names of the languages that Adapt It will use to create a new
/// project. The dialog also allows the user to specify whether sfm markers 
/// start on new lines. The interface resources for the CLanguagesPage are 
/// defined in LanguagesPageFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CLanguagesPage class is derived from wxWizardPage.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in LanguagesPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "LanguagesPage.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/wizard.h>
#include "LanguagesPage.h"

// whm 14Jun12 modified to #include <wx/fontdate.h> for wxWidgets 2.9.x and later
#if wxCHECK_VERSION(2,9,0)
#include <wx/fontdata.h>
#endif

#include "Adapt_It.h"
#include "KB.h"
#include "helpers.h"
#include "CollabUtilities.h"
#include "Adapt_ItView.h"
#include "DocPage.h"
#include "ProjectPage.h"
#include "FontPage.h"
#include "LanguageCodesDlg.h"

// This global is defined in Adapt_It.cpp.
//extern wxWizard* pStartWorkingWizard;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar;

// BEW 8Jun10, removed support for checkbox "Recognise standard format
// markers only following newlines"
// This global is defined in Adapt_It.cpp.
//extern bool	  gbSfmOnlyAfterNewlines;

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CFontPageWiz* pFontPageWiz;

//extern CDocPage* pDocPage;

extern LangInfo langsKnownToWX[]; // LangInfo defined in Adapt_It.h

IMPLEMENT_DYNAMIC_CLASS( CLanguagesPage, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CLanguagesPage, wxWizardPage)
	EVT_INIT_DIALOG(CLanguagesPage::InitDialog)
	EVT_TEXT(IDC_SOURCE_LANGUAGE,CLanguagesPage::OnEditSourceLanguageName)
	EVT_TEXT(IDC_TARGET_LANGUAGE,CLanguagesPage::OnEditTargetLanguageName)
	EVT_WIZARD_PAGE_CHANGING(-1, CLanguagesPage::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
	EVT_BUTTON(ID_BUTTON_LOOKUP_CODES, CLanguagesPage::OnBtnLookupCodes) // whm added 10May10
    EVT_WIZARD_CANCEL(-1, CLanguagesPage::OnWizardCancel)
END_EVENT_TABLE()

CLanguagesPage::CLanguagesPage()
{
}

CLanguagesPage::CLanguagesPage(wxWizard* parent) // dialog constructor
{
	Create( parent );

	// InitDialog uses the following temp variables to initialize the 
	// languagesPage's GUI controls.
	tempSourceName = gpApp->m_sourceName;
	tempTargetName = gpApp->m_targetName;
	tempSourceLangCode = gpApp->m_sourceLanguageCode; // whm added 10May10
	tempTargetLangCode = gpApp->m_targetLanguageCode; // whm added 10May10
	tempSfmEscCharStr = gSFescapechar;
	// BEW 8Jun10, removed support for checkbox "Recognise standard format
	// markers only following newlines"
	//tempSfmOnlyAfterNewlines = gbSfmOnlyAfterNewlines;

	// use wxGenericValidator for simple dialog data transfer
	pSrcBox = (wxTextCtrl*)FindWindowById(IDC_SOURCE_LANGUAGE);
	wxASSERT(pSrcBox != NULL);

	pTgtBox = (wxTextCtrl*)FindWindowById(IDC_TARGET_LANGUAGE);
	wxASSERT(pTgtBox != NULL);

	pSrcLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_SOURCE_LANG_CODE); // whm added 10May10
	wxASSERT(pSrcLangCodeBox != NULL);

	pTgtLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_TARGET_LANG_CODE); // whm added 10May10
	wxASSERT(pTgtLangCodeBox != NULL);

	pButtonLookupCodes = (wxButton*)FindWindowById(ID_BUTTON_LOOKUP_CODES); // whm added 10May10
	wxASSERT(pButtonLookupCodes != NULL);

	wxColor backgrndColor = this->GetBackgroundColour();
	// BEW 8Jun10, removed support for checkbox "Recognise standard format
	// markers only following newlines"
	//pTextCtrlAsStaticSFMsAlwasStNewLine = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_NL);
	//wxASSERT(pTextCtrlAsStaticSFMsAlwasStNewLine != NULL);
	//pTextCtrlAsStaticSFMsAlwasStNewLine->SetBackgroundColour(gpApp->sysColorBtnFace);

	// BEW 8Jun10, removed support for checkbox "Recognise standard format
	// markers only following newlines"
	//pSfmOnlyAfterNLCheckBox = (wxCheckBox*)FindWindowById(IDC_CHECK_SFM_AFTER_NEWLINES);
}

CLanguagesPage::~CLanguagesPage() // destructor
{
	
}

bool CLanguagesPage::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	// whm: If we are operating on a small screen resolution, the parent wxWizard will be
	// restricted in height so that it will fit within the screen. If our wxWizardPage is too large to
	// also fit within the restricted parent wizard, we want it to fit within that limit as well, and 
	// scroll if necessary so the user can still access the whole wxWizardPage. 
	//gpApp->FitWithScrolling((wxDialog*)this, m_scrolledWindow, parent->GetClientSize()); //GetSizer()->Fit(this);
	return TRUE;
}

void CLanguagesPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pLangPageSizer = LanguagesPageFunc(this, TRUE, TRUE);
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(pLangPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CLanguagesPage::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pProjectPage; 
}
wxWizardPage* CLanguagesPage::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pFontPageWiz;
}

void CLanguagesPage::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
	gpApp->LogUserAction(_T("In LanguagesPage: user Cancel from wizard"));
}
	
void CLanguagesPage::OnBtnLookupCodes(wxCommandEvent& WXUNUSED(event)) // whm added 10May10
{
	// Call up CLanguageCodesDlg here so the user can enter language codes for
	// the source and target languages which are needed for the LIFT XML lang attribute of 
	// the <form lang="xxx"> tags (where xxx is a 3-letter ISO639-3 language/Ethnologue code)
	// 
    // BEW additional comment of 25Jul12, for xhtml exports we support not just src and tgt
    // language codes, but also language codes for glosses language, and free translation
    // language - all four languages are independently settable. However, while all four
    // can be set by repeated invokations of the Lookup Codes button, in the CLanguagesPage
    // we are interested only in setting up a new Adapt It project, and for that task only
    // the source and target languages are relevant, and so we pick up and store only the
    // codes, if the user bothers to set them, for either or both of these languages. To
    // set codes for glosses language, and/or free translation language, go later on to the
    // Backups and Misc page of the Preferences -- settings made there are remembered, and
	// all four are saved to the basic and project configuration files - whether the
	// document is saved or not on closure.
    
	CLanguageCodesDlg lcDlg(this); // make the CLanguagesPage the parent in this case
	lcDlg.Center();

	// initialize the language code edit boxes with the values currently in
	// the LanguagePage's edit boxes (which InitDialog initialized to the current 
	// values on the App, or which the user manually edited before pressing the 
	// Lookup Codes button).
	lcDlg.m_sourceLangCode = pSrcLangCodeBox->GetValue();
	lcDlg.m_targetLangCode = pTgtLangCodeBox->GetValue();
	int returnValue = lcDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		// user cancelled
		return;
	}
	// transfer language codes to the edit box controls and the App's members
	pSrcLangCodeBox->SetValue(lcDlg.m_sourceLangCode);
	pTgtLangCodeBox->SetValue(lcDlg.m_targetLangCode);
	gpApp->m_sourceLanguageCode = lcDlg.m_sourceLangCode;
	gpApp->m_targetLanguageCode = lcDlg.m_targetLangCode;
}

// MFC's OnSetActive() has no direct equivalent in wxWidgets. 
// It would not be needed in any case since in our
// design InitDialog is moved to public and called once 
// in the App where the wizard pages are constructed before 
// wizard itself starts.

void CLanguagesPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// Load the temp member data into text controls.
	pSrcBox->ChangeValue(tempSourceName);
	pSrcBox->SetFocus(); // start with focus on the Source edit box
	pTgtBox->ChangeValue(tempTargetName);
	pSrcLangCodeBox->ChangeValue(tempSourceLangCode); // whm added 10May10
	pTgtLangCodeBox->ChangeValue(tempTargetLangCode); // whm added 10May10
	// BEW 8Jun10, removed support for checkbox "Recognise standard format
	// markers only following newlines"
	// Note: tempSfmOnlyAfterNewLines is initialized in constructor above which happens only
	// once when the languages page is created in the App
	//pSfmOnlyAfterNLCheckBox->SetValue(tempSfmOnlyAfterNewlines);

	//pDefaultSystemLanguageBox->SetValue(gpApp->m_languageInfo->Description);

	//if (langsKnownToWX[gpApp->currLocalizationInfo.curr_UI_Language].code == wxLANGUAGE_DEFAULT)
	//	pInterfaceLanguageBox->SetValue(_("(Use system default language)"));
	//else
	//	pInterfaceLanguageBox->SetValue(langsKnownToWX[gpApp->currLocalizationInfo.curr_UI_Language].fullName);

	// make the font show user's point size in the dialog
	// we are using the navtext encoding for all these, the dlg fonts are
	// created on the fly each time, so doesn't matter that we use m_pDlgSrcFont
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pSrcBox, pTgtBox,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pSrcBox, pTgtBox, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif
}

void CLanguagesPage::OnEditSourceLanguageName(wxCommandEvent& WXUNUSED(event)) // whm added 14May10
{
	// user is editing the Source Language Name which invalidates any code value
	// conatined within the Source Language Code edit box. Blank out the code string
	// from the Source Language Code edit box.
	tempSourceLangCode.Empty();
	if (!pSrcLangCodeBox->GetValue().IsEmpty())
		pSrcLangCodeBox->ChangeValue(_T(""));

}

void CLanguagesPage::OnEditTargetLanguageName(wxCommandEvent& WXUNUSED(event)) // whm added 14May10
{
	// user is editing the Target Language Name which invalidates any code value
	// conatined within the Target Language Code edit box. Blank out the code string
	// from the Target Language Code edit box.
	tempTargetLangCode.Empty();
	if (!pTgtLangCodeBox->GetValue().IsEmpty())
	pTgtLangCodeBox->ChangeValue(_T(""));
}
	
//void CLanguagesPage::OnUILanguage(wxCommandEvent& WXUNUSED(event))
//{
//	// This is called when the Start Working Wizard is setting up a new project.
//	gpApp->ChangeUILanguage();
//}

// Prevent leaving the page if either the source or target language
// names are left blank
void CLanguagesPage::OnWizardPageChanging(wxWizardEvent& event)
{
	// Can put any code that needs to execute regardless of whether
	// Next or Prev button was pushed here.

	// Determine which direction we're going and implement
	// the MFC equivalent of OnWizardNext() and OnWizardBack() here
	bool bMovingForward = event.GetDirection();

	// Notes: Any changes made to OnOK should also be made to 
	// OnWizardPageChanging below.
	// In DoStartWorkingWizard, CLanguagesPage::OnWizardPageChanging() 
	// is called.
	if (bMovingForward)
	{
		if (pSrcBox->GetValue().IsEmpty())
		{
			// IDS_NULL_SOURCE_NAME
			wxMessageBox(_("Sorry, the source language name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			pSrcBox->SetFocus();
			event.Veto();
			return;
		}
		if (pTgtBox->GetValue().IsEmpty())
		{
			// IDS_NULL_TARGET_NAME
			wxMessageBox(_("Sorry, the target language name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			pTgtBox->SetFocus();
			event.Veto();
			return;
		}

		gpApp->LogUserAction(_T("In LanguagesPage: Next selected"));
		// if we get here the source and language name edits have values
		// moving forward toward fontPage, assume user wants to
		// store the dialog's values.
		// put the source & target language names in storage on the App
		CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

		// Retrieve languages page member data from text controls and update the
		// App's values
		pApp->m_sourceName = pSrcBox->GetValue();
		pApp->m_targetName = pTgtBox->GetValue();
		// get any final edits of lang codes from edit boxes
		pApp->m_sourceLanguageCode = pSrcLangCodeBox->GetValue();
		pApp->m_targetLanguageCode = pTgtLangCodeBox->GetValue();
		//gSFescapechar = pSfmBox->GetValue().GetChar(0);
		// BEW 8Jun10, removed support for checkbox "Recognise standard format
		// markers only following newlines"
		//gbSfmOnlyAfterNewlines = pSfmOnlyAfterNLCheckBox->GetValue();
		
		// whm: The stuff below was in MFC's fontPage, but it should go here in the languagesPage.
		// set up the directories using the new names, plus a KB, for the new project
		// (all the error conditions abort (I think), so ignore returned value)
		// (since this is a new project, we don't need to worry here about making a backup KB, nor
		// need we do any integrity checking). However, in case the user moved back to the fonts page
		// after having used Next> (which would have created a stub knowledge base), we would have
		// the m_pKB member still non-null, so SetupDirectories() would fail. Hence if so, delete the
		// stub from memory & reset member of null, so the call will not fail.
		if (pApp->m_pKB != NULL || pApp->m_pGlossingKB != NULL)
		{
			UnloadKBs(pApp);
		}
		/*
		if (gpApp->m_pKB != NULL)
		{
			// we have moved back, so clear the stub etc.
			delete gpApp->m_pKB;
			gpApp->m_pKB = (CKB*)NULL;
		}
		if (gpApp->m_pGlossingKB != NULL) // whm added
		{
			// we have moved back, so clear the stub etc.
			delete gpApp->m_pGlossingKB;
			gpApp->m_pGlossingKB = (CKB*)NULL;
		}
		*/
		bool bDirectoriesOK;
		bDirectoriesOK = gpApp->SetupDirectories();  // also sets KB paths and loads KBs & Guesser
		// needs to succeed, so wxCHECK_RET() call should be used here
		if (!bDirectoriesOK)
			gpApp->LogUserAction(_T("In LanguagesPage SetupDirectories() failed"));
		wxCHECK_RET(bDirectoriesOK, _T("OnWizardPageChanging(): SetupDirectories() failed, line 367 in LanguagesPage.cpp, processing will continue & app may be in an unstable state. Save and shutdown would be wise."));
		// SetupDirectories does not set the CWD but does set m_curAdaptionsPath
		
		// have the name for the new project into the projectPage's listBox
		pProjectPage->m_pListBox->Append(gpApp->m_curProjectName);
		// stuff above was in MFC's fontPage

		// Movement through wizard pages is sequential - the next page is the fontPage.
		// The pFontPageWiz's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pFontPageWiz->InitDialog(idevent);
	}
	else
	{
		gpApp->LogUserAction(_T("In LanguagesPage: Back selected"));
		// moving backward toward projectPage, so we may need to
		// undo some values
		CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

		pApp->m_sourceName = tempSourceName;
		pApp->m_targetName = tempTargetName;
		// whm Note: The tempSourceLangCode and tempTargetLangCode temp variables
		// are not modified within the languagesPage wizard page. Therefore, we
		// can roll back the App's stored values to what they were before if the
		// user backs up from the languagesPage to the projectPage.
		pApp->m_sourceLanguageCode = tempSourceLangCode; // whm added 10May10
		pApp->m_targetLanguageCode = tempTargetLangCode; // whm added 10May10
	}
}


