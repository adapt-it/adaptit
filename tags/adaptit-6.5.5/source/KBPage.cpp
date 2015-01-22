/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBPage.cpp
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \rcs_id $Id$
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
/// BEW 25Jul12, added support for free translation language name and language code
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
#include "LanguageCodesDlg.h"

/// This global is defined in Adapt_ItView.cpp
extern bool gbAdaptBeforeGloss;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbLegacySourceTextCopy;	// default is legacy behaviour, to copy the source text (unless
									// the project config file establishes the FALSE value instead)

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

IMPLEMENT_DYNAMIC_CLASS( CKBPage, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CKBPage, wxPanel)
	EVT_INIT_DIALOG(CKBPage::InitDialog)
	EVT_CHECKBOX(IDC_CHECK_KB_BACKUP, CKBPage::OnCheckKbBackup)
	EVT_CHECKBOX(IDC_CHECK_BAKUP_DOC, CKBPage::OnCheckBakupDoc)
	EVT_RADIOBUTTON(IDC_RADIO_ADAPT_BEFORE_GLOSS, CKBPage::OnBnClickedRadioAdaptBeforeGloss)
	EVT_RADIOBUTTON(IDC_RADIO_GLOSS_BEFORE_ADAPT, CKBPage::OnBnClickedRadioGlossBeforeAdapt)
	EVT_RADIOBUTTON(IDC_RADIO_COPY_SRC_WORD_DELIM, CKBPage::OnBnClickedRadioCopySrcDelim)
	EVT_RADIOBUTTON(IDC_RADIO_USE_ONLY_LATIN_SPACE, CKBPage::OnBnClickedRadioUseLatinSpace)
	EVT_BUTTON(ID_BUTTON_LOOKUP_CODES, CKBPage::OnBtnLookupCodes) // whm added 10May10
END_EVENT_TABLE()


CKBPage::CKBPage()
{
}

CKBPage::CKBPage(wxWindow* parent) // dialog constructor
{
	Create( parent );

	pApp = (CAdapt_ItApp*)&wxGetApp();


	tempDisableAutoKBBackups = FALSE;
	tempBackupDocument = FALSE;
	tempAdaptBeforeGloss = TRUE;
	tempNotLegacySourceTextCopy = FALSE;
	tempSrcName = _T("");
	tempTgtName = _T("");
	tempGlsName = _T("");
	tempFreeTransName = _T("");
	tempSrcLangCode = _T("");
	tempTgtLangCode = _T("");
	tempGlsLangCode = _T("");
	tempFreeTransLangCode = _T("");

	// use wxGenericValidator for simple dialog data transfer
	m_pEditSrcName = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_NAME);
	wxASSERT(m_pEditSrcName != NULL);

	m_pEditTgtName = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_NAME);
	wxASSERT(m_pEditTgtName != NULL);

	m_pEditGlsName = (wxTextCtrl*)FindWindowById(IDC_EDIT_GLS_NAME);
	wxASSERT(m_pEditGlsName != NULL);

	m_pEditFreeTransName = (wxTextCtrl*)FindWindowById(ID_EDIT_FRTR_NAME);
	wxASSERT(m_pEditGlsName != NULL);

	m_pCheckDisableAutoBkups = (wxCheckBox*)FindWindowById(IDC_CHECK_KB_BACKUP);
	wxASSERT(m_pCheckDisableAutoBkups != NULL);

	m_pCheckBkupWhenClosing = (wxCheckBox*)FindWindowById(IDC_CHECK_BAKUP_DOC);
	wxASSERT(m_pCheckBkupWhenClosing != NULL);

	pSrcLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_SOURCE_LANG_CODE); // whm added 10May10
	wxASSERT(pSrcLangCodeBox != NULL);

	pTgtLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_TARGET_LANG_CODE); // whm added 10May10
	wxASSERT(pTgtLangCodeBox != NULL);

	pGlsLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_GLOSS_LANG_CODE); // whm added 5Dec11
	wxASSERT(pGlsLangCodeBox != NULL);

	pFreeTransLangCodeBox = (wxTextCtrl*)FindWindowById(ID_EDIT_FRTR_LANG_CODE); // whm added 5Dec11
	wxASSERT(pGlsLangCodeBox != NULL);

	pButtonLookupCodes = (wxButton*)FindWindowById(ID_BUTTON_LOOKUP_CODES); // whm added 10May10
	wxASSERT(pButtonLookupCodes != NULL);

	// get the button pointers
	pRadioAdaptBeforeGloss = (wxRadioButton*)FindWindowById(IDC_RADIO_ADAPT_BEFORE_GLOSS);
	wxASSERT(pRadioAdaptBeforeGloss != NULL);
	pRadioGlossBeforeAdapt = (wxRadioButton*)FindWindowById(IDC_RADIO_GLOSS_BEFORE_ADAPT);
	wxASSERT(pRadioGlossBeforeAdapt != NULL);

	pRadioUseSrcWordBreak = (wxRadioButton*)FindWindowById(IDC_RADIO_COPY_SRC_WORD_DELIM);
	wxASSERT(pRadioUseSrcWordBreak != NULL);
	pRadioUseLatinSpace = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_ONLY_LATIN_SPACE);
	wxASSERT(pRadioUseLatinSpace != NULL);

	m_pCheckLegacySourceTextCopy = (wxCheckBox*)FindWindowById(IDC_CHECK_LEGACY_SRC_TEXT_COPY);
	//m_pCheckLegacySourceTextCopy->SetValidator(wxGenericValidator(&tempNotLegacySourceTextCopy));
	wxASSERT(m_pCheckLegacySourceTextCopy != NULL);

	pTextCtrlAsStaticTextBackupsKB = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_BACKUPS_AND_KB_PAGE);
	wxASSERT(pTextCtrlAsStaticTextBackupsKB != NULL);
	// Make the wxTextCtrl that is displaying static text have window background color
	wxColor backgrndColor = this->GetBackgroundColour();
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

void CKBPage::OnBtnLookupCodes(wxCommandEvent& WXUNUSED(event)) // whm added 10May10
{
	// Call up CLanguageCodesDlg here so the user can enter language codes for
	// the source and target languages which are needed for the LIFT XML lang attribute of 
	// the <form lang="xxx"> tags (where xxx is a 3-letter ISO639-3 language/Ethnologue code)
	CLanguageCodesDlg lcDlg(this); // make the CKBPage the parent in this case
	lcDlg.Center();
	// initialize the language code edit boxes with the values currently in
	// the KBPage's edit boxes (which InitDialog initialized to the current values 
	// on the App, or which the user manually edited before pressing the
	// Lookup Codes button).
	tempSrcLangCode = pSrcLangCodeBox->GetValue();
	tempTgtLangCode = pTgtLangCodeBox->GetValue();
	tempGlsLangCode = pGlsLangCodeBox->GetValue();
	tempFreeTransLangCode = pFreeTransLangCodeBox->GetValue();
	lcDlg.m_sourceLangCode = tempSrcLangCode;
	lcDlg.m_targetLangCode = tempTgtLangCode;
	lcDlg.m_glossLangCode = tempGlsLangCode;
	lcDlg.m_freeTransLangCode = tempFreeTransLangCode;
	int returnValue = lcDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		// user cancelled
		return;
	}
	// user pressed OK so update the temp variables and the edit boxes
	tempSrcLangCode = lcDlg.m_sourceLangCode;
	pSrcLangCodeBox->SetValue(tempSrcLangCode);

	tempTgtLangCode = lcDlg.m_targetLangCode;
	pTgtLangCodeBox->SetValue(tempTgtLangCode);

	if (!lcDlg.m_glossLangCode.IsEmpty())
	{
		tempGlsLangCode = lcDlg.m_glossLangCode;
		pGlsLangCodeBox->SetValue(tempGlsLangCode);
	}
	if (!lcDlg.m_freeTransLangCode.IsEmpty())
	{
		tempFreeTransLangCode = lcDlg.m_freeTransLangCode;
		pFreeTransLangCodeBox->SetValue(tempFreeTransLangCode);
	}

	// update the language names as well, but do so only for gloss or free translation
	// being changed -- we require change of the name of the source language or target
	// language name to be done manually in the parent dialog (so as to permit more
	// variety of src and/or tgt language names than the iso639-3 standard supports)
	if (!lcDlg.m_glossesLangName.IsEmpty())
	{
		m_pEditGlsName->SetValue(lcDlg.m_glossesLangName);
	}
	if (!lcDlg.m_freeTransLangName.IsEmpty())
	{
		m_pEditFreeTransName->SetValue(lcDlg.m_freeTransLangName);
	}
}


void CKBPage::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Notes: Any changes made to OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CKBPage::OnWizardPageChanging() 
	// is called.
	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling CKBPage::OnOK().
	// BEW 25Jul12 -- are the above comments now no longer relevant for the wx version??

	// User pressed OK so assume user wants to store the dialog's temp... values.
	// put the source & target language names in storage on the App, etc
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
			pApp->SaveKB(FALSE, TRUE); // don't do backup
		}
	}
	// get any final edits of lang codes from edit boxes
	tempSrcLangCode = pSrcLangCodeBox->GetValue();
	tempTgtLangCode = pTgtLangCodeBox->GetValue();
	tempGlsLangCode = pGlsLangCodeBox->GetValue();
	tempFreeTransLangCode = pFreeTransLangCodeBox->GetValue();
	// update the lang codes values held on the App
	pApp->m_sourceLanguageCode = tempSrcLangCode;
	pApp->m_targetLanguageCode = tempTgtLangCode;
	pApp->m_glossesLanguageCode = tempGlsLangCode;
	pApp->m_freeTransLanguageCode = tempFreeTransLangCode;

	// BEW added 23July12, if the gloss or free translation language name is different,
	// then update the app's m_glossesName or m_freeTransName
	tempGlsName = m_pEditGlsName->GetValue();
	if (!tempGlsName.IsEmpty() && strSaveGlsName != tempGlsName)
	{
		pApp->m_glossesName = tempGlsName; // Prefs will now display it when reopened,
				// and it will be saved to basic and project config files
	}
	tempFreeTransName = m_pEditFreeTransName->GetValue();
	if (!tempFreeTransName.IsEmpty() && strSaveFreeTransName != tempFreeTransName)
	{
		pApp->m_freeTransName = tempFreeTransName; // Prefs will now display it when reopened,
				// and it will be saved to basic and project config files
	}

	// BEW added 21Jul14 support for new flag - commit to its value as set
	// currently
	pApp->m_bUseSrcWordBreak = bTempUseSrcWordBreak;
}

void CKBPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// initialize our local temp variables from those on the App
	tempDisableAutoKBBackups = !pApp->m_bAutoBackupKB;
	tempBackupDocument = pApp->m_bBackupDocument;
	tempAdaptBeforeGloss = gbAdaptBeforeGloss;
	tempNotLegacySourceTextCopy = !gbLegacySourceTextCopy;
	tempSrcName = pApp->m_sourceName;
	tempTgtName = pApp->m_targetName;
	tempGlsName = pApp->m_glossesName; // Bill & I added 6Dec11
	tempFreeTransName = pApp->m_freeTransName; // BEW added 25Jul12
	tempSrcLangCode = pApp->m_sourceLanguageCode;
	tempTgtLangCode = pApp->m_targetLanguageCode;
	tempGlsLangCode = pApp->m_glossesLanguageCode;
	tempFreeTransLangCode = pApp->m_freeTransLanguageCode;
	// BEW added 21Jul14, the next two
	bTempUseSrcWordBreak = pApp->m_bUseSrcWordBreak;

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
	m_pEditGlsName->SetValue(tempGlsName);
	m_pEditFreeTransName->SetValue(tempFreeTransName);
	pSrcLangCodeBox->SetValue(tempSrcLangCode);
	pTgtLangCodeBox->SetValue(tempTgtLangCode);
	pGlsLangCodeBox->SetValue(tempGlsLangCode);
	pFreeTransLangCodeBox->SetValue(tempFreeTransLangCode);

	// save names to check for any changes made by user
	strSaveSrcName = pApp->m_sourceName;
	strSaveTgtName = pApp->m_targetName;
	strSaveGlsName = pApp->m_glossesName;
	strSaveFreeTransName = pApp->m_freeTransName;

	// Since most users won't likely want any particular setting in this
	// panel, we won't set focus to any particular control.

	// make the fonts show user's desired point size in the dialog
	CopyFontBaseProperties(pApp->m_pSourceFont,pApp->m_pDlgSrcFont);
	pApp->m_pDlgSrcFont->SetPointSize(pApp->m_dialogFontSize);
	m_pEditSrcName->SetFont(*pApp->m_pDlgSrcFont);

	CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pDlgTgtFont);	
	pApp->m_pDlgTgtFont->SetPointSize(pApp->m_dialogFontSize);
	m_pEditTgtName->SetFont(*pApp->m_pDlgTgtFont);
	// and free translations use a same font as is used for the target text
	// (see, for example, DrawFreeTransStringsInDisplayRects() within FreeTrans.cpp)
	m_pEditFreeTransName->SetFont(*pApp->m_pDlgTgtFont);

	// gloss text uses the target text Font, unless the user has specified that glossing
	// should be done in the nav text font -- set the font in the edit box accordingly
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pDlgGlossFont);	
		pApp->m_pDlgGlossFont->SetPointSize(pApp->m_dialogFontSize);
		m_pEditGlsName->SetFont(*pApp->m_pDlgGlossFont);
	}
	else
	{
		CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pDlgGlossFont);	
		pApp->m_pDlgGlossFont->SetPointSize(pApp->m_dialogFontSize);
		m_pEditGlsName->SetFont(*pApp->m_pDlgGlossFont);
	}

	if (bTempUseSrcWordBreak)
	{
		wxCommandEvent dummy;
		OnBnClickedRadioCopySrcDelim(dummy);
	}
	else
	{
		wxCommandEvent dummy;
		OnBnClickedRadioUseLatinSpace(dummy);
	}

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
		m_pEditFreeTransName->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditTgtName->SetLayoutDirection(wxLayout_LeftToRight);
		m_pEditFreeTransName->SetLayoutDirection(wxLayout_LeftToRight);
	}

	// if nav text is to be shown RTL, and provided glossing uses the nav text font, then
	// the edit box for gloss language should be RTL too
	if (pApp->m_bNavTextRTL && gbGlossingUsesNavFont)
	{
		m_pEditGlsName->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditGlsName->SetLayoutDirection(wxLayout_LeftToRight);
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


void CKBPage::OnBnClickedRadioCopySrcDelim(wxCommandEvent& WXUNUSED(event))
{
	bTempUseSrcWordBreak = TRUE;
	pRadioUseSrcWordBreak->SetValue(TRUE);
	pRadioUseLatinSpace->SetValue(FALSE);
}

void CKBPage::OnBnClickedRadioUseLatinSpace(wxCommandEvent& WXUNUSED(event))
{
	bTempUseSrcWordBreak = FALSE;
	pRadioUseSrcWordBreak->SetValue(FALSE);
	pRadioUseLatinSpace->SetValue(TRUE);
}


