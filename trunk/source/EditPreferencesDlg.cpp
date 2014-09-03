/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EditPreferencesDlg.cpp
/// \author			Bill Martin
/// \date_created	13 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CEditPreferencesDlg class. 
/// The CEditPreferencesDlg class acts as a dialog wrapper for the tab pages of
/// an "Edit Preferences" wxNotebook. The interface resources for the wxNotebook 
/// dialog are defined in EditPreferencesDlgFunc(), which was created and is 
/// maintained by wxDesigner. The notebook contains nine tabs labeled "Fonts", 
/// "Backups and KB", "View", "Auto-Saving", "Punctuation", "Case", "Units", and
/// "USFM and Filtering" depending on the current user workflow profile selected.
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
#include <wx/display.h> // for wxDisplay

#include "Adapt_It.h" // for access to extern fontInfo structs below

#if wxCHECK_VERSION(2,9,0)
	// Use the built-in scrolling dialog features available in wxWidgets 2.9.x
#include <wx/propdlg.h> // whm 8Jun12 Note: this now includes wxScrollingPropertyScheetDialog features in 2.9.3
#else
	// The wxWidgets library being used is pre-2.9.x, so use our own modified
	// version named wxScrollingDialog located in scrollingdialog.h
#include "scrollingdialog.h"
#endif

#include "EditPreferencesDlg.h"
#include "Adapt_It_Resources.h"
#include "Adapt_ItDoc.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "ToolbarPage.h"
#include "CaseEquivPage.h"
#include "KBPage.h"
#include "ViewPage.h"
#include "AutoSavingPage.h"
#include "UnitsPage.h"
#include "UsfmFilterPage.h"
#include "helpers.h"
#include "Pile.h"
#include "Layout.h"

//extern wxChar gSFescapechar;
//extern bool gbSfmOnlyAfterNewlines;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing;

/// This global is defined in Adapt_It.cpp.
extern CUsfmFilterPagePrefs* pUsfmFilterPageInPrefs; // set the App's pointer to the filterPage

/// This global is defined in Adapt_ItView.cpp.
extern CAdapt_ItApp* gpApp;

// For temporary emulation of MFC's Logfont structs. These
// may disappear if/when we remove Logfont member values from
// our config files.
extern struct fontInfo SrcFInfo, TgtFInfo, NavFInfo;

//IMPLEMENT_DYNAMIC_CLASS(CEditPreferencesDlg, wxPropertySheetDialog)

// event handler table
BEGIN_EVENT_TABLE(CEditPreferencesDlg, wxPropertySheetDialog)
	EVT_INIT_DIALOG(CEditPreferencesDlg::InitDialog)
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
	// The following wrapper handlers are for the ToolbarPage
	EVT_RADIOBUTTON(ID_RDO_TOOLBAR_SMALL, CEditPreferencesDlg::OnRadioToolbarSmall)
	EVT_RADIOBUTTON(ID_RDO_TOOLBAR_MEDIUM, CEditPreferencesDlg::OnRadioToolbarMedium)
	EVT_RADIOBUTTON(ID_RDO_TOOLBAR_LARGE, CEditPreferencesDlg::OnRadioToolbarLarge)
	EVT_CHOICE(ID_CBO_TOOLBAR_ICON, CEditPreferencesDlg::OnCboToolbarIcon)
	EVT_BUTTON(ID_BTN_TOOLBAR_MINIMAL, CEditPreferencesDlg::OnBnToolbarMinimal)
	EVT_BUTTON(ID_TOOLBAR_RESET, CEditPreferencesDlg::OnBnToolbarReset)
	EVT_LIST_ITEM_SELECTED(ID_LST_TOOLBAR_BUTTONS, CEditPreferencesDlg::OnClickLstToolbarButtons)
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
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES, CEditPreferencesDlg::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
	// The following wrapper handlers are for the filterPage
	EVT_LISTBOX(IDC_LIST_SFMS, CEditPreferencesDlg::OnLbnSelchangeListSfmsDoc)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS, CEditPreferencesDlg::OnCheckListBoxToggleDoc)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CEditPreferencesDlg::OnLbnSelchangeListSfmsProj)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_PROJ, CEditPreferencesDlg::OnCheckListBoxToggleProj)
END_EVENT_TABLE()

CEditPreferencesDlg::CEditPreferencesDlg()
{
}

CEditPreferencesDlg::CEditPreferencesDlg(
	wxWindow* parent, wxWindowID id, const wxString& title,
	const wxPoint& pos, const wxSize& size,
	long style)
{
	fontPage = (CFontPagePrefs*)NULL;
	punctMapPage = (CPunctCorrespPagePrefs*)NULL;
	toolbarPage = (CToolbarPagePrefs*)NULL;
	caseEquivPage = (CCaseEquivPagePrefs*)NULL;
	kbPage = (CKBPage*)NULL;
	viewPage = (CViewPage*)NULL;
	autoSavePage = (CAutoSavingPage*)NULL;
	unitsPage = (CUnitsPage*)NULL;
	usfmFilterPage = (CUsfmFilterPagePrefs*)NULL;
	
	Create(parent, id, title, pos, size, style);
}

bool CEditPreferencesDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP);
    wxPropertySheetDialog::Create( parent, id, caption, pos, size, style );

    CreateButtons(wxOK|wxCANCEL); //|wxHELP);
	// whm note: the wxPropertySheetDialog has internal smarts to reverse the order of the
	// OK and Cancel buttons on creation, so we don't need to call the App's 
	// ReverseOkCancelButtonsForMac() function.
    CreateControls();
    LayoutDialog();
    Centre();
    return true;
}

void CEditPreferencesDlg::CreateControls()
{    
	pNotebook = GetBookCtrl(); // gets pointer to the default wxNotebook that contains the tab pages

	// Create the pages for the notebook of the property sheet and add 
	// pages to the notebook. Get the largest minimum page size needed 
	// for all pages of the prefs to display fully
	wxSize neededSize;

	if (TabIsVisibleInCurrentProfile(_("Fonts")))
	{
		fontPage = new CFontPagePrefs(pNotebook);
		wxASSERT(fontPage != NULL);
		pNotebook->AddPage( fontPage, _("Fonts"),TRUE ); // TRUE = page should be selected
		wxSize fontPageSize = fontPage->GetSize();
		if (fontPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(fontPageSize.GetX());
		if (fontPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(fontPageSize.GetY());
	}
	
	if (TabIsVisibleInCurrentProfile(_("Punctuation")))
	{
		punctMapPage = new CPunctCorrespPagePrefs(pNotebook);
		wxASSERT(punctMapPage != NULL);
		pNotebook->AddPage( punctMapPage, _("Punctuation"),FALSE );
		wxSize punctMapPageSize = punctMapPage->GetSize();
		if (punctMapPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(punctMapPageSize.GetX());
		if (punctMapPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(punctMapPageSize.GetY());
	}

	if (TabIsVisibleInCurrentProfile(_("Toolbar")))
	{
		toolbarPage = new CToolbarPagePrefs(pNotebook);
		wxASSERT(toolbarPage != NULL);
		pNotebook->AddPage( toolbarPage, _("Toolbar"),FALSE );
		wxSize toolbarPageSize = toolbarPage->GetSize();
		if (toolbarPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(toolbarPageSize.GetX());
		if (toolbarPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(toolbarPageSize.GetY());
	}
	
	if (TabIsVisibleInCurrentProfile(_("Case")))
	{
		caseEquivPage = new CCaseEquivPagePrefs(pNotebook);
		wxASSERT(caseEquivPage != NULL);
		pNotebook->AddPage( caseEquivPage, _("Case"),FALSE );
		wxSize caseEquivPageSize = caseEquivPage->GetSize();
		if (caseEquivPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(caseEquivPageSize.GetX());
		if (caseEquivPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(caseEquivPageSize.GetY());
	}
		
	if (TabIsVisibleInCurrentProfile(_("Backups and Misc")))
	{
		kbPage = new CKBPage(pNotebook);
		wxASSERT(kbPage != NULL);
		pNotebook->AddPage( kbPage, _("Backups and Misc"),FALSE ); // was "Backups and KB" in legacy app
		wxSize kbPageSize = kbPage->GetSize();
		if (kbPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(kbPageSize.GetX());
		if (kbPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(kbPageSize.GetY());
	}
		
	if (TabIsVisibleInCurrentProfile(_("View")))
	{
		viewPage = new CViewPage(pNotebook);
		wxASSERT(viewPage != NULL);
		pNotebook->AddPage( viewPage, _("View"),FALSE );
		wxSize viewPageSize = viewPage->GetSize();
		if (viewPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(viewPageSize.GetX());
		if (viewPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(viewPageSize.GetY());
	}
		
	if (TabIsVisibleInCurrentProfile(_("Auto-Saving")))
	{
		autoSavePage = new CAutoSavingPage(pNotebook);
		wxASSERT(autoSavePage != NULL);
		pNotebook->AddPage( autoSavePage, _("Auto-Saving"),FALSE );
		wxSize autoSavePageSize = autoSavePage->GetSize();
		if (autoSavePageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(autoSavePageSize.GetX());
		if (autoSavePageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(autoSavePageSize.GetY());
	}
		
	if (TabIsVisibleInCurrentProfile(_("Units")))
	{
		unitsPage = new CUnitsPage(pNotebook);
		wxASSERT(unitsPage != NULL);
		pNotebook->AddPage( unitsPage, _("Units"),FALSE );
		wxSize unitsPageSize = unitsPage->GetSize();
		if (unitsPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(unitsPageSize.GetX());
		if (unitsPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(unitsPageSize.GetY());
	}
		
	if (TabIsVisibleInCurrentProfile(_("USFM and Filtering")) 
		// BEW 3Sep14 removed the collaboration subtest, we want to allow filtering or unfiltering
		// when collaboration is ON, because filtering footnotes & later unfiltering of them are
		// the kind of things which need to be done easily when collaborating
		//&& (!gpApp->m_bCollaboratingWithParatext && !gpApp->m_bCollaboratingWithBibledit)
		)
	{
		usfmFilterPage = new CUsfmFilterPagePrefs(pNotebook);
		pUsfmFilterPageInPrefs = usfmFilterPage; // set the App's pointer to the usfmPage
		wxASSERT(usfmFilterPage != NULL);
		pNotebook->AddPage( usfmFilterPage, _("USFM and Filtering"),FALSE );
		wxSize usfmPageSize = usfmFilterPage->GetSize();
		if (usfmPageSize.GetX() > neededSize.GetX()) neededSize.SetWidth(usfmPageSize.GetX());
		if (usfmPageSize.GetY() > neededSize.GetY()) neededSize.SetWidth(usfmPageSize.GetY());
	}

	// Make the pPunctCorrespPageWiz and the pCaseEquivPageWiz
	// scrollable if we are on a small screen

	/*
	// This code below is now unneeded with the use of the special wxScrolledWizard class.
	// 
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
			// whm added 5Nov11 to make sure that the width is not zero in the case where
			// this code block senses that there is not enough vertical height in the
			// screen's displaySize.
			maxSz.SetWidth(displaySize.GetWidth() - 50);
			this->SetMaxSize(maxSz);
		}
	}
	// The wxNotebook will automatically adjust to the largest page if the conditional block above
	// doesn't execute.
	*/
}



CEditPreferencesDlg::~CEditPreferencesDlg(void)
{
	/*
	// whm 22Oct11 removed. The tab pages of pNotebook should be destroyed
	// by the pNotebook itself since pNotebook->AddPage() was called in the
	// CreateControls() function.
	// 
	// When a tab page was not visible it was not added to 
	// the wxNotebook. It's memory therefore needs to be 
	// deallocated here in the destructor.
	if (!TabIsVisibleInCurrentProfile(_("Fonts")))
	{
		if (fontPage != NULL) // whm 11Jun12 added NULL test
			delete fontPage;
	}
	if (!TabIsVisibleInCurrentProfile(_("Punctuation")))
	{
		if (punctMapPage != NULL) // whm 11Jun12 added NULL test
			delete punctMapPage;
	}
	if (!TabIsVisibleInCurrentProfile(_("Case")))
	{
		if (caseEquivPage != NULL) // whm 11Jun12 added NULL test
			delete caseEquivPage;
	}
	if (!TabIsVisibleInCurrentProfile(_("Backups and Misc")))
	{
		if (kbPage != NULL) // whm 11Jun12 added NULL test
			delete kbPage;
	}
	if (!TabIsVisibleInCurrentProfile(_("View")))
	{
		if (viewPage != NULL) // whm 11Jun12 added NULL test
			delete viewPage;
	}
	if (!TabIsVisibleInCurrentProfile(_("Auto-Saving")))
	{
		if (autoSavePage != NULL) // whm 11Jun12 added NULL test
			delete autoSavePage;
	}
	if (!TabIsVisibleInCurrentProfile(_("Units")))
	{
		if (unitsPage != NULL) // whm 11Jun12 added NULL test
			delete unitsPage;
	}
	if (TabIsVisibleInCurrentProfile(_("USFM and Filtering")) 
		&& (!gpApp->m_bCollaboratingWithParatext && !gpApp->m_bCollaboratingWithBibledit))
	{
		if (usfmFilterPage != NULL) // whm 11Jun12 added NULL test
			delete usfmFilterPage;
	}
	*/
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
	pLayout->m_bToolbarChanged = FALSE;
	pLayout->m_bCaseEquivalencesChanged = FALSE;
	pLayout->m_bFontInfoChanged = FALSE;

	m_bDismissDialog = TRUE;

	// Initialize each page of the notebook; each page's InitDialog() is not called when
	// the page is created in CEditPreferencesDlg's CreateControls() method, so we ensure
	// they get called here in CEditPreferencesDlg's own InitDialog() method.
	// whm 5Oct10 note: Under user profiles, even when certain tab pages are not added
	// to the wxNotebook, they still exist and some (in particular the usfmPage and the
	// filterPage) interact, so we still call the InitDialog() handlers for all pages
	// so that any necessary initializations are done even though the page may not be
	// visible.
	if (TabIsVisibleInCurrentProfile(_("Fonts")))
	{
		fontPage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("Punctuation")))
	{
		punctMapPage->InitDialog(event);
	}
//	if (TabIsVisibleInCurrentProfile(_("Toolbar")))
	{
		toolbarPage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("Case")))
	{
		caseEquivPage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("Backups and Misc")))
	{
		kbPage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("View")))
	{
		viewPage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("Auto-Saving")))
	{
		autoSavePage->InitDialog(event);
	}
	if (TabIsVisibleInCurrentProfile(_("Units")))
	{
		unitsPage->InitDialog(event);
	}
	// whm 22Aug11 modified - don't initialize the USFM and Filtering tab when collaborating with PT/BE
	if (TabIsVisibleInCurrentProfile(_("USFM and Filtering")) 
		&& (!gpApp->m_bCollaboratingWithParatext && !gpApp->m_bCollaboratingWithBibledit))
	{
		usfmFilterPage->InitDialog(event);
	}

	wxASSERT(pNotebook != NULL);

	// set the fontPage as the tab showing
	//int curSelPg;
	//curSelPg = pNotebook->ChangeSelection(0); // disregard returned previous selection int
	// See the AddPage() calls in the CreateControls() method above - fontPage should be
	// the selected tab.
	// whm 5Oct10 modified to accommodate user profiles in which one or more tabs were not added
	// to the wxNotebook control. We no longer necessarily have a fixed number of tab pages, so
	// we scan through the existing pages to find the "Punctuation" page and remove it if glossing.
	int pgCt;
	int totCt = pNotebook->GetPageCount();
	wxString pgText;
	for (pgCt = 0; pgCt < totCt; pgCt++)
	{
		pgText = pNotebook->GetPageText(pgCt);
		if (gbIsGlossing && pgText == _("Punctuation"))
		{
			// Remove Punctuation page from Edit Preferences notebook when Glossing box is checked
			wxMessageBox(_("Note: The Edit Preferences \"Punctuation\" Tab is not available\nwhen the Glossing box is checked on the control bar."),_T(""),wxICON_INFORMATION | wxOK); 
			pNotebook->RemovePage(pgCt);
			break;
		}
	}
}// end of InitDialog()

void CEditPreferencesDlg::OnOK(wxCommandEvent& event)
{
	// Check to ensure the font facenames aren't blank.
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
	// 
	// Use these enum symbols for the page selections
	// enum PrefsPageIndices {
	//    fontsPageIndex = 0,
	//    punctuationPageIndex,
	//    toolbarPageIndex,
	//    casePageIndex,
	//    kbPageIndex, // the 'Backups and KB" page
	//    viewPageIndex,
	//    autosavePageIndex,
	//    unitsPageIndex,
	//    filterPageIndex
	// };
	wxASSERT(pNotebook != NULL);
	wxString strTemp, subStr, msg;
	if (fontPage != NULL)
	{
		if (fontPage->fontPgCommon.pSrcFontNameBox->GetValue().IsEmpty())
		{
			pNotebook->SetSelection(fontsPageIndex); // Font tab is first in EditPreferencesDlg notebook
			wxMessageBox(_("Sorry, the source font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pSrcFontNameBox->SetFocus();
			return;
		}
		if (fontPage->fontPgCommon.pTgtFontNameBox->GetValue().IsEmpty())
		{
			pNotebook->SetSelection(fontsPageIndex);
			wxMessageBox(_("Sorry, the target font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pTgtFontNameBox->SetFocus();
			return;
		}
		if (fontPage->fontPgCommon.pNavFontNameBox->GetValue().IsEmpty())
		{
			pNotebook->SetSelection(fontsPageIndex);
			wxMessageBox(_("Sorry, the navigation font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pNavFontNameBox->SetFocus();
			return;
		}

		// Don't accept font sizes outside the 6pt to 72pt range
		int fSize;
		strTemp = fontPage->fontPgCommon.pSrcFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp);
		subStr = _("Sorry, the %s font size must be between %d and %d points. A default size of 12 points will be used instead.");
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_("source"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			pNotebook->SetSelection(fontsPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pSrcFontSizeBox->SetFocus();
			fontPage->fontPgCommon.pSrcFontSizeBox->SetValue(_T("12"));
			return;
		}
		strTemp = fontPage->fontPgCommon.pTgtFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp);
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_("target"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			pNotebook->SetSelection(fontsPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pTgtFontSizeBox->SetValue(_T("12"));
			fontPage->fontPgCommon.pTgtFontSizeBox->SetFocus();
			return;
		}
		strTemp = fontPage->fontPgCommon.pNavFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp);
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_("navigation"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			pNotebook->SetSelection(fontsPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPage->fontPgCommon.pNavFontSizeBox->SetValue(_T("12"));
			fontPage->fontPgCommon.pNavFontSizeBox->SetFocus();
			fontPage->fontPgCommon.tempNavTextSize = 12;
			return;
		}
	}

	if (kbPage != NULL)
	{
		// Validate kbPage data
		// MFC version DataExchange limits the m_strSrcName and m_strTgtName to 
		// 64 chars each
		subStr = _("Sorry, the %s language name should be limited to\nno more than 64 characters in length. Please type a shorter name.");
		if (kbPage->m_pEditSrcName->GetValue().Length() > 64)
		{
			msg = msg.Format(subStr, _("source"));
			pNotebook->SetSelection(kbPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			kbPage->m_pEditSrcName->SetFocus();
			return;
		}
		if (kbPage->m_pEditTgtName->GetValue().Length() > 64)
		{
			msg = msg.Format(subStr, _("target"));
			pNotebook->SetSelection(kbPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			kbPage->m_pEditTgtName->SetFocus();
			return;
		}
	}

	int intTemp;
	if (viewPage != NULL)
	{
		// Validate viewPage data
		subStr = _("Sorry, the %s value must be between %d and %d.\nPlease type a value within that range.");
		strTemp = viewPage->m_pEditLeading->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 14 || intTemp > 80)
		{
			msg = msg.Format(subStr,_("vertical gap between text strips"),14,80);
			pNotebook->SetSelection(viewPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			viewPage->m_pEditLeading->SetFocus();
			return;
		}
		strTemp = viewPage->m_pEditGapWidth->GetValue();
		intTemp = wxAtoi(strTemp);
		// BEW 18Nov13 changed 40 to 80 in test, the 80 limit has been set in the config
		// file test for years, but was forgotten to be changed from 40 to 80 here too.
		//if (intTemp < 6 || intTemp > 40)
		if (intTemp < 6 || intTemp > 80)
		{
			//msg = msg.Format(subStr,_("inter-pile gap width"),6,40);
			msg = msg.Format(subStr,_("inter-pile gap width"),6,80);
			pNotebook->SetSelection(viewPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			viewPage->m_pEditGapWidth->SetFocus();
			return;
		}
		// BEW 19Nov13 added next one
		strTemp = viewPage->m_pEditLeftMargin->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 16 || intTemp > 40)
		{
			msg = msg.Format(subStr,_("left margin width"),16,40);
			pNotebook->SetSelection(viewPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			viewPage->m_pEditLeftMargin->SetFocus();
			return;
		}
		strTemp = viewPage->m_pEditMultiplier->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 5 || intTemp > 30)
		{
			msg = msg.Format(subStr,_("expansion multiplier"),5,30);
			pNotebook->SetSelection(viewPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			viewPage->m_pEditMultiplier->SetFocus();
			return;
		}
		// BEW 19Nov13 added next one
		strTemp = viewPage->m_pEditDlgFontSize->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 10 || intTemp > 24)
		{
			msg = msg.Format(subStr,_("font size for dialogs"),10,24);
			pNotebook->SetSelection(viewPageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			viewPage->m_pEditDlgFontSize->SetFocus();
			return;
		}
	}

	if (autoSavePage != NULL)
	{
		// Validate autoSavePage data
		strTemp = autoSavePage->m_pEditMinutes->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 1 || intTemp > 30)
		{
			msg = msg.Format(subStr,_("number of minutes value"),1,30);
			pNotebook->SetSelection(autosavePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			autoSavePage->m_pEditMinutes->SetFocus();
			return;
		}
		strTemp = autoSavePage->m_pEditMoves->GetValue();
		intTemp = wxAtoi(strTemp);
		if (intTemp < 10 || intTemp > 1000)
		{
			msg = msg.Format(subStr,_("number of moves value"),10,1000);
			pNotebook->SetSelection(autosavePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			autoSavePage->m_pEditMoves->SetFocus();
			return;
		}
		strTemp = autoSavePage->m_pEditKBMinutes->GetValue();
		intTemp = wxAtoi(strTemp); 
		if (intTemp < 2 || intTemp > 60)
		{
			msg = msg.Format(subStr,_("number of minutes"),2,60);
			pNotebook->SetSelection(autosavePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			autoSavePage->m_pEditKBMinutes->SetFocus();
			return;
		}
	}

	if (punctMapPage != NULL)
	{
		// Validate punctMapPage data
		subStr = _("Sorry, this %s language punctuation edit box cannot have more than 4 punctuation characters.");
		for (int i = 0; i < MAXPUNCTPAIRS; i++)
		{
			if (punctMapPage->punctPgCommon.m_editSrcPunct[i]->GetValue().Length() > 4)
			{
				msg = msg.Format(subStr,_("source"));
				pNotebook->SetSelection(punctuationPageIndex);
				wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
				punctMapPage->punctPgCommon.m_editSrcPunct[i]->SetFocus();
				return;
			}
			if (punctMapPage->punctPgCommon.m_editTgtPunct[i]->GetValue().Length() > 4)
			{
				msg = msg.Format(subStr,_("target"));
				pNotebook->SetSelection(punctuationPageIndex);
				wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
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
				pNotebook->SetSelection(punctuationPageIndex);
				wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
				punctMapPage->punctPgCommon.m_editSrcPunct[i]->SetFocus();
				return;
			}
			if (punctMapPage->punctPgCommon.m_editTgtPunct[i]->GetValue().Length() > 9)
			{
				msg = msg.Format(subStr,_("target"));
				pNotebook->SetSelection(punctuationPageIndex);
				wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
				punctMapPage->punctPgCommon.m_editTgtPunct[i]->SetFocus();
				return;
			}
		}
	}

	if (toolbarPage != NULL)
	{
		// EDB TODO: validate ToolbarPage data
	}

	if (caseEquivPage != NULL)
	{
		// Validate caseEquivPage data
		subStr = _("Sorry, the list of %s language case correspondences should be limited to\nno more than 180 characters in length. Please type a shorter list.");
		if (caseEquivPage->casePgCommon.m_pEditSrcEquivalences->GetValue().Length() > 180)
		{
			msg = msg.Format(subStr, _("source"));
			pNotebook->SetSelection(casePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			caseEquivPage->casePgCommon.m_pEditSrcEquivalences->SetFocus();
			return;
		}
		if (caseEquivPage->casePgCommon.m_pEditTgtEquivalences->GetValue().Length() > 180)
		{
			msg = msg.Format(subStr, _("target"));
			pNotebook->SetSelection(casePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			caseEquivPage->casePgCommon.m_pEditTgtEquivalences->SetFocus();
			return;
		}
		if (caseEquivPage->casePgCommon.m_pEditGlossEquivalences->GetValue().Length() > 180)
		{
			msg = msg.Format(subStr, _("gloss"));
			pNotebook->SetSelection(casePageIndex);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			caseEquivPage->casePgCommon.m_pEditGlossEquivalences->SetFocus();
			return;
		}
	}

	// Validate unitsPage data
	// No validation needed for unitsPage

	// Validate usfmPage data
	// TODO: determine any validation necessary

	// Validate filterPage data
	// TODO: determine any validation necessary

	// Call the OnOK() methods of each notebook page
	// whm 5Oct10 note: we leave these OnOK() handler calls even though
	// some tab pages may not be visible
	if (fontPage != NULL)
		fontPage->OnOK(event);
	if (punctMapPage != NULL)
		punctMapPage->OnOK(event);
	if (toolbarPage != NULL)
		toolbarPage->OnOK(event);
	if (caseEquivPage != NULL)
		caseEquivPage->OnOK(event);
	if (kbPage != NULL)
		kbPage->OnOK(event);
	if (viewPage != NULL)
		viewPage->OnOK(event);
	if (autoSavePage != NULL)
		autoSavePage->OnOK(event);
	if (unitsPage != NULL)
		unitsPage->OnOK(event);
	if (usfmFilterPage != NULL)
		usfmFilterPage->OnOK(event);

	if (m_bDismissDialog)
		event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event);

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

// Wrapper handlers for the ToolbarPage
void CEditPreferencesDlg::OnRadioToolbarSmall(wxCommandEvent& event)
{
	toolbarPage->OnRadioToolbarSmall(event);
}

void CEditPreferencesDlg::OnRadioToolbarMedium (wxCommandEvent& event)
{
	toolbarPage->OnRadioToolbarMedium(event);
}

void CEditPreferencesDlg::OnRadioToolbarLarge (wxCommandEvent& event)
{
	toolbarPage->OnRadioToolbarLarge(event);
}

void CEditPreferencesDlg::OnCboToolbarIcon (wxCommandEvent& event)
{
	toolbarPage->OnCboToolbarIcon(event);
}

void CEditPreferencesDlg::OnBnToolbarMinimal (wxCommandEvent& event)
{
	toolbarPage->OnBnToolbarMinimal(event);
}

void CEditPreferencesDlg::OnBnToolbarReset (wxCommandEvent& event)
{
	toolbarPage->OnBnToolbarReset(event);
}

void CEditPreferencesDlg::OnClickLstToolbarButtons (wxListEvent& event)
{
	toolbarPage->OnClickLstToolbarButtons(event);
}



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
	usfmFilterPage->OnBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedRadioUseBothSetsDoc(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedRadioUseUbsSetOnlyProj(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CEditPreferencesDlg::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedRadioUseBothSetsProj(event);
}

void CEditPreferencesDlg::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfmFilterPage->OnBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}

// The following are wrapper handlers for the filterPage
void CEditPreferencesDlg::OnLbnSelchangeListSfmsDoc(wxCommandEvent& event)
{
	usfmFilterPage->OnLbnSelchangeListSfmsDoc(event);
}

// This handler is invoked when an item in the Doc check list box is checked or unchecked
void CEditPreferencesDlg::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	usfmFilterPage->OnCheckListBoxToggleDoc(event);
}

// This handler is invoked when an item in the Proj check list box is checked or unchecked
void CEditPreferencesDlg::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	usfmFilterPage->OnCheckListBoxToggleProj(event);
}

void CEditPreferencesDlg::OnLbnSelchangeListSfmsProj(wxCommandEvent& event)
{
	usfmFilterPage->OnLbnSelchangeListSfmsProj(event);
}

bool CEditPreferencesDlg::TabIsVisibleInCurrentProfile(wxString tabLabel)
{
	// Note: This function is similar to the App's MenuItemIsVisibleInThisProfile()
	// and they could be combined into a single function ItemIsVisibleInThisProfile().
	// We assume that a menu item is visible unless the m_pUserProfiles data
	// indicates otherwise.
	bool bItemIsVisible = TRUE;
	if (gpApp->m_nWorkflowProfile == 0)
	{
		// The work flow profile 0 (zero) is the "None" selection all all interface
		// items are visible by default
		return bItemIsVisible; 
	}
	int ct;
	int totct;
	totct = gpApp->m_pUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < totct; ct++)
	{
		UserProfileItem* pUserProfileItem;
		ProfileItemList::Node* node;
		node = gpApp->m_pUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = node->GetData();
		wxASSERT(pUserProfileItem != NULL);
		if (pUserProfileItem->itemType == _T("preferencesTab"))
		{
			int indexFromProfile = 0;
			if (gpApp->m_nWorkflowProfile <= 0)
				indexFromProfile = 0;
			else if (gpApp->m_nWorkflowProfile > 0)
				indexFromProfile = gpApp->m_nWorkflowProfile - 1;
			wxString itemText;
			itemText = pUserProfileItem->itemText;
			// Note: the text on tabs don't have decorations
			if (itemText == tabLabel && pUserProfileItem->usedVisibilityValues.Item(indexFromProfile) == _T("0"))
			{
				return FALSE;
			}
		}
	}
	return bItemIsVisible;
}
