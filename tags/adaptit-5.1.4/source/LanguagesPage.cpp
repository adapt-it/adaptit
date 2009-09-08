/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguagesPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
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

#include "Adapt_It.h"
#include "KB.h"
#include "Adapt_ItView.h"
#include "DocPage.h"
#include "ProjectPage.h"
#include "FontPage.h"

// This global is defined in Adapt_It.cpp.
//extern wxWizard* pStartWorkingWizard;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar;

/// This global is defined in Adapt_It.cpp.
extern bool	  gbSfmOnlyAfterNewlines;

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CFontPageWiz* pFontPageWiz;

//extern CDocPage* pDocPage;

extern LangInfo langsKnownToWX[]; // LangInfo defined in Adapt_It.h

IMPLEMENT_DYNAMIC_CLASS( CLanguagesPage, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CLanguagesPage, wxWizardPage)
	EVT_INIT_DIALOG(CLanguagesPage::InitDialog)// not strictly necessary for dialogs based on wxDialog
    EVT_WIZARD_PAGE_CHANGING(-1, CLanguagesPage::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CLanguagesPage::OnWizardCancel)
	//EVT_BUTTON(ID_BUTTON_CHANGE_INTERFACE_LANGUAGE, CLanguagesPage::OnUILanguage)
END_EVENT_TABLE()

CLanguagesPage::CLanguagesPage()
{
}

CLanguagesPage::CLanguagesPage(wxWizard* parent) // dialog constructor
{
	Create( parent );

	// Since wizLangaugesPage appears in a notebook tab when the user
	// has indicated in Start Here that he/she wants to create a new
	// project, we will initially present empty names to user. Presenting
	// names from a previously open project might be a convenience to some
	// but would be potentially confusing to a novice user.
	tempSourceName = gpApp->m_sourceName; //_T("");
	tempTargetName = gpApp->m_targetName; //_T("");
	tempSfmEscCharStr = gSFescapechar;
	tempSfmOnlyAfterNewlines = gbSfmOnlyAfterNewlines;

	// use wxGenericValidator for simple dialog data transfer
	pSrcBox = (wxTextCtrl*)FindWindowById(IDC_SOURCE_LANGUAGE);
	wxASSERT(pSrcBox != NULL);

	pTgtBox = (wxTextCtrl*)FindWindowById(IDC_TARGET_LANGUAGE);
	wxASSERT(pTgtBox != NULL);

	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticSFMsAlwasStNewLine = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_NL);
	wxASSERT(pTextCtrlAsStaticSFMsAlwasStNewLine != NULL);
	//pTextCtrlAsStaticSFMsAlwasStNewLine->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticSFMsAlwasStNewLine->SetBackgroundColour(gpApp->sysColorBtnFace);

	//pDefaultSystemLanguageBox = (wxTextCtrl*)FindWindowById(ID_TEXT_SYS_DEFAULT_LANGUAGE);
	//wxASSERT(pDefaultSystemLanguageBox != NULL);
	//pDefaultSystemLanguageBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	//pInterfaceLanguageBox = (wxTextCtrl*)FindWindowById(ID_TEXT_CURR_INTERFACE_LANGUAGE);
	//wxASSERT(pInterfaceLanguageBox != NULL);
	//pInterfaceLanguageBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	//pChangeInterfaceLangBtn = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_INTERFACE_LANGUAGE);
	//wxASSERT(pChangeInterfaceLangBtn != NULL);


	pSfmOnlyAfterNLCheckBox = (wxCheckBox*)FindWindowById(IDC_CHECK_SFM_AFTER_NEWLINES);
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
	pSrcBox->SetValue(tempSourceName);
	pSrcBox->SetFocus(); // start with focus on the Source edit box
	pTgtBox->SetValue(tempTargetName);
	// Note: tempSfmOnlyAfterNewLines is initialized in constructor above which happens only
	// once when the languages page is created in the App
	pSfmOnlyAfterNLCheckBox->SetValue(tempSfmOnlyAfterNewlines);

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
	
void CLanguagesPage::OnUILanguage(wxCommandEvent& WXUNUSED(event))
{
	// This is called when the Start Working Wizard is setting up a new project.
	gpApp->ChangeUILanguage();
}

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
			wxMessageBox(_("Sorry, the source language name cannot be left blank."), _T(""), wxICON_INFORMATION);
			pSrcBox->SetFocus();
			event.Veto();
			return;
		}
		if (pTgtBox->GetValue().IsEmpty())
		{
			// IDS_NULL_TARGET_NAME
			wxMessageBox(_("Sorry, the target language name cannot be left blank."), _T(""), wxICON_INFORMATION);
			pTgtBox->SetFocus();
			event.Veto();
			return;
		}

		// if we get here the source and language name edits have values
		// moving forward toward fontPage, assume user wants to
		// store the dialog's values.
		// put the source & target language names in storage on the App
		CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

		// Retrieve languages page member data from text controls and update the
		// App's values
		pApp->m_sourceName = pSrcBox->GetValue();
		pApp->m_targetName = pTgtBox->GetValue();
		//gSFescapechar = pSfmBox->GetValue().GetChar(0);
		gbSfmOnlyAfterNewlines = pSfmOnlyAfterNLCheckBox->GetValue();
		
		// whm: The stuff below was in MFC's fontPage, but it should go here in the languagesPage.
		// set up the directories using the new names, plus a KB, for the new project
		// (all the error conditions abort (I think), so ignore returned value)
		// (since this is a new project, we don't need to worry here about making a backup KB, nor
		// need we do any integrity checking). However, in case the user moved back to the fonts page
		// after having used Next> (which would have created a stub knowledge base), we would have
		// the m_pKB member still non-null, so SetupDirectories() would fail. Hence if so, delete the
		// stub from memory & reset member of null, so the call will not fail.
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
		bool bDirectoriesOK;
		bDirectoriesOK = gpApp->SetupDirectories(); 
		// SetupDirectories does not set the CWD but does set m_curAdaptionsPath
		
		// have the name for the new project into the projectPage's listBox
		pProjectPage->m_pListBox->Append(gpApp->m_curProjectName);
		// stuff above was in MFC's fontPage

		//// from the wizard we need to call the docPage's InitDialog now that we have the
		//// language names to form a project name; The docPage's InitDialog calls docPage's
		//// OnSetActive which loads the listbox with the appropriate documents if they exist
		//wxInitDialogEvent idevent;
		//pDocPage->InitDialog(idevent);
		
		// Movement through wizard pages is sequential - the next page is the fontPage.
		// The pFontPageWiz's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pFontPageWiz->InitDialog(idevent);
	}
	else
	{
		// moving backward toward projectPage, so we may need to
		// undo some values
		CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

		pApp->m_sourceName = tempSourceName;
		pApp->m_targetName = tempTargetName;
	}
}


