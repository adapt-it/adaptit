/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBPage.cpp
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CKBPage class. 
/// The CKBPage class creates a wxPanel that allows the 
/// user to define knowledge base backup options and to reenter the 
/// source and target language names should they become corrupted. 
/// The panel becomes a "Backups and KB" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the BackupsAndKBPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CKBPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBPage.h"
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
#include "KBPage.h"

#include "Adapt_It.h"
#include "KB.h"
#include "helpers.h"

/// This global is defined in Adapt_ItView.cpp
extern bool gbAdaptBeforeGloss;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbLegacySourceTextCopy;	// default is legacy behaviour, to copy the source text (unless
									// the project config file establishes the FALSE value instead)

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

IMPLEMENT_DYNAMIC_CLASS( CKBPage, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CKBPage, wxPanel)
	EVT_INIT_DIALOG(CKBPage::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_CHECKBOX(IDC_CHECK_KB_BACKUP, CKBPage::OnCheckKbBackup)
	EVT_CHECKBOX(IDC_CHECK_BAKUP_DOC, CKBPage::OnCheckBakupDoc)
	EVT_RADIOBUTTON(IDC_RADIO_ADAPT_BEFORE_GLOSS, CKBPage::OnBnClickedRadioAdaptBeforeGloss)
	EVT_RADIOBUTTON(IDC_RADIO_GLOSS_BEFORE_ADAPT, CKBPage::OnBnClickedRadioGlossBeforeAdapt)
END_EVENT_TABLE()


CKBPage::CKBPage()
{
}

CKBPage::CKBPage(wxWindow* parent) // dialog constructor
{
	Create( parent );

	tempDisableAutoKBBackups = FALSE;
	tempBackupDocument = FALSE;
	tempAdaptBeforeGloss = TRUE;
	tempNotLegacySourceTextCopy = FALSE;
	tempSrcName = _T("");
	tempTgtName = _T("");

	// use wxGenericValidator for simple dialog data transfer
	m_pEditSrcName = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_NAME);
	wxASSERT(m_pEditSrcName != NULL);

	m_pEditTgtName = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_NAME);
	wxASSERT(m_pEditTgtName != NULL);

	m_pCheckDisableAutoBkups = (wxCheckBox*)FindWindowById(IDC_CHECK_KB_BACKUP);
	wxASSERT(m_pCheckDisableAutoBkups != NULL);

	m_pCheckBkupWhenClosing = (wxCheckBox*)FindWindowById(IDC_CHECK_BAKUP_DOC);
	wxASSERT(m_pCheckBkupWhenClosing != NULL);

	// get the button pointers
	pRadioAdaptBeforeGloss = (wxRadioButton*)FindWindowById(IDC_RADIO_ADAPT_BEFORE_GLOSS);
	wxASSERT(pRadioAdaptBeforeGloss != NULL);
	pRadioGlossBeforeAdapt = (wxRadioButton*)FindWindowById(IDC_RADIO_GLOSS_BEFORE_ADAPT);
	wxASSERT(pRadioGlossBeforeAdapt != NULL);

	m_pCheckLegacySourceTextCopy = (wxCheckBox*)FindWindowById(IDC_CHECK_LEGACY_SRC_TEXT_COPY);
	m_pCheckLegacySourceTextCopy->SetValidator(wxGenericValidator(&tempNotLegacySourceTextCopy));
	wxASSERT(m_pCheckLegacySourceTextCopy != NULL);
	
	pTextCtrlAsStaticTextBackupsKB = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_BACKUPS_AND_KB_PAGE);
	wxASSERT(pTextCtrlAsStaticTextBackupsKB != NULL);
	// Make the wxTextCtrl that is displaying static text have window background color
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticTextBackupsKB->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTextBackupsKB->SetBackgroundColour(gpApp->sysColorBtnFace);
}

CKBPage::~CKBPage() // destructor
{
}

bool CKBPage::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CKBPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pKBPageSizer = BackupsAndKBPageFunc(this, TRUE, TRUE);
}

void CKBPage::OnCheckKbBackup(wxCommandEvent& WXUNUSED(event)) 
{
	if (tempDisableAutoKBBackups)
		tempDisableAutoKBBackups = FALSE;
	else
		tempDisableAutoKBBackups = TRUE;
	m_pCheckDisableAutoBkups->SetValue(tempDisableAutoKBBackups);
	
}

void CKBPage::OnCheckBakupDoc(wxCommandEvent& WXUNUSED(event)) 
{
	if (tempBackupDocument)
		tempBackupDocument = FALSE;
	else
		tempBackupDocument = TRUE;
	m_pCheckBkupWhenClosing->SetValue(tempBackupDocument);
	
}

// MFC's OnSetActive() has no direct equivalent in wxWidgets. 
// It would not be needed in any case since in our
// design InitDialog is moved to public and called once 
// in the App where the wizard pages are constructed before 
// wizard itself starts.

// Prevent leaving the page if either the source or target language
// names are left blank
// KBPage not used in any wizards, only as tab in notebook dialogs
//void CKBPage::OnWizardPageChanging(wxWizardEvent& event)
//{
//}

void CKBPage::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Notes: Any changes made to OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CKBPage::OnWizardPageChanging() 
	// is called.
	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling CKBPage::OnOK().

	// User pressed OK so assume user wants to store the dialog's temp... values.
	// put the source & target language names in storage on the App
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// get the auto backup flag's value, etc
	pApp->m_bAutoBackupKB = !tempDisableAutoKBBackups;
	pApp->m_bBackupDocument = tempBackupDocument;
	
	gbAdaptBeforeGloss = tempAdaptBeforeGloss; // get the flag value for
		// vertical edit order; whether adaptations updating precedes or follows
		// glosses updating (this setting is preserved in the project config file)
	
	// determine what the Copy of the source text when pile has no adaptation, or gloss,
	// should do in gloss mode, or adaptations mode, respectively (BEW added 16July08)
	gbLegacySourceTextCopy = !tempNotLegacySourceTextCopy;

	if (strSaveSrcName != tempSrcName || strSaveTgtName != tempTgtName)
	{
		// user updated the names, so fix the KB saved copies accordingly (this can
		// be done whether glossing is on or not)
		if (pApp->m_pKB != NULL)
		{
			pApp->m_sourceName = tempSrcName;
			pApp->m_targetName = tempTgtName;
			pApp->m_pKB->m_sourceLanguageName = tempSrcName;
			pApp->m_pKB->m_targetLanguageName = tempTgtName;
			pApp->SaveKB(FALSE); // don't do backup
		}
	}
}

void CKBPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// initialize our local temp variables from those on the App
	tempDisableAutoKBBackups = !pApp->m_bAutoBackupKB;
	tempBackupDocument = pApp->m_bBackupDocument;
	tempAdaptBeforeGloss = gbAdaptBeforeGloss;
	tempNotLegacySourceTextCopy = !gbLegacySourceTextCopy;
	tempSrcName = pApp->m_sourceName;
	tempTgtName = pApp->m_targetName;

	m_pCheckDisableAutoBkups->SetValue(tempDisableAutoKBBackups);
	m_pCheckBkupWhenClosing->SetValue(tempBackupDocument);
	// initialize the buttons to whatever value is in tempAdaptBeforeGloss
	if (tempAdaptBeforeGloss)
	{
		pRadioAdaptBeforeGloss->SetValue(TRUE);
		pRadioGlossBeforeAdapt->SetValue(FALSE);
	}
	else
	{
		pRadioAdaptBeforeGloss->SetValue(FALSE);
		pRadioGlossBeforeAdapt->SetValue(TRUE);
	}
	m_pCheckLegacySourceTextCopy->SetValue(tempNotLegacySourceTextCopy);
	m_pEditSrcName->SetValue(tempSrcName);
	m_pEditTgtName->SetValue(tempTgtName);

	// save names to check for any changes made by user
	strSaveSrcName = pApp->m_sourceName;
	strSaveTgtName = pApp->m_targetName;

	// Since most users won't likely want any particular setting in this
	// panel, we won't set focus to any particular control.

	// make the fonts show user's desired point size in the dialog
	CopyFontBaseProperties(pApp->m_pSourceFont,pApp->m_pDlgSrcFont);
	pApp->m_pDlgSrcFont->SetPointSize(pApp->m_dialogFontSize);
	m_pEditSrcName->SetFont(*pApp->m_pDlgSrcFont);

	CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pDlgTgtFont);	
	pApp->m_pDlgTgtFont->SetPointSize(pApp->m_dialogFontSize);
	m_pEditTgtName->SetFont(*pApp->m_pDlgTgtFont);
	
#ifdef _RTL_FLAGS
	if (pApp->m_bSrcRTL)
	{
		m_pEditSrcName->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditSrcName->SetLayoutDirection(wxLayout_LeftToRight);
	}

	if (pApp->m_bTgtRTL)
	{
		m_pEditTgtName->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditTgtName->SetLayoutDirection(wxLayout_LeftToRight);
	}
#endif
}


void CKBPage::OnBnClickedRadioAdaptBeforeGloss(wxCommandEvent& WXUNUSED(event))
{
	// make the first radio button be checked (standard order, adaptations first, then glosses)
	tempAdaptBeforeGloss = TRUE;
	pRadioAdaptBeforeGloss->SetValue(TRUE);
	pRadioGlossBeforeAdapt->SetValue(FALSE);
}

void CKBPage::OnBnClickedRadioGlossBeforeAdapt(wxCommandEvent& WXUNUSED(event))
{
	// make the second radio button be checked (alternate order, glosses first, then adaptations)
	tempAdaptBeforeGloss = FALSE;
	pRadioAdaptBeforeGloss->SetValue(FALSE);
	pRadioGlossBeforeAdapt->SetValue(TRUE);
}



