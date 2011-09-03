/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EditPreferencesDlg.cpp
/// \author			Bill Martin
/// \date_created	13 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CEditPreferencesDlg class. 
/// The CEditPreferencesDlg class acts as a dialog wrapper for the tab pages of
/// an "Edit Preferences" wxNotebook. The interface resources for the wxNotebook 
/// dialog are defined in EditPreferencesDlgFunc(), which was created and is 
/// maintained by wxDesigner. The notebook contains nine tabs labeled "Fonts", 
/// "Backups and KB", "View", "Auto-Saving", "Punctuation", "Case", "Units", 
/// "USFM", and "Filtering".
/// \derivation		The CEditPreferencesDlg class is derived from wxPropertySheetDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in EditPreferencesDlg.cpp (in order of importance): (search for "TODO")
// 1. Debug the RTL stuff and conditional compile it for other platforms
//
// Unanswered questions: (search for "???")
// 1. Not sure why wxGenericValidator won't work for me here in CEditPreferencesDlg.
//    Perhaps it's because of the multiple page nature of the dialog. So, as a work
//    around I've done transfers to/from contols and variables manually.
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "EditPreferencesDlg.h"
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
#include <wx/fontdlg.h>
#include <wx/cmndata.h>
#include <wx/valgen.h>
#include <wx/colordlg.h>
#include <wx/wizard.h>
#include <wx/propdlg.h>
#include <wx/display.h> // for wxDisplay

#include "Adapt_It.h" // for access to extern fontInfo structs below
#include "EditPreferencesDlg.h"
#include "Adapt_It_Resources.h"
#include "Adapt_ItDoc.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "KBPage.h"
#include "ViewPage.h"
#include "AutoSavingPage.h"
#include "UnitsPage.h"
#include "USFMPage.h"
#include "FilterPage.h"
#include "helpers.h"
#include "Pile.h"
#include "Layout.h"

//extern wxChar gSFescapechar;
//extern bool gbSfmOnlyAfterNewlines;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing;

/// This global is defined in Adapt_It.cpp.
extern CFilterPagePrefs* pFilterPageInPrefs; // set the App's pointer to the filterPage

/// This global is defined in Adapt_It.cpp.
extern CUSFMPagePrefs* pUSFMPageInPrefs; // set the App's pointer to the filterPage

/// This global is defined in Adapt_ItView.cpp.
extern CAdapt_ItApp* gpApp;

// For temporary emulation of MFC's Logfont structs. These
// may disappear if/when we remove Logfont member values from
// our config files.
extern struct fontInfo SrcFInfo, TgtFInfo, NavFInfo;

IMPLEMENT_DYNAMIC_CLASS(CEditPreferencesDlg, wxPropertySheetDialog)

// event handler table
BEGIN_EVENT_TABLE(CEditPreferencesDlg, wxPropertySheetDialog)
	EVT_INIT_DIALOG(CEditPreferencesDlg::InitDialog) // not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CEditPreferencesDlg::OnOK)

	// Note: The following handlers call methods of the same name in CFontPagePrefs
	EVT_BUTTON(IDC_SOURCE_LANG, CEditPreferencesDlg::OnSourceFontChangeBtn)
	EVT_BUTTON(IDC_TARGET_LANG, CEditPreferencesDlg::OnTargetFontChangeBtn)
	EVT_BUTTON(IDC_CHANGE_NAV_TEXT, CEditPreferencesDlg::OnNavTextFontChangeBtn)
	EVT_BUTTON(IDC_BUTTON_SPECTEXTCOLOR, CEditPreferencesDlg::OnButtonSpecTextColor)
	EVT_BUTTON(IDC_RETRANSLATION_BUTTON, CEditPreferencesDlg::OnButtonRetranTextColor)
	EVT_BUTTON(IDC_BUTTON_NAV_TEXT_COLOR, CEditPreferencesDlg::OnButtonNavTextColor)
	EVT_BUTTON(IDC_BUTTON_SOURCE_COLOR, CEditPreferencesDlg::OnButtonSourceTextColor)
	EVT_BUTTON(IDC_BUTTON_TARGET_COLOR, CEditPreferencesDlg::OnButtonTargetTextColor)
	
	// The following wrapper handlers are for kbPage
	EVT_CHECKBOX(IDC_CHECK_KB_BACKUP, CEditPreferencesDlg::OnCheckKbBackup)
	EVT_CHECKBOX(IDC_CHECK_BAKUP_DOC, CEditPreferencesDlg::OnCheckBakupDoc)
	// The following wrapper handler is for the viewPage
	EVT_BUTTON(IDC_BUTTON_CHOOSE_HIGHLIGHT_COLOR, CEditPreferencesDlg::OnButtonHighlightColor)
	// The following wrapper handlers are for the autoSavePage
	EVT_RADIOBUTTON(IDC_RADIO_BY_MINUTES, CEditPreferencesDlg::OnRadioByMinutes)
	EVT_RADIOBUTTON(IDC_RADIO_BY_MOVES, CEditPreferencesDlg::OnRadioByMoves)
	EVT_CHECKBOX(IDC_CHECK_NO_AUTOSAVE, CEditPreferencesDlg::OnCheckNoAutoSave)
	// The following wrapper handlers are for the punctMapPage
#ifdef _UNICODE
	EVT_BUTTON(IDC_TOGGLE_UNNNN_BTN, CEditPreferencesDlg::OnBnClickedToggleUnnnn)
#endif
	// The following wrapper handlers are for the caseEquivPage
	EVT_BUTTON(IDC_BUTTON_CLEAR_SRC_LIST, CEditPreferencesDlg::OnBnClickedClearSrcList)
	EVT_BUTTON(IDC_BUTTON_SRC_SET_ENGLISH, CEditPreferencesDlg::OnBnClickedSrcSetEnglish)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_NEXT, CEditPreferencesDlg::OnBnClickedSrcCopyToNext)
	EVT_BUTTON(IDC_BUTTON_SRC_COPY_TO_GLOSS, CEditPreferencesDlg::OnBnClickedSrcCopyToGloss)
	EVT_BUTTON(IDC_BUTTON_CLEAR_TGT_LIST, CEditPreferencesDlg::OnBnClickedClearTgtList)
	EVT_BUTTON(IDC_BUTTON_TGT_SET_ENGLISH, CEditPreferencesDlg::OnBnClickedTgtSetEnglish)
	EVT_BUTTON(IDC_BUTTON_TGT_COPY_TO_NEXT, CEditPreferencesDlg::OnBnClickedTgtCopyToNext)
	EVT_BUTTON(IDC_BUTTON_CLEAR_GLOSS_LIST, CEditPreferencesDlg::OnBnClickedClearGlossList)
	EVT_BUTTON(IDC_BUTTON_GLOSS_SET_ENGLISH, CEditPreferencesDlg::OnBnClickedGlossSetEnglish)
	EVT_BUTTON(IDC_BUTTON_GLOSS_COPY_TO_NEXT, CEditPreferencesDlg::OnBnClickedGlossCopyToNext)
	EVT_CHECKBOX(ID_CHECK_SOURCE_USES_CAPS, CEditPreferencesDlg::OnBnCheckedSrcHasCaps)
	EVT_CHECKBOX(ID_CHECK_USE_AUTO_CAPS,CEditPreferencesDlg::OnBnCheckedUseAutoCaps)
	// The following wrapper handlers are for the unitsPage
	EVT_RADIOBUTTON(IDC_RADIO_INCHES, CEditPreferencesDlg::OnRadioUseInches)
	EVT_RADIOBUTTON(IDC_RADIO_CM, CEditPreferencesDlg::OnRadioUseCentimeters)
	// The following wrapper handlers are for the usfmPage
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY, CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY, CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS, CEditPreferencesDlg::OnBnClickedRadioUseBothSetsDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_PROJ, CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ, CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_PROJ, CEditPreferencesDlg::OnBnClickedRadioUseBothSetsProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_FACTORY, CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_FACTORY, CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_FACTORY, CEditPreferencesDlg::OnBnClickedRadioUseBothSetsFactory)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES, CEditPreferencesDlg::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
	// The following wrapper handlers are for the filterPage
	EVT_LISTBOX(IDC_LIST_SFMS, CEditPreferencesDlg::OnLbnSelchangeListSfmsDoc)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS, CEditPreferencesDlg::OnCheckListBoxToggleDoc)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CEditPreferencesDlg::OnLbnSelchangeListSfmsProj)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_PROJ, CEditPreferencesDlg::OnCheckListBoxToggleProj)
	EVT_LISTBOX(IDC_LIST_SFMS_FACTORY, CEditPreferencesDlg::OnLbnSelchangeListSfmsFactory)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_FACTORY, CEditPreferencesDlg::OnCheckListBoxToggleFactory)
END_EVENT_TABLE()

CEditPreferencesDlg::CEditPreferencesDlg()
{
}

CEditPreferencesDlg::CEditPreferencesDlg(
	wxWindow* parent, wxWindowID id, const wxString& title,
	const wxPoint& pos, const wxSize& size,
	long style)
{
	Create(parent, id, title, pos, size, style);
}

bool CEditPreferencesDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP);
    wxPropertySheetDialog::Create( parent, id, caption, pos, size, style );

    CreateButtons(wxOK|wxCANCEL|wxHELP);
    CreateControls();
    LayoutDialog();
    Centre();
    return true;
}

void CEditPreferencesDlg::CreateControls()
{    
	pNotebook = GetBookCtrl(); // gets pointer to the default wxNotebook that contains the tab pages

	// create the pages for the notebook of the property sheet
	fontPage = new CFontPagePrefs(pNotebook);
	wxASSERT(fontPage != NULL);
	punctMapPage = new CPunctCorrespPagePrefs(pNotebook);
	wxASSERT(punctMapPage != NULL);
	caseEquivPage = new CCaseEquivPagePrefs(pNotebook);
	wxASSERT(caseEquivPage != NULL);
	kbPage = new CKBPage(pNotebook);
	wxASSERT(kbPage != NULL);
	viewPage = new CViewPage(pNotebook);
	wxASSERT(viewPage != NULL);
	autoSavePage = new CAutoSavingPage(pNotebook);
	wxASSERT(autoSavePage != NULL);
	unitsPage = new CUnitsPage(pNotebook);
	wxASSERT(unitsPage != NULL);
	usfmPage = new CUSFMPagePrefs(pNotebook);
	pUSFMPageInPrefs = usfmPage; // set the App's pointer to the usfmPage
	wxASSERT(usfmPage != NULL);
	filterPage = new CFilterPagePrefs(pNotebook);
	pFilterPageInPrefs = filterPage; // set the App's pointer to the filterPage
	wxASSERT(filterPage != NULL);

	// add pages to the notebook
    pNotebook->AddPage( fontPage, _("Fonts"),TRUE ); // TRUE = page should be selected
    pNotebook->AddPage( kbPage, _("Backups and Misc"),FALSE ); // was "Backups and KB" in legacy app
    pNotebook->AddPage( viewPage, _("View"),FALSE );
    pNotebook->AddPage( autoSavePage, _("Auto-Saving"),FALSE );
    pNotebook->AddPage( punctMapPage, _("Punctuation"),FALSE );
    pNotebook->AddPage( caseEquivPage, _("Case"),FALSE );
    pNotebook->AddPage( unitsPage, _("Units"),FALSE );
    pNotebook->AddPage( usfmPage, _("USFM"),FALSE );
    pNotebook->AddPage( filterPage, _("Filtering"),FALSE );
	
	// Check if the Preferences property sheet is going to be too big. 
	// Make the pPunctCorrespPageWiz and the pCaseEquivPageWiz
	// scrollable if we are on a small screen
	// Check the display size to see if we need to make size adjustments in
	// the Wizard.
	// Get the largest minimum page size needed for all pages of the prefs to display fully
	wxSize neededSize;
	wxSize fontPageSize = fontPage->GetSize();
	if (fontPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(fontPageSize.GetX());
	if (fontPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(fontPageSize.GetY());
	wxSize kbPageSize = kbPage->GetSize();
	if (kbPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(kbPageSize.GetX());
	if (kbPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(kbPageSize.GetY());
	wxSize viewPageSize = viewPage->GetSize();
	if (viewPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(viewPageSize.GetX());
	if (viewPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(viewPageSize.GetY());
	wxSize autoSavePageSize = autoSavePage->GetSize();
	if (autoSavePageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(autoSavePageSize.GetX());
	if (autoSavePageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(autoSavePageSize.GetY());
	wxSize punctMapPageSize = punctMapPage->GetSize();
	if (punctMapPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(punctMapPageSize.GetX());
	if (punctMapPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(punctMapPageSize.GetY());
	wxSize caseEquivPageSize = caseEquivPage->GetSize();
	if (caseEquivPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(caseEquivPageSize.GetX());
	if (caseEquivPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(caseEquivPageSize.GetY());
	wxSize unitsPageSize = unitsPage->GetSize();
	if (unitsPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(unitsPageSize.GetX());
	if (unitsPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(unitsPageSize.GetY());
	wxSize usfmPageSize = usfmPage->GetSize();
	if (usfmPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(usfmPageSize.GetX());
	if (usfmPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(usfmPageSize.GetY());
	wxSize filterPageSize = filterPage->GetSize();
	if (filterPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(filterPageSize.GetX());
	if (filterPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(filterPageSize.GetY());

	// whm 31Aug10 added test below to validate results from wxDisplay after finding some problems with
	// an invalid index on a Linux machine that had dual monitors.
	int indexOfDisplay,numDisplays;
	numDisplays = wxDisplay::GetCount();
	indexOfDisplay = wxDisplay::GetFromWindow(this);
	if (numDisplays >= 1 && numDisplays <= 4 && indexOfDisplay != wxNOT_FOUND && indexOfDisplay >= 0 && indexOfDisplay <=3)
	{
		wxSize displaySize = wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea().GetSize();
		wxSize prefsSize = this->GetSize();
		wxSize prefsClientSize = this->GetClientSize();
		int prefsFrameHeight = abs(prefsSize.GetY() - prefsClientSize.GetY());
		if (neededSize.GetHeight() + prefsFrameHeight > displaySize.GetHeight())
		{
			// We fit the prefs to the neededSize but it will be too big to fit in the display
			// window, so we will have to limit the size of the prefs dialog and possibly make the 
			// taller pages such as the punctMapPage a scrolling pane.
			wxSize maxSz;
			maxSz.SetHeight(displaySize.GetHeight() - 50);
			this->SetMaxSize(maxSz);
		}
	}
	// The wxNotebook will automatically adjust to the largest page if the conditional block above
	// doesn't execute.
}



CEditPreferencesDlg::~CEditPreferencesDlg(void)
{
}

void CEditPreferencesDlg::InitDialog(wxInitDialogEvent& event)
{
    // clear the booleans for tracking whether or not the user changes any of the
    // preferences settings; we also clear each one individually at the individual page -
    // because only doing that will catch the situation where the user may visit the same
    // page more than once and edit each time and after the final edit he's restored the
    // original state and so no change has been done; but no harm done to have them all
    // cleared together here for documentation's purposes
	CLayout* pLayout = gpApp->m_pLayout;
	pLayout->m_bViewParamsChanged = FALSE;
	pLayout->m_bUSFMChanged = FALSE;
	pLayout->m_bFilteringChanged = FALSE;
	pLayout->m_bPunctuationChanged = FALSE;
	pLayout->m_bCaseEquivalencesChanged = FALSE;
	pLayout->m_bFontInfoChanged = FALSE;

	m_bDismissDialog = TRUE;

	// Initialize each page of the notebook; each page's InitDialog() is not called when
	// the page is created in CEditPreferencesDlg's CreateControls() method, so we insure
	// they get called here in CEditPreferencesDlg's own InitDialog() method.
	fontPage->InitDialog(event);
	punctMapPage->InitDialog(event);
	caseEquivPage->InitDialog(event);
	kbPage->InitDialog(event);
	viewPage->InitDialog(event);
	autoSavePage->InitDialog(event);
	unitsPage->InitDialog(event);
	usfmPage->InitDialog(event);
	filterPage->InitDialog(event);

	wxASSERT(pNotebook != NULL);

	// set the fontPage as the tab showing
	int curSelPg;
	curSelPg = pNotebook->ChangeSelection(0); // disregard returned previous selection int
	// See the AddPage() calls in the CreateControls() method above - fontPage should be
	// the selected tab.
	if (pNotebook->GetPageCount() >= 4)
	{
		if (gbIsGlossing)
		{
			// Remove Punctuation page from Edit Preferences notebook when Glossing box is checked
			if (pNotebook->GetPageText(4) == _("Punctuation"))
			{
				wxMessageBox(_("Note: The Edit Preferences \"Punctuation\" Tab is not available\nwhen the Glossing box is checked on the control bar."),_T(""),wxICON_INFORMATION); 
				pNotebook->RemovePage(4);
			}
		}
		else
		{
			// Since the Edit Preferences dialog is recreated each time it is called the
			// should always initially be present and the following if block never execute. 
			// Insert Punctuation page into Edit Preferences notebook if it was previously removed
			if (pNotebook->GetPageText(4) != _("Punctuation"))
			{
				pNotebook->InsertPage(4, punctMapPage, _("Punctuation"), FALSE);
			}
		}
	}
}// end of InitDialog()

void CEditPreferencesDlg::OnOK(wxCommandEvent& event)
{
	// Check to insure the font facenames aren't blank.
	// On Windows, at least, the font dialog won't return
	// any name other than one that is actually selected
	// from the font list in the dialog. This seems to be
	// true even if the user deletes the face name or enters
	// a bogus name then Ok. The string returned from the
	// dialog will be the previous font name it was 
	// originally initialized to. I'm not sure, however,
	// how other platforms will react to such user errors, 
	// so validation code here is for safety.

	// Validate the font data and veto the OnOK
	// if the font face names are blank or the font sizes
	// are not within the 6pt to 72pt range. 
	// We should always offer to substitute some default 
	// valid value until the user enters valid ones or cancels.

	// Validate fontPage data
	// Don't accept blank face names for any of the 3 fonts
	wxASSERT(pNotebook != NULL);
	if (fontPage->fontPgCommon.pSrcFontNameBox->GetValue().IsEmpty())
	{
		pNotebook->SetSelection(0); // Font tab is first in EditPreferencesDlg notebook
		wxMessageBox(_("Sorry, the source font name cannot be left blank."), _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pSrcFontNameBox->SetFocus();
		return;
	}
	if (fontPage->fontPgCommon.pTgtFontNameBox->GetValue().IsEmpty())
	{
		pNotebook->SetSelection(0);
		wxMessageBox(_("Sorry, the target font name cannot be left blank."), _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pTgtFontNameBox->SetFocus();
		return;
	}
	if (fontPage->fontPgCommon.pNavFontNameBox->GetValue().IsEmpty())
	{
		pNotebook->SetSelection(0);
		wxMessageBox(_("Sorry, the navigation font name cannot be left blank."), _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pNavFontNameBox->SetFocus();
		return;
	}

	// Don't accept font sizes outside the 6pt to 72pt range
	int fSize;
	wxString strTemp, subStr, msg;
	strTemp = fontPage->fontPgCommon.pSrcFontSizeBox->GetValue();
	fSize = wxAtoi(strTemp);
	subStr = _("Sorry, the %s font size must be between %d and %d points. A default size of 12 points will be used instead.");
	if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
	{
		msg = msg.Format(subStr,_("source"),MIN_FONT_SIZE,MAX_FONT_SIZE);
		pNotebook->SetSelection(0);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pSrcFontSizeBox->SetFocus();
		fontPage->fontPgCommon.pSrcFontSizeBox->SetValue(_T("12"));
		return;
	}
	strTemp = fontPage->fontPgCommon.pTgtFontSizeBox->GetValue();
	fSize = wxAtoi(strTemp);
	if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
	{
		msg = msg.Format(subStr,_("target"),MIN_FONT_SIZE,MAX_FONT_SIZE);
		pNotebook->SetSelection(0);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pTgtFontSizeBox->SetValue(_T("12"));
		fontPage->fontPgCommon.pTgtFontSizeBox->SetFocus();
		return;
	}
	strTemp = fontPage->fontPgCommon.pNavFontSizeBox->GetValue();
	fSize = wxAtoi(strTemp);
	if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
	{
		msg = msg.Format(subStr,_("navigation"),MIN_FONT_SIZE,MAX_FONT_SIZE);
		pNotebook->SetSelection(0);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		fontPage->fontPgCommon.pNavFontSizeBox->SetValue(_T("12"));
		fontPage->fontPgCommon.pNavFontSizeBox->SetFocus();
		fontPage->fontPgCommon.tempNavTextSize = 12;
		return;
	}
	// Validate kbPage data
	// MFC version DataExchange limits the m_strSrcName and m_strTgtName to 
	// 64 chars each
	subStr = _("Sorry, the %s language name should be limited to\nno more than 64 characters in length. Please type a shorter name.");
	if (kbPage->m_pEditSrcName->GetValue().Length() > 64)
	{
		msg = msg.Format(subStr, _("source"));
		pNotebook->SetSelection(1);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		kbPage->m_pEditSrcName->SetFocus();
		return;
	}
	if (kbPage->m_pEditTgtName->GetValue().Length() > 64)
	{
		msg = msg.Format(subStr, _("target"));
		pNotebook->SetSelection(1);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		kbPage->m_pEditTgtName->SetFocus();
		return;
	}

	// Validate viewPage data
	int intTemp;
	subStr = _("Sorry, the %s value must be between %d and %d.\nPlease type a value within that range.");
	// refactored 26Apr09 - this item is no longer needed
	//strTemp = viewPage->m_pEditMaxSrcWordsDisplayed->GetValue();
	//intTemp = wxAtoi(strTemp);
	//if (intTemp < 60 || intTemp > 4000)
	//{
	//	msg = msg.Format(subStr,_("maximum number of source words"),60,4000);
	//	pNotebook->SetSelection(2);
	//	wxMessageBox(msg, _T(""), wxICON_INFORMATION);
	//	viewPage->m_pEditMaxSrcWordsDisplayed->SetFocus();
	//	return;
	//}
	// refactored 26Apr09 - this item is no longer needed
	//strTemp = viewPage->m_pEditMinPrecContext->GetValue();
	//intTemp = wxAtoi(strTemp);
	//if (intTemp < 20 || intTemp > 80)
	//{
	//	msg = msg.Format(subStr,_("minimum number of words in the preceding context"),20,80);
	//	pNotebook->SetSelection(2);
	//	wxMessageBox(msg, _T(""), wxICON_INFORMATION);
	//	viewPage->m_pEditMinPrecContext->SetFocus();
	//	return;
	//}
	// refactored 26Apr09 - this item is no longer needed
	//strTemp = viewPage->m_pEditMinFollContext->GetValue();
	//intTemp = wxAtoi(strTemp);
	//if (intTemp < 20 || intTemp > 60)
	//{
	//	msg = msg.Format(subStr,_("minimum number of words in the following context"),20,60);
	//	pNotebook->SetSelection(2);
	//	wxMessageBox(msg, _T(""), wxICON_INFORMATION);
	//	viewPage->m_pEditMinFollContext->SetFocus();
	//	return;
	//}
	strTemp = viewPage->m_pEditLeading->GetValue();
	intTemp = wxAtoi(strTemp);
	if (intTemp < 14 || intTemp > 80)
	{
		msg = msg.Format(subStr,_("vertical gap between text strips"),14,80);
		pNotebook->SetSelection(2);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		viewPage->m_pEditLeading->SetFocus();
		return;
	}
	strTemp = viewPage->m_pEditGapWidth->GetValue();
	intTemp = wxAtoi(strTemp);
	if (intTemp < 6 || intTemp > 40)
	{
		msg = msg.Format(subStr,_("inter-pile gap width"),6,40);
		pNotebook->SetSelection(2);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		viewPage->m_pEditGapWidth->SetFocus();
		return;
	}
	strTemp = viewPage->m_pEditMultiplier->GetValue();
	intTemp = wxAtoi(strTemp);
	if (intTemp < 5 || intTemp > 30)
	{
		msg = msg.Format(subStr,_("expansion multiplier"),5,30);
		pNotebook->SetSelection(2);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		viewPage->m_pEditMultiplier->SetFocus();
		return;
	}

	// Validate autoSavePage data
	strTemp = autoSavePage->m_pEditMinutes->GetValue();
	intTemp = wxAtoi(strTemp);
	if (intTemp < 1 || intTemp > 30)
	{
		msg = msg.Format(subStr,_("number of minutes value"),1,30);
		pNotebook->SetSelection(3);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		autoSavePage->m_pEditMinutes->SetFocus();
		return;
	}
	strTemp = autoSavePage->m_pEditMoves->GetValue();
	intTemp = wxAtoi(strTemp);
	if (intTemp < 10 || intTemp > 1000)
	{
		msg = msg.Format(subStr,_("number of moves value"),10,1000);
		pNotebook->SetSelection(3);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		autoSavePage->m_pEditMoves->SetFocus();
		return;
	}
	strTemp = autoSavePage->m_pEditKBMinutes->GetValue();
	intTemp = wxAtoi(strTemp); 
	if (intTemp < 2 || intTemp > 60)
	{
		msg = msg.Format(subStr,_("number of minutes"),2,60);
		pNotebook->SetSelection(3);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		autoSavePage->m_pEditKBMinutes->SetFocus();
		return;
	}

	// Validate punctMapPage data
	subStr = _("Sorry, this %s language punctuation edit box cannot have more than 4 punctuation characters.");
	for (int i = 0; i < MAXPUNCTPAIRS; i++)
	{
		if (punctMapPage->punctPgCommon.m_editSrcPunct[i]->GetValue().Length() > 4)
		{
			msg = msg.Format(subStr,_("source"));
			pNotebook->SetSelection(4);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION);
			punctMapPage->punctPgCommon.m_editSrcPunct[i]->SetFocus();
			return;
		}
		if (punctMapPage->punctPgCommon.m_editTgtPunct[i]->GetValue().Length() > 4)
		{
			msg = msg.Format(subStr,_("target"));
			pNotebook->SetSelection(4);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION);
			punctMapPage->punctPgCommon.m_editTgtPunct[i]->SetFocus();
			return;
		}
	}
	subStr = _("Sorry, this %s language punctuation edit box cannot have more than 9 punctuation characters.");
	for (int i = 0; i < MAXTWOPUNCTPAIRS; i++)
	{
		if (punctMapPage->punctPgCommon.m_editSrcPunct[i]->GetValue().Length() > 9)
		{
			msg = msg.Format(subStr,_("source"));
			pNotebook->SetSelection(4);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION);
			punctMapPage->punctPgCommon.m_editSrcPunct[i]->SetFocus();
			return;
		}
		if (punctMapPage->punctPgCommon.m_editTgtPunct[i]->GetValue().Length() > 9)
		{
			msg = msg.Format(subStr,_("target"));
			pNotebook->SetSelection(4);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION);
			punctMapPage->punctPgCommon.m_editTgtPunct[i]->SetFocus();
			return;
		}
	}

	// Validate caseEquivPage data
	subStr = _("Sorry, the list of %s language case correspondences should be limited to\nno more than 180 characters in length. Please type a shorter list.");
	if (caseEquivPage->casePgCommon.m_pEditSrcEquivalences->GetValue().Length() > 180)
	{
		msg = msg.Format(subStr, _("source"));
		pNotebook->SetSelection(5);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		caseEquivPage->casePgCommon.m_pEditSrcEquivalences->SetFocus();
		return;
	}
	if (caseEquivPage->casePgCommon.m_pEditTgtEquivalences->GetValue().Length() > 180)
	{
		msg = msg.Format(subStr, _("target"));
		pNotebook->SetSelection(5);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		caseEquivPage->casePgCommon.m_pEditTgtEquivalences->SetFocus();
		return;
	}
	if (caseEquivPage->casePgCommon.m_pEditGlossEquivalences->GetValue().Length() > 180)
	{
		msg = msg.Format(subStr, _("gloss"));
		pNotebook->SetSelection(5);
		wxMessageBox(msg, _T(""), wxICON_INFORMATION);
		caseEquivPage->casePgCommon.m_pEditGlossEquivalences->SetFocus();
		return;
	}

	// Validate unitsPage data
	// No validation needed for unitsPage

	// Validate usfmPage data
	// TODO: determine any validation necessary

	// Validate filterPage data
	// TODO: determine any validation necessary

	// Call the OnOK() methods of each notebook page
	fontPage->OnOK(event);
	punctMapPage->OnOK(event);
	caseEquivPage->OnOK(event);
	kbPage->OnOK(event);
	viewPage->OnOK(event);
	autoSavePage->OnOK(event);
	unitsPage->OnOK(event);
	usfmPage->OnOK(event);
	filterPage->OnOK(event);

	if (m_bDismissDialog)
		event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event);

}// end of OnOK

// The following are wrapper handlers for the fontPage
void CEditPreferencesDlg::OnSourceFontChangeBtn(wxCommandEvent& event) // top right Change... button
{
	fontPage->OnSourceFontChangeBtn(event);
}

void CEditPreferencesDlg::OnTargetFontChangeBtn(wxCommandEvent& event) // middle right Change... button
{
	fontPage->OnTargetFontChangeBtn(event);
}

void CEditPreferencesDlg::OnNavTextFontChangeBtn(wxCommandEvent& event) // lower right Change... button
{
	fontPage->OnNavTextFontChangeBtn(event);
}

void CEditPreferencesDlg::OnButtonSpecTextColor(wxCommandEvent& event) // bottom left button
{
	fontPage->OnButtonSpecTextColor(event);
}

void CEditPreferencesDlg::OnButtonRetranTextColor(wxCommandEvent& event) // bottom center button
{
	fontPage->OnButtonRetranTextColor(event);
}

void CEditPreferencesDlg::OnButtonNavTextColor(wxCommandEvent& event) // bottom right button
{
	fontPage->OnButtonNavTextColor(event);
}

// added in version 2.3.0
void CEditPreferencesDlg::OnButtonSourceTextColor(wxCommandEvent& event) 
{
	fontPage->OnButtonSourceTextColor(event);
}

// added in version 2.3.0
void CEditPreferencesDlg::OnButtonTargetTextColor(wxCommandEvent& event) 
{
	fontPage->OnButtonTargetTextColor(event);
}

#ifdef _UNICODE
// The following are wrapper handlers for the punctMapPage
void CEditPreferencesDlg::OnBnClickedToggleUnnnn(wxCommandEvent& event)
{
	punctMapPage->punctPgCommon.OnBnClickedToggleUnnnn(event);
}
#endif

// The following are wrapper handlers for the caseEquivPage
void CEditPreferencesDlg::OnBnClickedClearSrcList(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedClearSrcList(event);
}

void CEditPreferencesDlg::OnBnClickedSrcSetEnglish(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedSrcSetEnglish(event);
}

void CEditPreferencesDlg::OnBnClickedSrcCopyToNext(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedSrcCopyToNext(event);
}

void CEditPreferencesDlg::OnBnClickedSrcCopyToGloss(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedSrcCopyToGloss(event);
}

void CEditPreferencesDlg::OnBnClickedClearTgtList(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedClearTgtList(event);
}

void CEditPreferencesDlg::OnBnClickedTgtSetEnglish(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedTgtSetEnglish(event);
}

void CEditPreferencesDlg::OnBnClickedTgtCopyToNext(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedTgtCopyToNext(event);
}

void CEditPreferencesDlg::OnBnClickedClearGlossList(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedClearGlossList(event);
}

void CEditPreferencesDlg::OnBnClickedGlossSetEnglish(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedGlossSetEnglish(event);
}

void CEditPreferencesDlg::OnBnClickedGlossCopyToNext(wxCommandEvent& event)
{
	caseEquivPage->OnBnClickedGlossCopyToNext(event);
}

void CEditPreferencesDlg::OnBnCheckedSrcHasCaps(wxCommandEvent& event)
{
	caseEquivPage->OnBnCheckedSrcHasCaps(event);
}

void CEditPreferencesDlg::OnBnCheckedUseAutoCaps(wxCommandEvent& event)
{
	caseEquivPage->OnBnCheckedUseAutoCaps(event);
}

// The following are wrapper handlers for the kbPage
void CEditPreferencesDlg::OnCheckKbBackup(wxCommandEvent& event) 
{
	kbPage->OnCheckKbBackup(event);
}

void CEditPreferencesDlg::OnCheckBakupDoc(wxCommandEvent& event) 
{
	kbPage->OnCheckBakupDoc(event);
	
}

// The following are wrapper handlers for the viewPage
void CEditPreferencesDlg::OnButtonHighlightColor(wxCommandEvent& event) 
{
	viewPage->OnButtonHighlightColor(event);
}

// The following are wrapper handlers for the autoSavePage
void CEditPreferencesDlg::OnCheckNoAutoSave(wxCommandEvent& event)
{
	autoSavePage->OnCheckNoAutoSave(event);
}

void CEditPreferencesDlg::OnRadioByMinutes(wxCommandEvent& event) 
{
	autoSavePage->OnRadioByMinutes(event);
}

void CEditPreferencesDlg::OnRadioByMoves(wxCommandEvent& event) 
{
	autoSavePage->OnRadioByMoves(event);
}

void CEditPreferencesDlg::EnableAll(bool bEnable)
{
	autoSavePage->EnableAll(bEnable);
}

// The following are wrapper handlers for the unitsPage
void CEditPreferencesDlg::OnRadioUseInches(wxCommandEvent& event) 
{	
	unitsPage->OnRadioUseInches(event);
}

void CEditPreferencesDlg::OnRadioUseCentimeters(wxCommandEvent& event) 
{
	unitsPage->OnRadioUseCentimeters(event);
}

// The following are wrapper handlers for the usfmPage
void CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseBothSetsDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseUbsSetOnlyProj(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseBothSetsProj(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseUbsSetOnlyFactory(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseSilpngSetOnlyFactory(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseBothSetsFactory(wxCommandEvent& event)
{
	usfmPage->OnBnClickedRadioUseBothSetsFactory(event);
}

void CEditPreferencesDlg::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfmPage->OnBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}

// The following are wrapper handlers for the filterPage
void CEditPreferencesDlg::OnLbnSelchangeListSfmsDoc(wxCommandEvent& event)
{
	filterPage->OnLbnSelchangeListSfmsDoc(event);
}

// This handler is invoked when an item in the Doc check list box is checked or unchecked
void CEditPreferencesDlg::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	filterPage->OnCheckListBoxToggleDoc(event);
}

// This handler is invoked when an item in the Proj check list box is checked or unchecked
void CEditPreferencesDlg::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	filterPage->OnCheckListBoxToggleProj(event);
}

// This handler is invoked when user tries to check or uncheck an item in the Proj check list box
void CEditPreferencesDlg::OnCheckListBoxToggleFactory(wxCommandEvent& event)
{
	filterPage->OnCheckListBoxToggleFactory(event);
}

void CEditPreferencesDlg::OnLbnSelchangeListSfmsProj(wxCommandEvent& event)
{
	filterPage->OnLbnSelchangeListSfmsProj(event);
}

void CEditPreferencesDlg::OnLbnSelchangeListSfmsFactory(wxCommandEvent& event)
{
	filterPage->OnLbnSelchangeListSfmsFactory(event);
}
