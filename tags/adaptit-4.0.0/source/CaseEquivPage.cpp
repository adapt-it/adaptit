/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CaseEquivPage.cpp
/// \author			Bill Martin
/// \date_created	29 April 2004
/// \date_revised	15 January 2008
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
// Pending Implementation Items in CaseEquivPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CaseEquivPage.h"
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
#include "CaseEquivPage.h"
#include "USFMPage.h"
#include "PunctCorrespPage.h"
#include "Adapt_It.h"

// This global is defined in Adapt_It.cpp.
//extern wxWizard* pStartWorkingWizard;

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp.
extern CUSFMPageWiz* pUsfmPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CPunctCorrespPageWiz* pPunctCorrespPageWiz;

// for support of auto-capatalization

/// This global is defined in Adapt_It.cpp.
extern bool gbSrcHasUcAndLc;

/// This global is defined in Adapt_It.cpp.
extern bool	gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool	gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNonSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbMatchedKB_UCentry;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoSourceCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoTargetCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoGlossCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcUC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcUC;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// next four are for version 2.0 which includes the option of a 3rd line for glossing

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

void CCaseEquivPageCommon::DoSetDataAndPointers()
{
	m_strSrcEquivalences = _T("");
	m_strTgtEquivalences = _T("");
	m_strGlossEquivalences = _T("");

	// I'm not getting wxGenericValidator to work properly so we'll transfer data
	// to/from controls and string variables manually.
	m_pEditSrcEquivalences = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_CASE_EQUIVALENCES);
	m_pEditSrcEquivalences->SetValidator(wxGenericValidator(&m_strSrcEquivalences));

	m_pEditTgtEquivalences = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_CASE_EQUIVALENCES);
	m_pEditTgtEquivalences->SetValidator(wxGenericValidator(&m_strTgtEquivalences));

	m_pEditGlossEquivalences = (wxTextCtrl*)FindWindowById(IDC_EDIT_GLOSS_CASE_EQUIVALENCES);
	m_pEditGlossEquivalences->SetValidator(wxGenericValidator(&m_strGlossEquivalences));
}

void CCaseEquivPageCommon::DoInit()
{
	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrcEquivalences, NULL,
													NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pEditSrcEquivalences, NULL, 
													NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditTgtEquivalences, NULL,
													NULL, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditTgtEquivalences, NULL, 
													NULL, NULL, gpApp->m_pDlgTgtFont);
	#endif

	if (gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditGlossEquivalences, NULL,
														NULL, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditGlossEquivalences, NULL, 
														NULL, NULL, gpApp->m_pDlgGlossFont);
		#endif
	}
	else
	{
		// glossing uses target text's font & directionality
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditGlossEquivalences, NULL,
														NULL, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bTgtRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditGlossEquivalences, NULL, 
														NULL, NULL, gpApp->m_pDlgGlossFont);
		#endif
	}

	// fill the source multiline wxTextCtrl
	BuildListString(gpApp->m_srcLowerCaseChars,gpApp->m_srcUpperCaseChars,m_strSrcEquivalences);

	// fill the target multiline wxTextCtrl
	BuildListString(gpApp->m_tgtLowerCaseChars,gpApp->m_tgtUpperCaseChars,m_strTgtEquivalences);

	// fill the glossing multiline wxTextCtrl
	BuildListString(gpApp->m_glossLowerCaseChars,gpApp->m_glossUpperCaseChars,m_strGlossEquivalences);

	TransferDataToWindow(); // transfer data from variables to controls
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);

	// The flag called gbSrcHasUcAndLc stores the user's indication of whether the 
	// source uses upper and lower case or not. Make the checkbox reflect the
	// value stored in gbSrcHasUcAndLc.
	if (gbSrcHasUcAndLc)
	{
		// Initialize the box as checked
		wxCheckBox* pChkBox1 = (wxCheckBox*)FindWindowById(ID_CHECK_SOURCE_USES_CAPS);
		pChkBox1->SetValue(TRUE);
	}
	else 
	{
		// Initialize the box as unchecked
		wxCheckBox* pChkBox1 = (wxCheckBox*)FindWindowById(ID_CHECK_SOURCE_USES_CAPS);
		pChkBox1->SetValue(FALSE);
		// When gbSrcHasUcAndLc is FALSE, gbAutoCaps cannot be true, so insure
		// that gbAutoCaps is FALSE here (in case user monkeyed with config file
		// and changed gbAutoCaps to TRUE while gbSrcHasUcAndLc was false).
		gbAutoCaps = FALSE;
		// second checkbox should also be initialized to a hidden state
		wxCheckBox* pChkBox2 = (wxCheckBox*)FindWindowById(ID_CHECK_USE_AUTO_CAPS);
		pChkBox2->Hide();
	}

	if (gbAutoCaps)
	{
		// Autocaps is activated so make sure the second checkbox is checked
		// and the other dialog items are made visible
		wxCheckBox* pChkBox2 = (wxCheckBox*)FindWindowById(ID_CHECK_USE_AUTO_CAPS);
		pChkBox2->SetValue(TRUE);
		ToggleControlsVisibility(gbAutoCaps);
	}
	else
	{
		// Autocaps is NOT activated so make sure the second checkbox is unchecked
		// and the other dialog items are made invisible
		wxCheckBox* pChkBox2 = (wxCheckBox*)FindWindowById(ID_CHECK_USE_AUTO_CAPS);
		pChkBox2->SetValue(FALSE);
		ToggleControlsVisibility(gbAutoCaps);
	}
}

void CCaseEquivPageCommon::DoBnClickedClearSrcList()
{
	m_strSrcEquivalences.Empty();
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
}

void CCaseEquivPageCommon::DoBnClickedSrcSetEnglish()
{
	// we will use the app class's SetDefaultCaseEquivalences( ) so that we have just the
	// one source for the English defaults; so we have to preserve the app's member strings
	// across this operation
	wxString saveSrcLower = gpApp->m_srcLowerCaseChars;
	wxString saveSrcUpper = gpApp->m_srcUpperCaseChars;
	wxString saveTgtLower = gpApp->m_tgtLowerCaseChars;
	wxString saveTgtUpper = gpApp->m_tgtUpperCaseChars;
	wxString saveGlossLower = gpApp->m_glossLowerCaseChars;
	wxString saveGlossUpper = gpApp->m_glossUpperCaseChars;
	gpApp->SetDefaultCaseEquivalences();
	BuildListString(gpApp->m_srcLowerCaseChars,gpApp->m_srcUpperCaseChars,m_strSrcEquivalences);
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	gpApp->m_srcLowerCaseChars = saveSrcLower;
	gpApp->m_srcUpperCaseChars = saveSrcUpper;
	gpApp->m_tgtLowerCaseChars = saveTgtLower;
	gpApp->m_tgtUpperCaseChars = saveTgtUpper;
	gpApp->m_glossLowerCaseChars = saveGlossLower;
	gpApp->m_glossUpperCaseChars = saveGlossUpper;
}

void CCaseEquivPageCommon::DoBnClickedSrcCopyToNext()
{
	TransferDataFromWindow(); // transfer data from contols to string variables
	m_strSrcEquivalences = m_pEditSrcEquivalences->GetValue();
	m_strTgtEquivalences = m_pEditTgtEquivalences->GetValue();
	m_strGlossEquivalences = m_pEditGlossEquivalences->GetValue();
	m_strTgtEquivalences = m_strSrcEquivalences;
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
	// scroll to the top if not already at the top
	m_pEditTgtEquivalences->ShowPosition(0); // scroll to top
	// Note: wxTextCtrl has ShowPosition(long) which "makes the line 
	// containing the given position visible." TODO: check to see how
	// this works - does it scroll the line to the top or what???
	//if (nFirstVisible > 0)
	//	m_pEditTgtEquivalences.LineScroll(-nFirstVisible,0);
}

void CCaseEquivPageCommon::DoBnClickedSrcCopyToGloss()
{
	m_strSrcEquivalences = m_pEditSrcEquivalences->GetValue();
	m_strTgtEquivalences = m_pEditTgtEquivalences->GetValue();
	m_strGlossEquivalences = m_pEditGlossEquivalences->GetValue();

	m_strGlossEquivalences = m_strSrcEquivalences;

	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
	//int nFirstVisible = m_editGlossEquivalences.GetFirstVisibleLine();
	// scroll to the top if not already at the top
	// Note: wxTextCtrl has ShowPosition(long) which "makes the line 
	// containing the given position visible." TODO: check to see how
	// this works - does it scroll the line to the top or what???
	//if (nFirstVisible > 0)
	//	m_editGlossEquivalences.LineScroll(-nFirstVisible,0);
}

void CCaseEquivPageCommon::DoBnClickedClearTgtList()
{
	m_strTgtEquivalences.Empty();
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
}

void CCaseEquivPageCommon::DoBnClickedTgtSetEnglish()
{
	// we will use the app class's SetDefaultCaseEquivalences( ) so that we have just the
	// one source for the English defaults; so we have to preserve the app's member strings
	// across this operation
	wxString saveSrcLower = gpApp->m_srcLowerCaseChars;
	wxString saveSrcUpper = gpApp->m_srcUpperCaseChars;
	wxString saveTgtLower = gpApp->m_tgtLowerCaseChars;
	wxString saveTgtUpper = gpApp->m_tgtUpperCaseChars;
	wxString saveGlossLower = gpApp->m_glossLowerCaseChars;
	wxString saveGlossUpper = gpApp->m_glossUpperCaseChars;
	gpApp->SetDefaultCaseEquivalences();
	BuildListString(gpApp->m_tgtLowerCaseChars,gpApp->m_tgtUpperCaseChars,m_strTgtEquivalences);
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	gpApp->m_srcLowerCaseChars = saveSrcLower;
	gpApp->m_srcUpperCaseChars = saveSrcUpper;
	gpApp->m_tgtLowerCaseChars = saveTgtLower;
	gpApp->m_tgtUpperCaseChars = saveTgtUpper;
	gpApp->m_glossLowerCaseChars = saveGlossLower;
	gpApp->m_glossUpperCaseChars = saveGlossUpper;
}

void CCaseEquivPageCommon::DoBnClickedTgtCopyToNext()
{
	TransferDataFromWindow(); // transfer data from contols to string variables
	m_strSrcEquivalences = m_pEditSrcEquivalences->GetValue();
	m_strTgtEquivalences = m_pEditTgtEquivalences->GetValue();
	m_strGlossEquivalences = m_pEditGlossEquivalences->GetValue();
	m_strGlossEquivalences = m_strTgtEquivalences;
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
	// Note: wxTextCtrl has ShowPosition(long) which "makes the line 
	// containing the given position visible." TODO: check to see how
	// this works - does it scroll the line to the top or what???
	//int nFirstVisible = m_editGlossEquivalences.GetFirstVisibleLine();
	// scroll to the top if not already at the top
	m_pEditGlossEquivalences->ShowPosition(0);
	//if (nFirstVisible > 0)
	//	m_editGlossEquivalences.LineScroll(-nFirstVisible,0);
}

void CCaseEquivPageCommon::DoBnClickedClearGlossList()
{
	m_strGlossEquivalences.Empty();
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
}

void CCaseEquivPageCommon::DoBnClickedGlossSetEnglish()
{
	// we will use the app class's SetDefaultCaseEquivalences( ) so that we have just the
	// one source for the English defaults; so we have to preserve the app's member strings
	// across this operation
	wxString saveSrcLower = gpApp->m_srcLowerCaseChars;
	wxString saveSrcUpper = gpApp->m_srcUpperCaseChars;
	wxString saveTgtLower = gpApp->m_tgtLowerCaseChars;
	wxString saveTgtUpper = gpApp->m_tgtUpperCaseChars;
	wxString saveGlossLower = gpApp->m_glossLowerCaseChars;
	wxString saveGlossUpper = gpApp->m_glossUpperCaseChars;
	gpApp->SetDefaultCaseEquivalences();
	BuildListString(gpApp->m_glossLowerCaseChars,gpApp->m_glossUpperCaseChars,m_strGlossEquivalences);
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
	gpApp->m_srcLowerCaseChars = saveSrcLower;
	gpApp->m_srcUpperCaseChars = saveSrcUpper;
	gpApp->m_tgtLowerCaseChars = saveTgtLower;
	gpApp->m_tgtUpperCaseChars = saveTgtUpper;
	gpApp->m_glossLowerCaseChars = saveGlossLower;
	gpApp->m_glossUpperCaseChars = saveGlossUpper;
}

void CCaseEquivPageCommon::DoBnClickedGlossCopyToNext()
{
	TransferDataFromWindow(); // transfer data from contols to string variables
	m_strSrcEquivalences = m_pEditSrcEquivalences->GetValue();
	m_strTgtEquivalences = m_pEditTgtEquivalences->GetValue();
	m_strGlossEquivalences = m_pEditGlossEquivalences->GetValue();
	m_strSrcEquivalences = m_strGlossEquivalences;
	TransferDataToWindow(); //transfer data from variables to contols
	m_pEditSrcEquivalences->SetValue(m_strSrcEquivalences);
	m_pEditTgtEquivalences->SetValue(m_strTgtEquivalences);
	m_pEditGlossEquivalences->SetValue(m_strGlossEquivalences);
	// Note: wxTextCtrl has ShowPosition(long) which "makes the line 
	// containing the given position visible." TODO: check to see how
	// this works - does it scroll the line to the top or what???
	//int nFirstVisible = m_editSrcEquivalences.GetFirstVisibleLine();
	// scroll to the top if not already at the top
	m_pEditSrcEquivalences->ShowPosition(0);
	//if (nFirstVisible > 0)
	//	m_editSrcEquivalences.LineScroll(-nFirstVisible,0);
}

void CCaseEquivPageCommon::DoBnCheckedSrcHasCaps() // added by whm 11Aug04
{
	if (gbSrcHasUcAndLc)
	{
		// Box is checked, so uncheck it
		wxCheckBox* pChkBox1 = (wxCheckBox*)FindWindowById(ID_CHECK_SOURCE_USES_CAPS);
		pChkBox1->SetValue(FALSE);
		gbSrcHasUcAndLc = FALSE;
		// When gbSrcHasUcAndLc is FALSE, gbAutoCaps cannot be true, so insure
		// that gbAutoCaps is FALSE here (in case user monkeyed with config file
		// and changed gbAutoCaps to TRUE while gbSrcHasUcAndLc was false).
		gbAutoCaps = FALSE;
		// uncheck and make the second checkbox invisible since autocapitalization 
		// cannot be enabled when source text makes no upper/lower case distinctions.
		wxCheckBox* pChkBox2 = (wxCheckBox*)FindWindowById(ID_CHECK_USE_AUTO_CAPS);
		pChkBox2->SetValue(FALSE);
		pChkBox2->Hide();
		ToggleControlsVisibility(FALSE);
	}
	else 
	{
		// Box is unchecked, so check it
		wxCheckBox* pChkBox1 = (wxCheckBox*)FindWindowById(ID_CHECK_SOURCE_USES_CAPS);
		pChkBox1->SetValue(TRUE);
		gbSrcHasUcAndLc = TRUE;
		// make the second checkbox visible
		wxCheckBox* pChkBox2 = (wxCheckBox*)FindWindowById(ID_CHECK_USE_AUTO_CAPS);
		pChkBox2->Show();
	}
}

void CCaseEquivPageCommon::DoBnCheckedUseAutoCaps() // added by whm 11Aug04
{
	if (gbAutoCaps)
	{
		gbAutoCaps = FALSE;
		ToggleControlsVisibility(gbAutoCaps);
	}
	else
	{
		gbAutoCaps = TRUE;
		ToggleControlsVisibility(gbAutoCaps);
	}
}

// helpers
void CCaseEquivPageCommon::BuildListString(wxString& lwrCase, wxString& upprCase, wxString& strList)
{
	strList.Empty();
	int len = lwrCase.Length();
	int len1 = upprCase.Length();
	if (len != len1)
	{
		// IDS_UNEQUAL_STR_LENGTHS
		wxMessageBox(_("Sorry, the length of the upper and lower case strings are not the same. Check the project's configuration file, edit it if necessary to make the lengths identical."),_T(""),wxICON_INFORMATION);
		return;
	}
	wxString endStr = _T("\n"); // wxWidgets uses only \n
	for (int i = 0; i < len; i++)
	{
		wxChar lowerChar = lwrCase.GetChar(i);
		wxChar upperChar = upprCase.GetChar(i);
		strList += lowerChar;
		strList += upperChar;
		strList += endStr;
	}
}

bool CCaseEquivPageCommon::BuildUcLcStrings(wxString& strList, wxString& lwrCase, wxString& upprCase)
{
	if (strList.IsEmpty())
	{
		lwrCase.Empty();
		upprCase.Empty();
		return TRUE;
	}
	
	bool bNoError = TRUE;
	// Why have a minimum size of zero (0) in the following call to GetBuffer(0) in MFC code???
	// The equivalent call of wxStringBuffer(strList,0) in wxWidgets asserts, so we'll
	// get the len earlier and use len+1 for buffer length in the WX version.
	//strList = "eE\ns\ntT\n"; // For testing a bad string - Tested OK 12Aug04
	int len = strList.Length(); // does not count the null terminator
	const wxChar* pString = strList.GetData();
	wxChar* pBufStart = (wxChar*)pString;
	wxASSERT(pString);
	wxChar* pChar = pBufStart; // initialise to point to start of string
	wxChar* pEnd = pBufStart + len; // point to null terminator at end
	wxASSERT(*pEnd == _T('\0')); 
	lwrCase.Empty();
	upprCase.Empty();
	do {
		lwrCase += *pChar++; // get the lower case character and advance pointer
		if (pChar == pEnd)
		{
			// if the list ends prematurely, skip the last singleton character;
			return FALSE; // flag the error
		}
		if (*pChar == _T('\n'))
		{
			// if there is a singleton character within the list, skip it, and 
			// continue with the next pair after removing whatever was stored for the
			// lower case character
			int length = lwrCase.Length();
			// Note: wsString::Remove must have the second param as 1 here otherwise
			// it will truncate the remainder of the string!
			lwrCase.Remove(--length,1);
			bNoError = FALSE; // flag the error
		}
		else
		{
			// otherwise, the current character pointed at is the upper case equivalent
			upprCase += *pChar++;
		}
		pChar++;
	} while (pChar < pEnd);
	return bNoError;
}

void CCaseEquivPageCommon::ToggleControlsVisibility(bool visible)
{
	if (visible == FALSE)
	{
		// Hide the other dialog controls
		wxTextCtrl* pTextCtrlAsStaticText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_CASE_PAGE_STATIC_TEXT);
		pTextCtrlAsStaticText->Hide();
		wxStaticText* pStatText5 = (wxStaticText*)FindWindowById(ID_TEXT_SL);
		pStatText5->Hide();
		wxStaticText* pStatText6 = (wxStaticText*)FindWindowById(ID_TEXT_TL);
		pStatText6->Hide();
		wxStaticText* pStatText7 = (wxStaticText*)FindWindowById(ID_TEXT_GL);
		pStatText7->Hide();

		wxTextCtrl* pEdit1 = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_CASE_EQUIVALENCES);
		pEdit1->Hide();
		wxTextCtrl* pEdit2 = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_CASE_EQUIVALENCES);
		pEdit2->Hide();
		wxTextCtrl* pEdit3 = (wxTextCtrl*)FindWindowById(IDC_EDIT_GLOSS_CASE_EQUIVALENCES);
		pEdit3->Hide();

		wxButton* pButton1 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_SRC_LIST);
		pButton1->Hide();
		wxButton* pButton2 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_SET_ENGLISH);
		pButton2->Hide();
		wxButton* pButton3 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_COPY_TO_NEXT);
		pButton3->Hide();
		wxButton* pButton4 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_COPY_TO_GLOSS);
		pButton4->Hide();

		wxButton* pButton5 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_TGT_LIST);
		pButton5->Hide();
		wxButton* pButton6 = (wxButton*)FindWindowById(IDC_BUTTON_TGT_SET_ENGLISH);
		pButton6->Hide();
		wxButton* pButton7 = (wxButton*)FindWindowById(IDC_BUTTON_TGT_COPY_TO_NEXT);
		pButton7->Hide();

		wxButton* pButton8 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_GLOSS_LIST);
		pButton8->Hide();
		wxButton* pButton9 = (wxButton*)FindWindowById(IDC_BUTTON_GLOSS_SET_ENGLISH);
		pButton9->Hide();
		wxButton* pButton10 = (wxButton*)FindWindowById(IDC_BUTTON_GLOSS_COPY_TO_NEXT);
		pButton10->Hide();
	}
	else
	{
		// show the other dialog controls
		wxTextCtrl* pTextCtrlAsStaticText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_CASE_PAGE_STATIC_TEXT);
		wxColor backgrndColor = this->GetBackgroundColour();
		pTextCtrlAsStaticText->SetBackgroundColour(backgrndColor);
		pTextCtrlAsStaticText->Show();
		wxStaticText* pStatText5 = (wxStaticText*)FindWindowById(ID_TEXT_SL);
		pStatText5->Show();
		wxStaticText* pStatText6 = (wxStaticText*)FindWindowById(ID_TEXT_TL);
		pStatText6->Show();
		wxStaticText* pStatText7 = (wxStaticText*)FindWindowById(ID_TEXT_GL);
		pStatText7->Show();

		wxTextCtrl* pEdit1 = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC_CASE_EQUIVALENCES);
		pEdit1->Show();
		wxTextCtrl* pEdit2 = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT_CASE_EQUIVALENCES);
		pEdit2->Show();
		wxTextCtrl* pEdit3 = (wxTextCtrl*)FindWindowById(IDC_EDIT_GLOSS_CASE_EQUIVALENCES);
		pEdit3->Show();

		wxButton* pButton1 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_SRC_LIST);
		pButton1->Show();
		wxButton* pButton2 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_SET_ENGLISH);
		pButton2->Show();
		wxButton* pButton3 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_COPY_TO_NEXT);
		pButton3->Show();
		wxButton* pButton4 = (wxButton*)FindWindowById(IDC_BUTTON_SRC_COPY_TO_GLOSS);
		pButton4->Show();

		wxButton* pButton5 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_TGT_LIST);
		pButton5->Show();
		wxButton* pButton6 = (wxButton*)FindWindowById(IDC_BUTTON_TGT_SET_ENGLISH);
		pButton6->Show();
		wxButton* pButton7 = (wxButton*)FindWindowById(IDC_BUTTON_TGT_COPY_TO_NEXT);
		pButton7->Show();

		wxButton* pButton8 = (wxButton*)FindWindowById(IDC_BUTTON_CLEAR_GLOSS_LIST);
		pButton8->Show();
		wxButton* pButton9 = (wxButton*)FindWindowById(IDC_BUTTON_GLOSS_SET_ENGLISH);
		pButton9->Show();
		wxButton* pButton10 = (wxButton*)FindWindowById(IDC_BUTTON_GLOSS_COPY_TO_NEXT);
		pButton10->Show();
	}
	pCaseEquivSizer->Layout();
}

IMPLEMENT_DYNAMIC_CLASS( CCaseEquivPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CCaseEquivPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CCaseEquivPageWiz::InitDialog)// not strictly necessary for dialogs based on wxDialog
    EVT_WIZARD_PAGE_CHANGING(-1, CCaseEquivPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CCaseEquivPageWiz::OnWizardCancel)
	EVT_BUTTON(IDC_BUTTON_CLEAR_SRC_LIST, CCaseEquivPageWiz::OnBnClickedClearSrcList)
	EVT_BUTTON(IDC_BUTTON_SRC_SET_ENGLISH, CCaseEquivPageWiz::OnBnClickedSrcSetEnglish)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_NEXT, CCaseEquivPageWiz::OnBnClickedSrcCopyToNext)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_GLOSS, CCaseEquivPageWiz::OnBnClickedSrcCopyToGloss)
	EVT_BUTTON(IDC_BUTTON_CLEAR_TGT_LIST, CCaseEquivPageWiz::OnBnClickedClearTgtList)
	EVT_BUTTON(IDC_BUTTON_TGT_SET_ENGLISH, CCaseEquivPageWiz::OnBnClickedTgtSetEnglish)
	EVT_BUTTON(IDC_BUTTON_TGT_COPY_TO_NEXT, CCaseEquivPageWiz::OnBnClickedTgtCopyToNext)
	EVT_BUTTON(IDC_BUTTON_CLEAR_GLOSS_LIST, CCaseEquivPageWiz::OnBnClickedClearGlossList)
	EVT_BUTTON(IDC_BUTTON_GLOSS_SET_ENGLISH, CCaseEquivPageWiz::OnBnClickedGlossSetEnglish)
	EVT_BUTTON(IDC_BUTTON_GLOSS_COPY_TO_NEXT, CCaseEquivPageWiz::OnBnClickedGlossCopyToNext)
	EVT_CHECKBOX(ID_CHECK_SOURCE_USES_CAPS,CCaseEquivPageWiz::OnBnCheckedSrcHasCaps)
	EVT_CHECKBOX(ID_CHECK_USE_AUTO_CAPS,CCaseEquivPageWiz::OnBnCheckedUseAutoCaps)
END_EVENT_TABLE()


CCaseEquivPageWiz::CCaseEquivPageWiz()
{
}

CCaseEquivPageWiz::CCaseEquivPageWiz(wxWizard* parent) // constructor
{
	Create( parent );

	casePgCommon.DoSetDataAndPointers();
}

CCaseEquivPageWiz::~CCaseEquivPageWiz() // destructor
{
	
}


bool CCaseEquivPageWiz::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CCaseEquivPageWiz::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	casePgCommon.pCaseEquivSizer = CaseEquivDlgFunc(this, TRUE, TRUE);
}

// implement wxWizardPage functions
wxWizardPage* CCaseEquivPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pPunctCorrespPageWiz; 
}
wxWizardPage* CCaseEquivPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pUsfmPageWiz;
}

void CCaseEquivPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}

void CCaseEquivPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	casePgCommon.DoInit();
}


void CCaseEquivPageWiz::OnBnClickedClearSrcList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearSrcList();
}

void CCaseEquivPageWiz::OnBnClickedSrcSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcSetEnglish();
}

void CCaseEquivPageWiz::OnBnClickedSrcCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcCopyToNext();
}

// At this point the MFC version has a handler called OnBnClickedTurnOnAutoCaps to handle
// the "Use Automatic Capitalization" at the bottom of the casePage. The wx version casePage
// does not have that button, but instead near the top has two check boxes, that function
// to hide most of the casePage controls and static text. The three list boxes and associated
// static text stay hidden unless the user checks both of the check boxes at the top of the
// casePage: (1) the first checkbox indicates that the source language does distinguish upper 
// and lower case, and (2) the second checkbox indicates the user wants Adapt It to 
// automatically distinguish between upper and lower case letters (the effect is the same as
// MFC's "Use Automatic Capitalization" button. The first checkbox is kept track of by the 
// gbSrcHasUcAndLc global, and the second is kep track of by the gbAutoCaps global. In the 
// wx version both values are stored in the project config file; whereas in the MFC version 
// only the gbAutoCaps value was stored in the project config file.
void CCaseEquivPageWiz::OnBnCheckedSrcHasCaps(wxCommandEvent& WXUNUSED(event)) // added by whm 11Aug04
{
	casePgCommon.DoBnCheckedSrcHasCaps();
}

void CCaseEquivPageWiz::OnBnCheckedUseAutoCaps(wxCommandEvent& WXUNUSED(event)) // added by whm 11Aug04
{
	casePgCommon.DoBnCheckedUseAutoCaps();
}

void CCaseEquivPageWiz::OnBnClickedClearTgtList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearTgtList();
}

void CCaseEquivPageWiz::OnBnClickedTgtSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedTgtSetEnglish();
}

void CCaseEquivPageWiz::OnBnClickedTgtCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedTgtCopyToNext();
}

void CCaseEquivPageWiz::OnBnClickedClearGlossList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearGlossList();
}

void CCaseEquivPageWiz::OnBnClickedGlossSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedGlossSetEnglish();
}

void CCaseEquivPageWiz::OnBnClickedGlossCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedGlossCopyToNext();
}

void CCaseEquivPageWiz::OnBnClickedSrcCopyToGloss(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcCopyToGloss();
}

void CCaseEquivPageWiz::OnWizardPageChanging(wxWizardEvent& event)
{
	// Can put any code that needs to execute regardless of whether
	// Next or Prev button was pushed here.

	// Determine which direction we're going and implement
	// the MFC equivalent of OnWizardNext() and OnWizardBack() here
	bool bMovingForward = event.GetDirection();
	if (bMovingForward)
	{
		// Next wizard button was selected
		TransferDataFromWindow(); // transfer data from contols to string variables
		casePgCommon.m_strSrcEquivalences = casePgCommon.m_pEditSrcEquivalences->GetValue();
		casePgCommon.m_strTgtEquivalences = casePgCommon.m_pEditTgtEquivalences->GetValue();
		casePgCommon.m_strGlossEquivalences = casePgCommon.m_pEditGlossEquivalences->GetValue();
		bool bGood = TRUE;

		// if the user turned on automatic capitalization in this dialog, then make sure the
		// menu item of that name is ticked (or unticked if off)
		if (gbAutoCaps)
		{
			gbAutoCaps = FALSE;
		}
		else
		{
			gbAutoCaps = TRUE;
		}
		gpApp->OnToolsAutoCapitalization(event); // TODO: check if this is needed ???
	
		// build the source case strings
		bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strSrcEquivalences, gpApp->m_srcLowerCaseChars, 
																		gpApp->m_srcUpperCaseChars);
		if (!bGood)
		{
			// don't let the user dismiss the dialog until the error is fixed
			wxString s;
			wxString strWhich;
			// IDS_SOURCE_STR
			strWhich = strWhich.Format(_("Source"));
			//IDS_CASE_EQUIVALENCES_ERROR
			s = s.Format(_("Sorry, in the Case wizard page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
			wxMessageBox(s,_T(""), wxICON_INFORMATION);
			event.Veto(); // add this to stop page change
			return;
		}
		if (gpApp->m_srcLowerCaseChars.IsEmpty() && gpApp->m_srcUpperCaseChars.IsEmpty())
			gbNoSourceCaseEquivalents = TRUE;
		else
			gbNoSourceCaseEquivalents = FALSE;

		// build the target case strings
		bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strTgtEquivalences, gpApp->m_tgtLowerCaseChars, 
																	gpApp->m_tgtUpperCaseChars);
		if (!bGood)
		{
			// don't let the user dismiss the dialog until the error is fixed
			wxString s;
			wxString strWhich;
			// IDS_TARGET_STR
			strWhich = strWhich.Format(_("Target"));
			// IDS_CASE_EQUIVALENCES_ERROR
			s = s.Format(_("Sorry, in the Case wizard page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
			wxMessageBox(s,_T(""), wxICON_INFORMATION);
			event.Veto(); // add this to stop page change
			return;
		}
		if (gpApp->m_tgtLowerCaseChars.IsEmpty() && gpApp->m_tgtUpperCaseChars.IsEmpty())
			gbNoTargetCaseEquivalents = TRUE;
		else
			gbNoTargetCaseEquivalents = FALSE;

		// build the gloss case strings
		bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strGlossEquivalences, gpApp->m_glossLowerCaseChars, 
																	gpApp->m_glossUpperCaseChars);
		if (!bGood)
		{
			// don't let the user dismiss the dialog until the error is fixed
			wxString s;
			wxString strWhich;
			// IDS_GLOSS_STR
			strWhich = strWhich.Format(_("Gloss"));
			// IDS_CASE_EQUIVALENCES_ERROR
			s = s.Format(_("Sorry, in the Case wizard page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
			wxMessageBox(s, _T(""), wxICON_INFORMATION);
			event.Veto(); // add this to stop page change
			return;
		}
		if (gpApp->m_glossLowerCaseChars.IsEmpty() && gpApp->m_glossUpperCaseChars.IsEmpty())
			gbNoGlossCaseEquivalents = TRUE;
		else
			gbNoGlossCaseEquivalents = FALSE;
		
		// Movement through wizard pages is sequential - the next page is the usfmPageWiz.
		// The pUsfmPageWiz's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pUsfmPageWiz->InitDialog(idevent);
	}
	else
	{
		// Prev wizard button was selected
		//TODO: implement anything here???
		;
	}
}

IMPLEMENT_DYNAMIC_CLASS( CCaseEquivPagePrefs, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CCaseEquivPagePrefs, wxWizardPage)
	EVT_INIT_DIALOG(CCaseEquivPagePrefs::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(IDC_BUTTON_CLEAR_SRC_LIST, CCaseEquivPagePrefs::OnBnClickedClearSrcList)
	EVT_BUTTON(IDC_BUTTON_SRC_SET_ENGLISH, CCaseEquivPagePrefs::OnBnClickedSrcSetEnglish)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_NEXT, CCaseEquivPagePrefs::OnBnClickedSrcCopyToNext)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_GLOSS, CCaseEquivPagePrefs::OnBnClickedSrcCopyToGloss)
	EVT_BUTTON(IDC_BUTTON_CLEAR_TGT_LIST, CCaseEquivPagePrefs::OnBnClickedClearTgtList)
	EVT_BUTTON(IDC_BUTTON_TGT_SET_ENGLISH, CCaseEquivPagePrefs::OnBnClickedTgtSetEnglish)
	EVT_BUTTON(IDC_BUTTON_TGT_COPY_TO_NEXT, CCaseEquivPagePrefs::OnBnClickedTgtCopyToNext)
	EVT_BUTTON(IDC_BUTTON_CLEAR_GLOSS_LIST, CCaseEquivPagePrefs::OnBnClickedClearGlossList)
	EVT_BUTTON(IDC_BUTTON_GLOSS_SET_ENGLISH, CCaseEquivPagePrefs::OnBnClickedGlossSetEnglish)
	EVT_BUTTON(IDC_BUTTON_GLOSS_COPY_TO_NEXT, CCaseEquivPagePrefs::OnBnClickedGlossCopyToNext)
	EVT_CHECKBOX(ID_CHECK_SOURCE_USES_CAPS,CCaseEquivPagePrefs::OnBnCheckedSrcHasCaps)
	EVT_CHECKBOX(ID_CHECK_USE_AUTO_CAPS,CCaseEquivPagePrefs::OnBnCheckedUseAutoCaps)
	//EVT_BUTTON(wxID_OK, CCaseEquivPagePrefs::OnOK)
END_EVENT_TABLE()


CCaseEquivPagePrefs::CCaseEquivPagePrefs()
{
}

CCaseEquivPagePrefs::CCaseEquivPagePrefs(wxWindow* parent) // constructor
{
	Create( parent );
	
	casePgCommon.DoSetDataAndPointers();
}

CCaseEquivPagePrefs::~CCaseEquivPagePrefs() // destructor
{
	
}


bool CCaseEquivPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CCaseEquivPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	casePgCommon.pCaseEquivSizer = CaseEquivDlgFunc(this, TRUE, TRUE);
}

void CCaseEquivPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	casePgCommon.DoInit();
}


void CCaseEquivPagePrefs::OnBnClickedClearSrcList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearSrcList();
}

void CCaseEquivPagePrefs::OnBnClickedSrcSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcSetEnglish();
}

void CCaseEquivPagePrefs::OnBnClickedSrcCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcCopyToNext();
}

// At this point the MFC version has a handler called OnBnClickedTurnOnAutoCaps to handle
// the "Use Automatic Capitalization" at the bottom of the casePage. The wx version casePage
// does not have that button, but instead near the top has two check boxes, that function
// to hide most of the casePage controls and static text. The three list boxes and associated
// static text stay hidden unless the user checks both of the check boxes at the top of the
// casePage: (1) the first checkbox indicates that the source language does distinguish upper 
// and lower case, and (2) the second checkbox indicates the user wants Adapt It to 
// automatically distinguish between upper and lower case letters (the effect is the same as
// MFC's "Use Automatic Capitalization" button. The first checkbox is kept track of by the 
// gbSrcHasUcAndLc global, and the second is kep track of by the gbAutoCaps global. In the 
// wx version both values are stored in the project config file; whereas in the MFC version 
// only the gbAutoCaps value was stored in the project config file.
void CCaseEquivPagePrefs::OnBnCheckedSrcHasCaps(wxCommandEvent& WXUNUSED(event)) // added by whm 11Aug04
{
	casePgCommon.DoBnCheckedSrcHasCaps();
}

void CCaseEquivPagePrefs::OnBnCheckedUseAutoCaps(wxCommandEvent& WXUNUSED(event)) // added by whm 11Aug04
{
	casePgCommon.DoBnCheckedUseAutoCaps();
}



void CCaseEquivPagePrefs::OnBnClickedClearTgtList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearTgtList();
}

void CCaseEquivPagePrefs::OnBnClickedTgtSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedTgtSetEnglish();
}

void CCaseEquivPagePrefs::OnBnClickedTgtCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedTgtCopyToNext();
}

void CCaseEquivPagePrefs::OnBnClickedClearGlossList(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedClearGlossList();
}

void CCaseEquivPagePrefs::OnBnClickedGlossSetEnglish(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedGlossSetEnglish();
}

void CCaseEquivPagePrefs::OnBnClickedGlossCopyToNext(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedGlossCopyToNext();
}

void CCaseEquivPagePrefs::OnOK(wxCommandEvent& event)
{
	// OK button was selected
	TransferDataFromWindow(); // transfer data from contols to string variables
	casePgCommon.m_strSrcEquivalences = casePgCommon.m_pEditSrcEquivalences->GetValue();
	casePgCommon.m_strTgtEquivalences = casePgCommon.m_pEditTgtEquivalences->GetValue();
	casePgCommon.m_strGlossEquivalences = casePgCommon.m_pEditGlossEquivalences->GetValue();
	bool bGood = TRUE;

	// if the user turned on automatic capitalization in this dialog, then make sure the
	// menu item of that name is ticked (or unticked if off)
	if (gbAutoCaps)
	{
		gbAutoCaps = FALSE;
	}
	else
	{
		gbAutoCaps = TRUE;
	}
	gpApp->OnToolsAutoCapitalization(event); // TODO: check if this is needed ???
	
	// build the source case strings
	bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strSrcEquivalences, gpApp->m_srcLowerCaseChars, 
																	gpApp->m_srcUpperCaseChars);
	if (!bGood)
	{
		// don't let the user dismiss the dialog until the error is fixed
		wxString s;
		wxString strWhich;
		// IDS_SOURCE_STR
		strWhich = strWhich.Format(_("Source"));
		//IDS_CASE_EQUIVALENCES_ERROR
		s = s.Format(_("Sorry, in the Case tab page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
		wxMessageBox(s,_T(""), wxICON_INFORMATION);
		//event.Veto(); // add this to stop page change
		// TODO: put code here to automatically select the Case tab
		return;
	}
	if (gpApp->m_srcLowerCaseChars.IsEmpty() && gpApp->m_srcUpperCaseChars.IsEmpty())
		gbNoSourceCaseEquivalents = TRUE;
	else
		gbNoSourceCaseEquivalents = FALSE;

	// build the target case strings
	bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strTgtEquivalences, gpApp->m_tgtLowerCaseChars, 
																gpApp->m_tgtUpperCaseChars);
	if (!bGood)
	{
		// don't let the user dismiss the dialog until the error is fixed
		wxString s;
		wxString strWhich;
		// IDS_TARGET_STR
		strWhich = strWhich.Format(_("Target"));
		// IDS_CASE_EQUIVALENCES_ERROR
		s = s.Format(_("Sorry, in the Case tab page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
		wxMessageBox(s,_T(""), wxICON_INFORMATION);
		//event.Veto(); // add this to stop page change
		// TODO: put code here to automatically select the Case tab
		return;
	}
	if (gpApp->m_tgtLowerCaseChars.IsEmpty() && gpApp->m_tgtUpperCaseChars.IsEmpty())
		gbNoTargetCaseEquivalents = TRUE;
	else
		gbNoTargetCaseEquivalents = FALSE;

	// build the gloss case strings
	bGood = casePgCommon.BuildUcLcStrings(casePgCommon.m_strGlossEquivalences, gpApp->m_glossLowerCaseChars, 
																gpApp->m_glossUpperCaseChars);
	if (!bGood)
	{
		// don't let the user dismiss the dialog until the error is fixed
		wxString s;
		wxString strWhich;
		// IDS_GLOSS_STR
		strWhich = strWhich.Format(_("Gloss"));
		// IDS_CASE_EQUIVALENCES_ERROR
		s = s.Format(_("Sorry, in the Case tab page, the %s list contains and error - one or more\nof the lines has only a single letter. Each line must contain a lower/upper case pair of letters."),strWhich.c_str());
		wxMessageBox(s, _T(""), wxICON_INFORMATION);
		//event.Veto(); // add this to stop page change
		// TODO: put code here to automatically select the Case tab
		return;
	}
	if (gpApp->m_glossLowerCaseChars.IsEmpty() && gpApp->m_glossUpperCaseChars.IsEmpty())
		gbNoGlossCaseEquivalents = TRUE;
	else
		gbNoGlossCaseEquivalents = FALSE;
}

void CCaseEquivPagePrefs::OnBnClickedSrcCopyToGloss(wxCommandEvent& WXUNUSED(event))
{
	casePgCommon.DoBnClickedSrcCopyToGloss();
}

