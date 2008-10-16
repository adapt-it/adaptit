/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			USFMPage.cpp
/// \author			Bill Martin
/// \date_created	18 April 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CUSFMPageWiz, CUSFMPagePrefs and CUSFMPageCommon classes. 
/// The CUSFMPageWiz class creates a wizard page and the CUSFMPagePrefs class creates a
/// tab panel for the preferences notebook. A third class CUSFMPageCommon contains the
/// routines that are common to both of the other two classes. Together they allow the user
/// to change the sfm set used for the current document and/or whole project.
/// The interface resources for the page/panel are defined in USFMPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CUSFMPageWiz is derived from wxWizardPage, CUSFMPagePrefs from wxPanel, and CUSFMPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in USFMPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "USFMPage.h"
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

#include "Adapt_It.h"
#include "USFMPage.h"

#include "Adapt_ItView.h"
#include "CaseEquivPage.h"
#include "FilterPage.h"
#include "StartWorkingWizard.h"

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp.
extern CCaseEquivPageWiz* pCaseEquivPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CFilterPageWiz* pFilterPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CFilterPagePrefs* pFilterPageInPrefs; // set the App's pointer to the filterPage

// This global is defined in Adapt_It.cpp.
//extern CFilterPageWiz* pFilterPageInWizard; // set the App's pointer to the filterPage

//extern CUSFMPagePrefs* pUSFMPageInPrefs; // set the App's pointer to the filterPage

wxString msgWarningMadeChanges = _("Warning: You just made changes to the document's inventory of filter markers in the Filtering page\nwhere the %s was in effect. If you make this change the document will use\nthe Filtering page's settings for the %s. Do you want to make this change?");


void CUSFMPageCommon::DoSetDataAndPointers()
{
	// These initializations are done here in the constructor, rather than
	// in OnInitDialog, because tempSfmSetAfterEditDoc and tempSfmSetAfterEditProj can be
	// changed from the filterPage. We don't want OnInitDialog to reinitialize
	// them again, so the initializations are done from here.
	//
	// The calling routine can determine if sfm user set has changed for the Doc
	// by comparing tempSfmSetBeforeEditDoc with the sfm user set after edit
	tempSfmSetBeforeEditDoc = gpApp->gCurrentSfmSet;
	// start tempSfmSetAfterEditDoc with the value of the global set before edit
	tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc;

	// The calling routine can determine if sfm user set has changed for the Project
	// by comparing tempSfmSetBeforeEditProj with the Project (config) sfm set after edit
	tempSfmSetBeforeEditProj = gpApp->gProjectSfmSetForConfig;
	// start tempSfmSetAfterEditProj with the value of the global Project set before edit
	tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj;

	bSFMsetChanged = FALSE; // accessed by filterPage to see if sfm set was changed

	bDocSfmSetChanged = FALSE; // the Doc sfm set Undo button starts disabled
	bProjectSfmSetChanged = FALSE; // the Project sfm set Undo button starts disabled
	bOnInitDlgHasExecuted = FALSE;

	// Can't call InitDialog() from within CUSFMPageWiz's constructor because InitDialog below
	// references pFilterPgInWizard which does not get created until CFilterPage's constructor
	// executes. Same is true for the CFilterPage's constructor can't call its InitDialog within
	// its constructor. Otherwise we set up reciprocal dependencies that cannot be met.
	//wxInitDialogEvent event = wxEVT_INIT_DIALOG;
	//this->InitDialog(event);
}

void CUSFMPageCommon::DoInit()
{
	// get pointers to our controls
	pRadioUsfmOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_UBS_SET_ONLY);
	wxASSERT(pRadioUsfmOnly != NULL);
	pRadioPngOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_SILPNG_SET_ONLY);
	wxASSERT(pRadioPngOnly != NULL);
	pRadioUsfmAndPng = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_BOTH_SETS);
	wxASSERT(pRadioUsfmAndPng != NULL);

	pRadioUsfmOnlyProj = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_UBS_SET_ONLY_PROJ);
	wxASSERT(pRadioUsfmOnlyProj != NULL);
	pRadioPngOnlyProj = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ);
	wxASSERT(pRadioPngOnlyProj != NULL);
	pRadioUsfmAndPngProj = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_BOTH_SETS_PROJ);
	wxASSERT(pRadioUsfmAndPngProj != NULL);

	pRadioUsfmOnlyFactory = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_UBS_SET_ONLY_FACTORY);
	wxASSERT(pRadioUsfmOnlyFactory != NULL);
	pRadioPngOnlyFactory = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_SILPNG_SET_ONLY_FACTORY);
	wxASSERT(pRadioPngOnlyFactory != NULL);
	pRadioUsfmAndPngFactory = (wxRadioButton*)FindWindowById(IDC_RADIO_USE_BOTH_SETS_FACTORY);
	wxASSERT(pRadioUsfmAndPngFactory != NULL);

	pChangeFixedSpaceToRegular = (wxCheckBox*)FindWindowById(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM);
	wxASSERT(pChangeFixedSpaceToRegular != NULL);

	pTextCtrlStaticTextUSFMPage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_USFMPAGE);
	wxASSERT(pTextCtrlStaticTextUSFMPage != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlStaticTextUSFMPage->SetBackgroundColour(backgrndColor);
	pTextCtrlStaticTextUSFMPage->SetBackgroundColour(gpApp->sysColorBtnFace);

	// prepare some message text - normal and warning
	wxString tempStrDoc,tempStrProj;
	switch(tempSfmSetAfterEditDoc)
	{
	case UsfmOnly: tempStrDoc = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	case PngOnly: tempStrDoc = _("PNG 1998 Marker Set"); //IDS_PNG_SET_ABBR
	case UsfmAndPng: tempStrDoc = _("USFM 2.0 and PNG 1998 Marker Sets"); //IDS_BOTH_SETS_ABBR
	default: tempStrDoc = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	}
	switch(tempSfmSetAfterEditProj)
	{
	case UsfmOnly: tempStrProj = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	case PngOnly: tempStrProj = _("PNG 1998 Marker Set"); //IDS_PNG_SET_ABBR
	case UsfmAndPng: tempStrProj = _("USFM 2.0 and PNG 1998 Marker Sets"); //IDS_BOTH_SETS_ABBR
	default: tempStrProj = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	}

	wxString normalText1, normalText2, warningText1, warningText2;
	normalText1 = _("USFM Set changes should be made here BEFORE making changes on the Filtering page. Normally the sfm set for the document should be the same as the sfm set used for the project."); //.Format(IDS_USFM_INFO_1);
	normalText2 = _("After setting the appropriate sfm set here, you can set marker filtering on the Filtering page."); //.Format(IDS_USFM_INFO_2);
	// IDS_WARN_SFM_SET_IS
	warningText1 = warningText1.Format(_("Warning: The Document is using the %s. The Project, however, defaults to the %s."), tempStrDoc.c_str(), tempStrProj.c_str());
	warningText2 = _("Normally the sfm set for the document should be set to agree with the sfm set used for the project."); //.Format(IDS_NORMALLY_SFM_SETS_AGREE); // may use later for additional warning info

	// if there is a mismatch between sfm set used for current doc and the project
	// notify user, otherwise display message text
	if (tempSfmSetAfterEditDoc == tempSfmSetAfterEditProj)
	{
		// Show the normal USFM info messages
		pTextCtrlStaticTextUSFMPage->ChangeValue(normalText1 + _T(' ') + normalText2);
	}
	else
	{
		// Show the mismatch warning messages
		pTextCtrlStaticTextUSFMPage->ChangeValue(warningText1 + _T(' ') + warningText2);
	}

	pTextCtrlStaticTextUSFMPage->Refresh();

	// start with checkbox unchecked
	bChangeFixedSpaceToRegularSpace = gpApp->m_bChangeFixedSpaceToRegularSpace;
	bChangeFixedSpaceToRegularBeforeEdit = bChangeFixedSpaceToRegularSpace;

	if (bShowFixedSpaceCheckBox) // bShowFixedSpaceCheckBox should be set in caller before calling DoModal()
		pChangeFixedSpaceToRegular->Show(TRUE);
	else
		pChangeFixedSpaceToRegular->Show(FALSE);

	// These pointers to the filter page cannot be initialized in USFM page's constructor
	// because the USFM page gets constructed before the Filtering page, so we initialize
	// them here the first time USFM page tab is selected.
	//pFilterPgInPrefs = pFilterPageInPrefs;
	//pFilterPgInWizard = pFilterPageWiz;

	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences.
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the current sfm set.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.

		// Initialize the Doc arrays
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditDoc,
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsDoc);

		// reinventory and add any unknown markers to the Doc's filter arrays
		pFilterPageInPrefs->filterPgCommon.AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// Initialize the filter page's "before" flags and "before" marker string for the Doc
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->Item(ct));

		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditDoc;

		// Initialize the Proj arrays
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditProj,
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// Initialize the filter page's "before" flags and "before" marker string for the Proj
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->Item(ct));
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditProj;
		// No need to call SetupFilterPageArrays for factory - it never needs updating
	}
	else
	{
		// We're in the wizard after <New Project> has been selected. Since no document has yet been
		// created for this project, we force the sfm set and filtering inventory for the (potential) 
		// document to be the same as the sfm set and filtering inventory specified for the Project.
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj;
		
		// We also want to disable the USFM page's Document related buttons, because we do not allow
		// the user to change the document to use anything other than the project settings. This
		// disabled state for the document remains in effect for the life of the wizard.
		pRadioUsfmOnly->Enable(FALSE);
		pRadioPngOnly->Enable(FALSE);
		pRadioUsfmAndPng->Enable(FALSE);

		// See note above before SetupFilterPageArrays call.
		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc,
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsDoc);

		// No Document is loaded while we're in the USFM and/or Filtering pages (<New Project>)
		// part of the wizard so we cannot inventory any markers unknown to the doc
		//pFilterPgInWizard->AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// Initialize the filter page's "before" flags and "before" marker string for the Doc
		pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->Item(ct));
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc = pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc;

		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj,
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsProj,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// Initialize the filter page's "before" flags and "before" marker string for the Proj
		pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsProj->Item(ct));
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj = pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj;
		// No need to call SetupFilterPageArrays for factory - it never needs updating
	}

	// set the default state of the buttons
	UpdateButtons();

	bOnInitDlgHasExecuted = TRUE;

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("In USFM page's OnInitDialog AFTER SetupFilterPageArrays and UpdateButtons call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& WXUNUSED(event))
{

	// Check for this session changes in the filter page's Doc Filter inventory
	// If changes were made this session there before changing the sfm set here now,
	// warn the user that the change will activate the UsfmOnly filter settings
	// in effect there in the Filtering page.
	//
	// whm Note 16Jun05: When the user changes the sfm set applied to the Doc, this 
	// action also necessarily changes the list of filter markers as well as the list 
	// of potential unknown markers for the Doc. 
	//
	wxString msg, newSet;
	// IDS_WARNING_JUST_CHANGED_FILT_MKRS
	// Warning: You just made changes to the document's inventory of filter markers in the Filtering page
	// \nwhere the %1 was in effect. If you make this change the document will use
	// \nthe Filtering page's settings for the %2. Do you want to make this change?
	newSet = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		if (pFilterPageInPrefs->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageInPrefs->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
				return;
		}
		// update tempSfmSetAfterEditDoc with the new sfm set
		tempSfmSetAfterEditDoc = UsfmOnly;
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set. Since a new sfm set also means the filter page's 
		// tempFilterMarkersAfterEditDoc has markers from the previous sfm set, we
		// need to use the UsfmFilterMarkersStr on the App for building our marker arrays.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		// Initialize the Doc arrays
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsDoc);

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM Page User clicked UsfmOnly - Now calling Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		pFilterPageInPrefs->filterPgCommon.AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// 
		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		int ct;
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		if (pFilterPageWiz->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			// IDS_WARNING_JUST_CHANGED_FILT_MKRS
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageWiz->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL); //MB_OKCANCEL
			if (nResult == wxCANCEL) // IDCANCEL
				return;
		}
		// update the tempSfmSetAfterEditDoc with the new sfm set
		tempSfmSetAfterEditDoc = UsfmOnly;
		// See note above before SetupFilterPageArrays call.
		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsDoc);

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		//pFilterPageWiz->pFilterFlagsDocBeforeEdit->Copy(*pFilterPgInWizard->pFilterFlagsDoc);

		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc;
	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page DOC Sfm Set changed to UsfmOnly\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& WXUNUSED(event))
{
	// Check for this session changes in the filter page's Doc Filter inventory
	// If changes were made this session there before changing the sfm set here now,
	// warn the user that the change will activate the PngOnly filter settings
	// in effect there in the Filtering page.
	wxString msg, newSet;
	// IDS_WARNING_JUST_CHANGED_FILT_MKRS
	// Warning: You just made changes to the document's inventory of filter markers in the Filtering page
	// \nwhere the %1 was in effect. If you make this change the document will use
	// \nthe Filtering page's settings for the %2. Do you want to make this change?
	newSet = _("PNG 1998 Marker Set"); //.Format(IDS_PNG_SET_ABBR);
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		if (pFilterPageInPrefs->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			// IDS_WARNING_JUST_CHANGED_FILT_MKRS
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageInPrefs->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
				return;
		}
		tempSfmSetAfterEditDoc = PngOnly;
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsDoc);

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM Page User clicked PngOnly - Now calling Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		pFilterPageInPrefs->filterPgCommon.AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		//pFilterPageInPrefs->pFilterFlagsDocBeforeEdit->Copy(*pFilterPgInPrefs->pFilterFlagsDoc);
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		if (pFilterPageWiz->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			// IDS_WARNING_JUST_CHANGED_FILT_MKRS
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageWiz->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL); //MB_OKCANCEL
			if (nResult == wxCANCEL)
				return;
		}
		tempSfmSetAfterEditDoc = PngOnly;
		// See note above.
		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsDoc);

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc;
	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page DOC Sfm Set changed to PngOnly\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseBothSetsDoc(wxCommandEvent& WXUNUSED(event))
{
	// Check for this session changes in the filter page's Doc Filter inventory
	// If changes were made this session there before changing the sfm set here now,
	// warn the user that the change will activate the UsfmAndPng filter settings
	// in effect there in the Filtering page.
	wxString msg, newSet;
	// IDS_WARNING_JUST_CHANGED_FILT_MKRS
	// Warning: You just made changes to the document's inventory of filter markers in the Filtering page
	// \nwhere the %1 was in effect. If you make this change the document will use
	// \nthe Filtering page's settings for the %2. Do you want to make this change?
	newSet = _("USFM 2.0 and PNG 1998 Marker Sets"); //.Format(IDS_BOTH_SETS_ABBR);
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		if (pFilterPageInPrefs->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			// IDS_WARNING_JUST_CHANGED_FILT_MKRS
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageInPrefs->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
				return;
		}
		tempSfmSetAfterEditDoc = UsfmAndPng;
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsDoc);

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM Page User clicked UsfmAndPng - Now calling Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		pFilterPageInPrefs->filterPgCommon.AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		if (pFilterPageWiz->filterPgCommon.bDocFilterMarkersChanged)
		{
			// Show the mismatch warning messages
			// IDS_WARNING_JUST_CHANGED_FILT_MKRS
			msg = msg.Format(msgWarningMadeChanges.c_str(), 
				pFilterPageWiz->filterPgCommon.GetSetNameStr(tempSfmSetAfterEditDoc).c_str(),
				newSet.c_str());
			int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL); //MB_OKCANCEL
			if (nResult == wxCANCEL) // IDCANCEL
				return;
		}
		tempSfmSetAfterEditDoc = UsfmAndPng;
		// See note above.
		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsDoc);

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc, 2);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsDocBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsDoc,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditDoc = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditDoc;
	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page DOC Sfm Set changed to UsfmAndPng\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	tempSfmSetAfterEditProj = UsfmOnly;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences.
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (UsfmOnly).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.

		// In Edit Preferences a document is currently open. When the user changes the
		// project setting to UsfmOnly we assume the user wishes to also cause the loaded
		// document to adopt the UsfmOnly sfm set too. Since the Document's radio buttons
		// are enabled when called via Edit Preferences (unlike the wizard where they are
		// disabled), the user can force the document to be different by directly clicking
		// on a different sfm set radio button, but we'll start the user off with a doc
		// setting that agrees with the project selection. 
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		// Set up the filter arrays based on the filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays.
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj;

		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadProjSFMListBox(LoadInitialDefaults);
		
		// A change in the Project's sfm set to UsfmOnly also sets the Doc's sfm set to UsfmOnly.
		// We can simulate a selection of UsfmOnly for the Doc by calling OnBnClickedRadioUseUbsSetOnlyDoc.
		DoBnClickedRadioUseUbsSetOnlyDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting. It is
		// "forced" because the Doc's buttons are disabled and are thus not user changeable.
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsProj,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to UsfmOnly also sets the Doc's sfm set to UsfmOnly.
		// We can simulate a selection of UsfmOnly for the Doc by calling OnBnClickedRadioUseUbsSetOnlyDoc.
		DoBnClickedRadioUseUbsSetOnlyDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page PROJ Sfm Set changed to UsfmOnly\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	tempSfmSetAfterEditProj = PngOnly;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences.
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (PngOnly).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.

		// In Edit Preferences a document is currently open. When the user changes the
		// project setting to PngOnly we assume the user wishes to also cause the loaded
		// document to adopt the PngOnly sfm set too. Since the Document's radio buttons
		// are enabled when called via Edit Preferences (unlike the wizard where they are
		// disabled), the user can force the document to be different by directly clicking
		// on a different sfm set radio button, but we'll start the user off with a doc
		// setting that agrees with the project selection. 
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		// Set up the filter arrays based on the filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays.
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadProjSFMListBox(LoadInitialDefaults);

		// A change in the Project's sfm set to PngOnly also sets the Doc's sfm set to PngOnly.
		// We can simulate a selection of PngOnly for the Doc by calling OnBnClickedRadioUseSilpngSetOnlyDoc.
		DoBnClickedRadioUseSilpngSetOnlyDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsProj,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to PngOnly also sets the Doc's sfm set to PngOnly.
		// We can simulate a selection of PngOnly for the Doc by calling OnBnClickedRadioUseSilpngSetOnlyDoc.
		DoBnClickedRadioUseSilpngSetOnlyDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page PROJ Sfm Set changed to PngOnly\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	tempSfmSetAfterEditProj = UsfmAndPng;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences.
		// Call the filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (UsfmAndPng).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.

		// In Edit Preferences a document is currently open. When the user changes the
		// project setting to UsfmAndPng we assume the user wishes to also cause the loaded
		// document to adopt the UsfmAndPng sfm set too. Since the Document's radio buttons
		// are enabled when called via Edit Preferences (unlike the wizard where they are
		// disabled), the user can force the document to be different by directly clicking
		// on a different sfm set radio button, but we'll start the user off with a doc
		// setting that agrees with the project selection. 
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		// Set up the filter arrays based on the filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		pFilterPageInPrefs->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj,
			&pFilterPageInPrefs->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageInPrefs->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageInPrefs->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageInPrefs->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageInPrefs->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageInPrefs->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageInPrefs->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageInPrefs->filterPgCommon.tempFilterMarkersBeforeEditProj;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		pFilterPageInPrefs->filterPgCommon.LoadProjSFMListBox(LoadInitialDefaults);

		// A change in the Project's sfm set to UsfmAndPng also sets the Doc's sfm set to UsfmAndPng.
		// We can simulate a selection of UsfmAndPng for the Doc by calling OnBnClickedRadioUseBothSetsDoc.
		DoBnClickedRadioUseBothSetsDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		pFilterPageWiz->filterPgCommon.SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
			&pFilterPageWiz->filterPgCommon.m_filterFlagsProj,
			&pFilterPageWiz->filterPgCommon.m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj, 2);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterPageWiz->filterPgCommon.pFilterFlagsProj->GetCount(); ct++)
			pFilterPageWiz->filterPgCommon.pFilterFlagsProjBeforeEdit->Add(pFilterPageWiz->filterPgCommon.pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj 
			= pFilterPageWiz->filterPgCommon.GetFilterMkrStrFromFilterArrays(&pFilterPageWiz->filterPgCommon.m_SfmMarkerAndDescriptionsProj,
																&pFilterPageWiz->filterPgCommon.m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		pFilterPageWiz->filterPgCommon.tempFilterMarkersAfterEditProj = pFilterPageWiz->filterPgCommon.tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to UsfmAndPng also sets the Doc's sfm set to UsfmAndPng.
		// We can simulate a selection of UsfmAndPng for the Doc by calling OnBnClickedRadioUseBothSetsDoc.
		DoBnClickedRadioUseBothSetsDoc(event); // this will set up the Doc's arrays in Filter Page.
	}
	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM page PROJ Sfm Set changed to UsfmAndPng\n");
#endif
}

void CUSFMPageCommon::DoBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& WXUNUSED(event))
{
	// The factory defaults cannot be changed, inform the user here
	// since this is a non-default selection
	wxMessageBox(_T("Sorry, the factory defaults cannot be changed."),_T(""),wxICON_INFORMATION);
	UpdateButtons();
}

void CUSFMPageCommon::DoBnClickedRadioUseBothSetsFactory(wxCommandEvent& WXUNUSED(event))
{
	// The factory defaults cannot be changed, inform the user here
	// since this is a non-default selection
	wxMessageBox(_T("Sorry, the factory defaults cannot be changed."),_T(""),wxICON_INFORMATION);
	UpdateButtons();
}

void CUSFMPageCommon::DoBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event))
{
	// Note: The value of bChangeFixedSpaceToRegularSpace is a local Boolean 
	// temporarily holding the users selection until he dismisses the edit
	// preferences dialog with the OK button. Edit Preferences then updates
	// the permanent member m_bChangeFixedSpaceToRegularSpace on the App 
	// and its integral equivalent stored in project config file.
	if (bChangeFixedSpaceToRegularSpace)
	{
		bChangeFixedSpaceToRegularSpace = FALSE;
	}
	else
	{
		bChangeFixedSpaceToRegularSpace = TRUE;
	}
	UpdateButtons();
}

void CUSFMPageCommon::UpdateButtons()
{
	// the doc sfm set radio buttons
	switch (tempSfmSetAfterEditDoc)
	{
	case UsfmOnly: 
		pRadioUsfmOnly->SetValue(TRUE);
		pRadioPngOnly->SetValue(FALSE);
		pRadioUsfmAndPng->SetValue(FALSE); break;
	case PngOnly:
		pRadioUsfmOnly->SetValue(FALSE);
		pRadioPngOnly->SetValue(TRUE);
		pRadioUsfmAndPng->SetValue(FALSE); break;
	case UsfmAndPng:
		pRadioUsfmOnly->SetValue(FALSE);
		pRadioPngOnly->SetValue(FALSE);
		pRadioUsfmAndPng->SetValue(TRUE); break;
	default:
		pRadioUsfmOnly->SetValue(TRUE);
		pRadioPngOnly->SetValue(FALSE);
		pRadioUsfmAndPng->SetValue(FALSE); break;
	}
	// the project sfm set radio buttons
	switch (tempSfmSetAfterEditProj)
	{
	case UsfmOnly: 
		pRadioUsfmOnlyProj->SetValue(TRUE);
		pRadioPngOnlyProj->SetValue(FALSE);
		pRadioUsfmAndPngProj->SetValue(FALSE); break;
	case PngOnly:
		pRadioUsfmOnlyProj->SetValue(FALSE);
		pRadioPngOnlyProj->SetValue(TRUE);
		pRadioUsfmAndPngProj->SetValue(FALSE); break;
	case UsfmAndPng:
		pRadioUsfmOnlyProj->SetValue(FALSE);
		pRadioPngOnlyProj->SetValue(FALSE);
		pRadioUsfmAndPngProj->SetValue(TRUE); break;
	default:
		pRadioUsfmOnlyProj->SetValue(TRUE);
		pRadioPngOnlyProj->SetValue(FALSE);
		pRadioUsfmAndPngProj->SetValue(FALSE); break;
	}
	// the factory sfm set radio buttons
	pRadioUsfmOnlyFactory->SetValue(TRUE); // factory default is always UsfmOnly
	pRadioPngOnlyFactory->SetValue(FALSE);
	pRadioUsfmAndPngFactory->SetValue(FALSE);

	// update the change-fixed-space-to-regular-space check box
	pChangeFixedSpaceToRegular->SetValue(bChangeFixedSpaceToRegularSpace);
}

IMPLEMENT_DYNAMIC_CLASS( CUSFMPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CUSFMPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CUSFMPageWiz::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_WIZARD_PAGE_CHANGING(-1, CUSFMPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CUSFMPageWiz::OnWizardCancel)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY, CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY, CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS, CUSFMPageWiz::OnBnClickedRadioUseBothSetsDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_PROJ, CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ, CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_PROJ, CUSFMPageWiz::OnBnClickedRadioUseBothSetsProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_FACTORY, CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_FACTORY, CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_FACTORY, CUSFMPageWiz::OnBnClickedRadioUseBothSetsFactory)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM, CUSFMPageWiz::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
END_EVENT_TABLE()


CUSFMPageWiz::CUSFMPageWiz()
{
}

CUSFMPageWiz::CUSFMPageWiz(wxWizard* parent) // dialog constructor
{
	Create( parent );

	usfmPgCommon.DoSetDataAndPointers();
}

CUSFMPageWiz::~CUSFMPageWiz() // destructor
{
	
}

bool CUSFMPageWiz::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CUSFMPageWiz::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	usfmPgCommon.pUSFMPageSizer = USFMPageFunc(this, TRUE, TRUE);
	usfmPgCommon.pUSFMPageSizer->Layout();
}

// implement wxWizardPage functions
wxWizardPage* CUSFMPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pCaseEquivPageWiz; 
}
wxWizardPage* CUSFMPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pFilterPageWiz;
}

void CUSFMPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}

void CUSFMPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	usfmPgCommon.DoInit();
}

void CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsDoc(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseUbsSetOnlyProj(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsProj(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& WXUNUSED(event))
{
	// Do nothing - the factory defaults cannot be changed
}

void CUSFMPageWiz::OnBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyFactory(event);
}

void CUSFMPageWiz::OnBnClickedRadioUseBothSetsFactory(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsFactory(event);
}

void CUSFMPageWiz::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}

void CUSFMPageWiz::OnWizardPageChanging(wxWizardEvent& event) 
{
	// Do any required changes to sfm and filter settings.
	// Note: We should not need to call RetokenizeText() from here because the 
	// filterPage is only accessible within the wizard after the user clicks on 
	// <New Project> from the projectPage, and no document will be open, since 
	// the docPage is the last page in the wizard, appearing after this page.
	// Note: Should the call to gpApp->DoUsfmSetChanges() be done in the DocPage's
	// OnWizardFinish routine rather than here???

#ifdef _Trace_FilterMarkers
	TRACE0("In USFM page's OnWizardNext BEFORE DoUsfmSetChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	usfmPgCommon.bSFMsetChanged = FALSE; // supplied for param list, used by filterPage to see if sfm set was changed
	// Note: DoUsfmSetChanges below updates the global sfm set and the global filter marker strings
	// for any changes the user has made in document and/or project settings
	gpApp->DoUsfmSetChanges(&usfmPgCommon, &pFilterPageWiz->filterPgCommon, usfmPgCommon.bSFMsetChanged, NoReparse); // NoReparse within wizard since no doc is open there
	
#ifdef _Trace_FilterMarkers
	TRACE0("In USFM page's OnWizardNext AFTER DoUsfmSetChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif
	
	bool bMovingForward = event.GetDirection();

	if (bMovingForward)
	{
		// insure the filterPage's InitDialog is called to load the list boxes properly, 
		// in case we just made changes to sfm set
		wxInitDialogEvent ievent = wxEVT_INIT_DIALOG;
		pFilterPageWiz->InitDialog(ievent);

	}
}

IMPLEMENT_DYNAMIC_CLASS( CUSFMPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CUSFMPagePrefs, wxPanel)
	EVT_INIT_DIALOG(CUSFMPagePrefs::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY, CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY, CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS, CUSFMPagePrefs::OnBnClickedRadioUseBothSetsDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_PROJ, CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ, CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_PROJ, CUSFMPagePrefs::OnBnClickedRadioUseBothSetsProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_FACTORY, CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_FACTORY, CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyFactory)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_FACTORY, CUSFMPagePrefs::OnBnClickedRadioUseBothSetsFactory)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM, CUSFMPagePrefs::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
END_EVENT_TABLE()


CUSFMPagePrefs::CUSFMPagePrefs()
{
}

CUSFMPagePrefs::CUSFMPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );

	usfmPgCommon.DoSetDataAndPointers();
}

CUSFMPagePrefs::~CUSFMPagePrefs() // destructor
{
	
}

bool CUSFMPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CUSFMPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	usfmPgCommon.pUSFMPageSizer = USFMPageFunc(this, TRUE, TRUE);
	usfmPgCommon.pUSFMPageSizer->Layout();
}

void CUSFMPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	usfmPgCommon.DoInit();
}

void CUSFMPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// wx version notes: Any changes made to the logic in OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CUSFMPagePrefs::OnWizardPageChanging() 
	// is called when page changes or Finish button is pressed. 
	// CUSFMPagePrefs::OnOK() is always called when the user dismisses the EditPreferencesDlg
	// dialog with the OK button.

	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling of this CFilterPage::OnOK() from there.

	// Do any required changes to sfm and filter settings.
	// Note: We should not need to call RetokenizeText() from here because the 
	// filterPage is only accessible within the wizard after the user clicks on 
	// <New Project> from the projectPage, and no document will be open, since 
	// the docPage is the last page in the wizard, appearing after this page.
	// Note: Should the call to gpApp->DoUsfmSetChanges() be done in the DocPage's
	// OnWizardFinish routine rather than here???

#ifdef _Trace_FilterMarkers
	TRACE0("In USFM page's OnOK BEFORE DoUsfmSetChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	usfmPgCommon.bSFMsetChanged = FALSE; // supplied for param list, used by filterPage to see if sfm set was changed
	// Note: DoUsfmSetChanges below updates the global sfm set and the global filter marker strings
	// for any changes the user has made in document and/or project settings. It also will set the 
	// usfmPgCommon's bSFMsetChanged flag to true if there was a set change which will insure that
	// the filterPage's OnOK handler calls DoUsfmFilterChanges() with NoReparse parameter.
	gpApp->DoUsfmSetChanges(&usfmPgCommon, &pFilterPageInPrefs->filterPgCommon, usfmPgCommon.bSFMsetChanged, DoReparse); // DoReparse for edit prefs
	
#ifdef _Trace_FilterMarkers
	TRACE0("In USFM page's OnOK AFTER DoUsfmSetChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif
}

void CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsDoc(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseUbsSetOnlyProj(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsProj(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& WXUNUSED(event))
{
	// Do nothing - the factory defaults cannot be changed
}

void CUSFMPagePrefs::OnBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseSilpngSetOnlyFactory(event);
}

void CUSFMPagePrefs::OnBnClickedRadioUseBothSetsFactory(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedRadioUseBothSetsFactory(event);
}

void CUSFMPagePrefs::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfmPgCommon.DoBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}
