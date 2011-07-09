/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UsfmFilterPage.h
/// \author			Bill Martin
/// \date_created	6 October 2010
/// \date_revised	6 October 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CUsfmFilterPageWiz and 
/// CUsfmFilterPagePrefs classes. It also defines a third class CUsfmFilterPageCommon which
/// contains the routines common to the above two classes.
/// The CUsfmFilterPageWiz class creates a wizard page for the StartWorkingWizard; the 
/// CUsfmFilterPagePrefs creates a identical panel as a tabbed page in the 
/// EditPreferencesDlg. Each form allows the user to change the USFM set and/or the
/// filtering state of standard format markers used in the current document and/or the
/// whole project. The interface resources for the page/panel are defined in 
/// UsfmFilterPageFunc() which was developed in and is maintained by wxDesigner.
/// \derivation		CUsfmFilterPageWiz is derived from wxWizardPage, 
/// CUsfmFilterPagePrefs from wxPanel, and CUsfmFilterPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "UsfmFilterPage.h"
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

// Note: The following is necessary because wxCheckListBox is a WX owner draw class
// and as such needs to include ownerdrw.h on the Windows platform
#ifdef __WXMSW__
    #include  "wx/ownerdrw.h"
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/wizard.h>
#include  "wx/checklst.h"

#include "Adapt_It.h"
#include "Pile.h"
#include "Layout.h"
#include "UsfmFilterPage.h"

#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "CaseEquivPage.h"
#include "StartWorkingWizard.h"
#include "MainFrm.h"
#include "DocPage.h"
#include "helpers.h"

#if !wxUSE_CHECKLISTBOX
    #error "This program can't be built without wxUSE_CHECKLISTBOX set to 1"
#endif // wxUSE_CHECKLISTBOX

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern CCaseEquivPageWiz* pCaseEquivPageWiz; // need this ???

/// This global is defined in Adapt_It.cpp.
extern CUsfmFilterPageWiz* pUsfmFilterPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CUsfmFilterPagePrefs* pUsfmFilterPageInPrefs; // set the App's pointer to the filterPage // need this ???

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage; // need this ???

// need this ??? No (BEW 5Jan11)
//wxString msgWarningMadeChanges = _("Warning: You just made changes to the document's inventory of filter markers in the Filtering page\nwhere the %s was in effect. If you make this change the document will use\nthe Filtering page's settings for the %s. Do you want to make this change?");
wxString msgCannotFilterAndChangeSFMset = _("Trying to change the standard format marker set at the same time as trying to change the marker filtering settings is illegal.\nFinish one type of change then return to the same page to do the other.");

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar;

// CUsfmFilterPageCommon Class /////////////////////////////////////////////////////
void CUsfmFilterPageCommon::DoSetDataAndPointers()
{
	bDocFilterMarkersChanged = FALSE; // for the Doc filter list Undo button enabling
	bProjectFilterMarkersChanged = FALSE; // for the Project filter list Undo button enabling

	// initialize pointers to arrays
	pSfmMarkerAndDescriptionsDoc = &m_SfmMarkerAndDescriptionsDoc;
	pFilterFlagsDoc = &m_filterFlagsDoc;
	pFilterFlagsDocBeforeEdit = &m_filterFlagsDocBeforeEdit; // to detect any changes to filter markers of the chosen set
	pUserCanSetFilterFlagsDoc = &m_userCanSetFilterFlagsDoc;
	
	pSfmMarkerAndDescriptionsProj = &m_SfmMarkerAndDescriptionsProj;
	pFilterFlagsProj = &m_filterFlagsProj;
	pFilterFlagsProjBeforeEdit = &m_filterFlagsProjBeforeEdit; // to detect any changes to filter markers of the chosen set
	pUserCanSetFilterFlagsProj = &m_userCanSetFilterFlagsProj;

	//pSfmMarkerAndDescriptionsFactory = &m_SfmMarkerAndDescriptionsFactory;
	//pFilterFlagsFactory = &m_filterFlagsFactory;
	//pUserCanSetFilterFlagsFactory = &m_userCanSetFilterFlagsFactory;

	bOnInitDlgHasExecuted = FALSE;

#ifdef _Trace_UnknownMarkers
	TRACE0("In Usfm Filter Page's constructor copying Doc's unk mkr arrays to local filter page's arrays:\n");
	TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif

	// fill the local unknown marker variables with copies of the same named variables on the App
	m_unknownMarkers.Clear();
	int uct;
	for (uct = 0; uct < (int)gpApp->m_unknownMarkers.GetCount(); uct++)
	{
		// copy all items from m_unknownMarkers into saveUnknownMarkers
		m_unknownMarkers.Add(gpApp->m_unknownMarkers.Item(uct));
	}
	m_filterFlagsUnkMkrs.Clear();
	for (uct = 0; uct < (int)gpApp->m_filterFlagsUnkMkrs.GetCount(); uct++)
	{
		// copy all items from m_unknownMarkers into saveUnknownMarkers
		m_filterFlagsUnkMkrs.Add(gpApp->m_filterFlagsUnkMkrs.Item(uct));
	}
	m_currentUnknownMarkersStr = gpApp->m_currentUnknownMarkersStr;

	// fill the Doc's local filter markers strings (before and after edit) from those on the App
	tempFilterMarkersBeforeEditDoc = gpApp->gCurrentFilterMarkers;
	tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;

	// fill the Project's local filter markers strings (before and after edit) from those on the App
	tempFilterMarkersBeforeEditProj = gpApp->gProjectFilterMarkersForConfig;
	tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;


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
	tempSfmSetAfterEditProj = gpApp->gProjectSfmSetForConfig;

	bSFMsetChanged = FALSE; // accessed by filterPage to see if sfm set was changed

	bDocSfmSetChanged = FALSE; // the Doc sfm set Undo button starts disabled
	bProjectSfmSetChanged = FALSE; // the Project sfm set Undo button starts disabled
	bOnInitDlgHasExecuted = FALSE;
	bWarningShownAlready = FALSE; // flag for duplicated warning suppression starts OFF
}

void CUsfmFilterPageCommon::DoInit()
{
	// get pointers to our controls
	pListBoxSFMsDoc = (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS);
	wxASSERT(pListBoxSFMsDoc != NULL);
	
	pListBoxSFMsProj =  (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS_PROJ);
	wxASSERT(pListBoxSFMsProj != NULL);
	
	pTextCtrlAsStaticFilterPage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_FILTERPAGE);
	wxASSERT(pTextCtrlAsStaticFilterPage != NULL);
	//wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticFilterPage->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticFilterPage->SetBackgroundColour(gpApp->sysColorBtnFace);

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

	pChangeFixedSpaceToRegular = (wxCheckBox*)FindWindowById(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM);
	wxASSERT(pChangeFixedSpaceToRegular != NULL);

	wxString normalText1, normalText2;
	normalText1 = _("To filter out (hide) the text associated with a marker, check the box next to the marker. To adapt the text associated with a marker, un-check the box next to the marker."); //.Format();
	normalText2 = _("When checked, the marker and associated text will be filtered out (hidden) from the adapted document."); //.Format();

	if (pStartWorkingWizard == NULL)
	{
		// We are in Edit Preferences with an existing document open
		if (tempSfmSetAfterEditProj == tempSfmSetAfterEditDoc)
		{
			// Show the normal USFM info messages
			pTextCtrlAsStaticFilterPage->ChangeValue(normalText1 + _T(' ') + normalText2);
		}
		else
		{
			// Show the mismatch warning messages
			wxString warningText1, warningText2;
			// IDS_WARN_SFM_SET_IS
			warningText1 = warningText1.Format(_("Warning: The Document is using the %s. The Project, however, defaults to the %s."), 
				GetSetNameStr(tempSfmSetAfterEditDoc).c_str(), 
				GetSetNameStr(tempSfmSetAfterEditProj).c_str());
			// IDS_NORMALLY_SFM_SETS_AGREE
			warningText2 = _("Normally the sfm set for the document should be set to agree with the sfm set used for the project."); //.Format(); // may use later for additional warning info
			pTextCtrlAsStaticFilterPage->ChangeValue(warningText1 + _T(' ') + warningText2);
		}
	}
	else
	{
		// We are in the Wizard after <New Project> has been selected
		
		// There will never be a mismatch between Doc and Proj settings from within the wizard
		pTextCtrlAsStaticFilterPage->ChangeValue(normalText1 + _T(' ') + normalText2);

		// Note: A New Project has been created and at the end of the present wizard a new document
		// will normally be created. We want any new document to take the filtering state
		// defined by the PROJECT settings, so we will here cause the document's filtering settings 
		// to assume the values of the current project's settings, and in addition, we always disable 
		// the document's filtering settings here in the wizard.

		// We're in the wizard after <New Project> has been selected. Since no document has yet been
		// created for this project, we force the sfm set and filtering inventory for the (potential) 
		// document to be the same as the sfm set and filtering inventory specified for the Project.
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;
		tempFilterMarkersAfterEditDoc = tempFilterMarkersAfterEditProj;

		// We also want to disable the Filtering page's Document related list and buttons, 
		// because we do not allow the user to change the document to use anything other than the project 
		// settings. This disabled state for the document remains in effect for the life of the wizard.
		pListBoxSFMsDoc->Enable(FALSE);
		// We also want to disable the page's Document related buttons, because we do not allow
		// the user to change the document to use anything other than the project settings. This
		// disabled state for the document remains in effect for the life of the wizard.
		pRadioUsfmOnly->Enable(FALSE);
		pRadioPngOnly->Enable(FALSE);
		pRadioUsfmAndPng->Enable(FALSE);
	}

	// start with checkbox unchecked
	bChangeFixedSpaceToRegularSpace = gpApp->m_bChangeFixedSpaceToRegularSpace;
	bChangeFixedSpaceToRegularBeforeEdit = bChangeFixedSpaceToRegularSpace;

	if (bShowFixedSpaceCheckBox) // bShowFixedSpaceCheckBox should be set in caller before calling DoModal()
		pChangeFixedSpaceToRegular->Show(TRUE);
	else
		pChangeFixedSpaceToRegular->Show(FALSE);


	LoadProjSFMListBox(LoadInitialDefaults);
	// whm 5Nov10 modified below to disable doc related controls when no doc is loaded
	if (pStartWorkingWizard == NULL && gpApp->m_pSourcePhrases->GetCount() > 0)
	{
		// We're in the Preferences dialog and a document is open
		LoadDocSFMListBox(LoadInitialDefaults);
		// initially have focus on the Doc list box
		pListBoxSFMsDoc->SetFocus();
	}
	else
	{
		// When in the wizard or in Preferences with NO doc open, load only a 
		// message into the pListBoxSFMsDoc saying that "(NO DOCUMENTS CREATED YET IN THIS PROJECT)"
		// and disable the doc controls
		pListBoxSFMsDoc->Clear();
		pListBoxSFMsDoc->Append(_("(NO DOCUMENT AVAILABLE IN THIS PROJECT - NO FILTER SETTINGS TO DISPLAY)"));
		pListBoxSFMsDoc->Enable(FALSE);
		pRadioUsfmOnly->Enable(FALSE);
		pRadioPngOnly->Enable(FALSE);
		pRadioUsfmAndPng->Enable(FALSE);
		// initially have focus on the Proj list box
		pListBoxSFMsProj->SetFocus();
	}

	// set the default state of the buttons
	UpdateButtons();

	bOnInitDlgHasExecuted = TRUE;
	bFirstWarningGiven = FALSE;

#ifdef _Trace_FilterMarkers
	TRACE0("In Usfm Filter page's OnInitDialog AFTER all List Boxes Loaded:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	pUsfmFilterPageSizer->Layout(); // only needed here to insure the layout of the help text in read-only edit is ok
}

void CUsfmFilterPageCommon::AddUnknownMarkersToDocArrays()
{
	// Add to the arrays any unknown markers listed for the Doc
	// NOTE 16Jun05: The USFM set may change and call AddUnknownMarkersToDocArrays. 
	// This requires we get from the document a new inventory of 
	// m_unknownMarkers and related arrays before we add them to the Doc's list box.

	wxString lbStr;
	// Get the currently selected sfm set
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	enum SfmSet useSfmSet;
	if (pStartWorkingWizard == NULL)
	{
		useSfmSet = tempSfmSetAfterEditDoc;
	}
	else
	{
		useSfmSet = tempSfmSetAfterEditDoc;
	}

#ifdef _Trace_UnknownMarkers
	TRACE0("In Usfm Filter Page's AddUnknownMarkersToDocArrays BEFORE GetUnknownMarkersFromDoc call:\n");
	TRACE1("         Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1(" Doc's m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
	TRACE1("         Filter Pg's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&m_unknownMarkers, &m_filterFlagsUnkMkrs));
	TRACE1(" Filter Pg's m_currentUnknownMarkersStr = %s\n", m_currentUnknownMarkersStr);
#endif

	// reinventory the unknown markers from the Doc filling the local copies of the
	// unknown marker arrays: m_unknownMarkers, m_filterFlagsUnkMkrs, and 
	// m_currentUnknownMarkersStr
	pDoc->GetUnknownMarkersFromDoc(useSfmSet,&m_unknownMarkers,&m_filterFlagsUnkMkrs,
							m_currentUnknownMarkersStr,useCurrentUnkMkrFilterStatus);

#ifdef _Trace_UnknownMarkers
	TRACE0("In Usfm Filter Page's AddUnknownMarkersToDocArrays AFTER GetUnknownMarkersFromDoc call:\n");
	TRACE1("         Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1(" Doc's m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
	TRACE1("         Filter Pg's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&m_unknownMarkers, &m_filterFlagsUnkMkrs));
	TRACE1(" Filter Pg's m_currentUnknownMarkersStr = %s\n", m_currentUnknownMarkersStr);
#endif

	int unkCount = m_unknownMarkers.GetCount();
	int unkIndex, foundIndex;
	wxString unkStr, wholeUnkMarker;
	unkStr = _("[UNKNOWN MARKER]"); //.Format(IDS_UNKNOWN_MARKER);
	for (unkIndex = 0; unkIndex < unkCount; unkIndex++)
	{
		wholeUnkMarker = m_unknownMarkers.Item(unkIndex);

		// Check if wholeUnkMarker already exists in pSfmMarkerAndDescriptionsDoc. If not,
		// then add it. If is already exists don't add it, just continue processing.
		if (pDoc->MarkerExistsInArrayString(pSfmMarkerAndDescriptionsDoc,wholeUnkMarker,foundIndex))
		{
			// it already exists in the marker and descriptions wxArrayString, so continue
			continue;
		}

		lbStr = wholeUnkMarker;
		lbStr += _T(" "); // separate the whole marker and its description with a space
		lbStr = _T("  ") + lbStr;	// prefix with two spaces to force unk markers to sort at beginning of 
									// list box
		lbStr += unkStr;
		// add the unknown marker-description string to the Doc's marker and desc array
		pSfmMarkerAndDescriptionsDoc->Add(lbStr);
		// add the unknown marker filter flag, using m_filterFlagsUnkMkrs, to the Doc's filter flags array
		pFilterFlagsDoc->Add(m_filterFlagsUnkMkrs.Item(unkIndex) == TRUE);
		// add a TRUE userCanSetFilter flag to the Doc's userCanSetFilter flag array for the unknown marker
		pUserCanSetFilterFlagsDoc->Add(TRUE);
	}
}

void CUsfmFilterPageCommon::SetupFilterPageArrays(MapSfmToUSFMAnalysisStruct* pMap,
		wxString filterMkrStr,
		enum UseStringOrMap useStrOrMap,
		wxArrayString* pSfmMarkerAndDescr,
		wxArrayInt* pFilterFlags,
		wxArrayInt* pUserCanSetFlags)
{
	// SetupFilterPageArrays is called once for each of the sets of marker arrays: Doc and Proj.
	// It initializes and populates the pSfmMarkerAndDescr, pFilterFlags and pUserCanSetFlags arrays with 
	// the data that LoadListBoxFromArrays() subsequently uses to fill the usfm filter page's list boxes (doc
	// and proj). The arrays contain information on all the markers for the active temporary sfm set. 
	//
	// whm 25May07 modified the wx version removing the "Show All" button. We still set up the usfm filter page
	// arrays the way they were done previously so that they contain information on all markers in the
	// given sfm set. However, when loading the markers in the list boxes (see LoadListBoxFromArrays below)
	// only markers that have pSfm->userCanSetFilter == TRUE are actually loaded into the list boxes for view
	// by the user. We need the arrays to contain all the markers so we can easily build the 
	// gProjectFilterMarkersForConfig string, etc.
	//
	// Now, in the wx version, individual elements of the filter flags array are changed by simply checking
	// or unchecking the list items' check boxes.
	MapSfmToUSFMAnalysisStruct::iterator iter;
	USFMAnalysis* pSfm;
	wxString key;
	wxString lbStr;

	// initialize arrays
	pSfmMarkerAndDescr->Clear();
	pFilterFlags->Clear();
	pUserCanSetFlags->Clear();

	// first add the known markers to the list box
	for (iter = pMap->begin(); iter != pMap->end(); ++iter)
	{
		// Retrieve each USFMAnalysis struct from the map
		key = iter->first;
		pSfm = iter->second;

		// we only show those sfms that the user can set in the wx version
		// wx version has no bShowAll
		lbStr = gSFescapechar;
		lbStr += pSfm->marker;
		lbStr += _T(" "); // separate the whole marker from the descriiption with a space
		lbStr = _T(' ') + lbStr; // prefix one initial space for normal markers (unknown markers prefixed by two spaces)
		lbStr += pSfm->description;
		// add the marker-description string to its array
		// Note: Since the USFMAnalysis maps only contain one of each marker (each marker is
		// unique in each map), there should not be any duplicate markers in any of these
		// filter arrays (pSfmMarkerAndDescr, pFilterFlags, pUserCanSetFlags).
		pSfmMarkerAndDescr->Add(lbStr);
		// Set filter flags using USFM map data or the filter marker string
		// parameter depending on value of useStrOrMap parameter. 
		// Note: Unknown markers are added to the Doc's list in the caller by invoking 
		// the usfm filter page's AddUnknownMarkersToDocArrays()
		if (useStrOrMap == useMap)
			pFilterFlags->Add(pSfm->filter == TRUE);
		else if (useStrOrMap == useString)
		{
			wxString wholeMkr = gSFescapechar + pSfm->marker + _T(' ');
			if (filterMkrStr.Find(wholeMkr) != -1)
				pFilterFlags->Add(TRUE);
			else
				pFilterFlags->Add(FALSE);
		}
		// set the userCanSetFilter flag
		pUserCanSetFlags->Add(pSfm->userCanSetFilter == TRUE);
	}
}

void CUsfmFilterPageCommon::LoadListBoxFromArrays(wxArrayString* pSfmMarkerAndDescr, 
		wxArrayInt* pFilterFlags, wxArrayInt* pUserCanSetFlags, wxCheckListBox* pListBox)
{
	pListBox->Clear(); // clears all items out of list
	// Populate the given input list box (doc, proj). Uses the data stored in the 
	// sfm marker&description array and the filter flags array.
	int markerCount, index;
	wxString tempStr, wholeMkr, bareMkr;
	markerCount = pSfmMarkerAndDescr->GetCount();
	for (index = 0; index < markerCount; index++)
	{
		// wx version: We do not have a "Show All" button and we only list markers that the
		// user can set flags.
		if (pUserCanSetFlags->Item(index) == TRUE)
		{
			// when we are not showing all markers we only show those with 
			// userCanSetFilter attribute

			// testing only !!!
			wxString test = pSfmMarkerAndDescr->Item(index);
			// testing only !!!

			int listBoxIndex;
			listBoxIndex = pListBox->Append(pSfmMarkerAndDescr->Item(index));
			// whm comment: MFC's FindStringExact and FindString do NOT do a case sensitive match 
			// by default so we'll use our own more flexible FindListBoxItem method below with 
			// caseSensitive and exactString parameters.
			//int listBoxIndex = pListBox->FindStringExact(-1,pSfmMarkerAndDescr->GetAt(index));
			//int listBoxIndex = gpApp->FindListBoxItem(pListBox, pSfmMarkerAndDescr->Item(index), caseSensitive, exactString);
			wxASSERT(listBoxIndex != -1); //LB_ERR
			if (listBoxIndex != -1) //LB_ERR
			{
				// the string is in the listbox
				// testing only !!!
				bool btest;
				btest = (pFilterFlags->Item(index) == TRUE);
				// testing only !!!

				pListBox->Check(listBoxIndex,pFilterFlags->Item(index) == TRUE); //pListBox->SetCheck(listBoxIndex,pFilterFlags->GetAt(index) == TRUE);
				// MFC used SetItemData. wxCheckListBox has SetClientData and GetClientData, but it is
				// used internally in the implementation of the class, so we can't use it here as we did
				// in the MFC version. Instead we'll just use GetSelection() to determine the actual index 
				// of the selection (this works even in a sorted list). 
			}
		}
	}
	if (pListBox->IsEnabled())
	{
		pListBox->SetFocus();
		// Make sure that initially the first item is selected in pListBox
	}
	wxASSERT(pListBox->GetCount() > 0);
	pListBox->SetSelection(0); // select first item in list
}


void CUsfmFilterPageCommon::LoadDocSFMListBox(enum ListBoxProcess lbProcess)
{
	// if lbProcess == LoadInitialDefaults this function removes any previous elements
	// from the doc's m_SfmMarkerAndDescriptionsDoc, m_filterFlagsDoc, and 
	// m_filterFlagsDocBeforeEdit arrays, and loads them and the list box afresh from
	// the global style maps stored on the App. If lbProcess == ReloadAndUpdateFromProjList
	// the arrays are first reloaded from the global style maps stored on the App, and
	// then the filter settings of the project's filter marker list are "copied" to the
	// doc's filter marker list (actually just the flags are made to agree and the list
	// box updated). If lbProcess == ReloadAndUpdateFromFactoryList the process does the 
	// same, but uses the filter settings of the factory's filter marker list.

	// Ensure that the arrays are empty before loading them.
	pSfmMarkerAndDescriptionsDoc->Clear();
	pFilterFlagsDoc->Clear();
	pFilterFlagsDocBeforeEdit->Clear();
	pUserCanSetFilterFlagsDoc->Clear();

	MapSfmToUSFMAnalysisStruct* pMap;
	wxString filterMkrStr;
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences 
		// Using the current sfm set get the appropriate USFMAnalysis struct map
		pMap = gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc);
	}
	else
	{
		// We're in the Wizard
		// using the current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc);
	}

	if (lbProcess == LoadInitialDefaults) // whm added 28Sep06 to make Undo work properly
	{
		//tempFilterMarkersAfterEditDoc = gpApp->gCurrentFilterMarkers;
		// No, the above doesn't work when the sfmSet has changed. tempFilterMarkersAfterEditDoc
		// needs to be assigned the filter markers for the currently chosen (temp) sfm set for the 
		// Doc, which is indicated by tempSfmSetAfterEditDoc. 
		// This correction could also be propagated back to the MFC version.
		if (pStartWorkingWizard == NULL)
		{
			// We're in Edit Preferences
			// so use pUSFMPageInPrefs
			if (bDocSfmSetChanged)
			{
				switch (tempSfmSetAfterEditDoc)
				{
				case UsfmOnly: tempFilterMarkersAfterEditDoc = gpApp->UsfmFilterMarkersStr;
					break;
				case PngOnly: tempFilterMarkersAfterEditDoc = gpApp->PngFilterMarkersStr;
					break;
				case UsfmAndPng: tempFilterMarkersAfterEditDoc = gpApp->UsfmAndPngFilterMarkersStr;
					break;
				default: tempFilterMarkersAfterEditDoc = gpApp->UsfmFilterMarkersStr;
				}
			}
			else
			{
				// The Doc sfm set has not changed in this preferences session so use
				// the list of filter markers in gCurrentFilterMarkers
				tempFilterMarkersAfterEditDoc = gpApp->gCurrentFilterMarkers;
			}
#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(6); // location 6
#endif

		}
		else
		{
			// We're in the Wizard
			// so use pUsfmPageWiz
			if (bDocSfmSetChanged)
			{
				switch (tempSfmSetAfterEditDoc)
				{
				case UsfmOnly: tempFilterMarkersAfterEditDoc = gpApp->UsfmFilterMarkersStr;
					break;
				case PngOnly: tempFilterMarkersAfterEditDoc = gpApp->PngFilterMarkersStr;
					break;
				case UsfmAndPng: tempFilterMarkersAfterEditDoc = gpApp->UsfmAndPngFilterMarkersStr;
					break;
				default: tempFilterMarkersAfterEditDoc = gpApp->UsfmFilterMarkersStr;
				}
			}
			else
			{
				// The Doc sfm set has not changed in this preferences session so use
				// the list of filter markers in gCurrentFilterMarkers
				tempFilterMarkersAfterEditDoc = gpApp->gCurrentFilterMarkers;
			}
#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(7); // location 7
#endif

		}
	}

	filterMkrStr = tempFilterMarkersAfterEditDoc;

	// whm added 19Jan09 the following wxStringArray to contain a list of the markers and descriptions
	// that will only appear in the actual FilterPage listboxes. This pSfmMarkerAndDescForFormatting
	// array will only contain any markers that have their pSfm->userCanSetFilter == TRUE. This array
	// is then passed to FormatMarkerAndDescriptionsStringArray() below which will provide appropriate
	// space padding between the marker and its description in the list box. Previously we passed the
	// complete set of markers in pSfmMarkerAndDescriptionsDoc to the formatting function, but this
	// made for too much space between the marker and descriptions because some of the longer markers
	// never appear in the list boxes because they are pSfm->userCanSetFilter == FALSE. I've added
	// another parameter to SetupFilterPageArrays() to populate this pSfmMarkerAndDescForFormatting
	// array since the natural place to do it is within SetupFilterPageArrays().
	wxArrayString SfmMarkerAndDescForFormatting;
	wxArrayString* pSfmMarkerAndDescForFormatting = &SfmMarkerAndDescForFormatting;
	pSfmMarkerAndDescForFormatting->Clear();

	SetupFilterPageArrays(pMap, filterMkrStr, useString, pSfmMarkerAndDescriptionsDoc, 
		pFilterFlagsDoc, pUserCanSetFilterFlagsDoc);

	// If the usfm filter page is called from the wizard no doc is open so we cannot
	// access markers unknown to the doc.
	if (pStartWorkingWizard == NULL)
	{
		// We are not being called from the wizard
		// Reinventory unknown markers from Doc and Add them to the Doc's arrays
		AddUnknownMarkersToDocArrays();
	}

	wxClientDC aDC((wxWindow*)gpApp->GetMainFrame()->canvas);
	gpApp->FormatMarkerAndDescriptionsStringArray(&aDC, 
			pSfmMarkerAndDescriptionsDoc, 2, pUserCanSetFilterFlagsDoc);

	if (lbProcess == ReloadAndUpdateFromProjList || lbProcess == ReloadAndUpdateFromFactoryList)
	{
		// When lbProcess == ReloadAndUpdateFromProjList or ReloadAndUpdateFromFactoryList
		// we get here because the user clicked on a "Copy to Document" button to copy filter markers
		// from the Proj or Factory list to the Doc's list, and there was detected in the copy handler
		// a mismatch between the sfm set of the copy "from" list and the sfm set of the copy "to" list.
		// We've told the user there that it is necessary to also change the sfm set of the Doc list to
		// agree in the process. So, we need to also reinitialize the "before edit" flags by copying 
		// the "after edit" flags in pFilterFlagsDoc. The later was repopulated by the 
		// SetupFilterPageArrays and AddUnknownMarkersToDocArrays calls above
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));

	}

	LoadListBoxFromArrays(pSfmMarkerAndDescriptionsDoc, 
		pFilterFlagsDoc, pUserCanSetFilterFlagsDoc, pListBoxSFMsDoc);

	if (lbProcess == LoadInitialDefaults)
	{
		// Copy the elements of pFilterFlagsDoc to pFilterFlagsDocBeforeEdit
		// The calling routine can determine if any filtering changes
		// have been made by comparing the two CUIntArrays. This copy operation
		// in which before and after edit flags are set should only be done when 
		// the dialog is first created and when the user clicks on the Doc list's 
		// Undo button.
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
	}
	else if (lbProcess == UndoEdits)
	{

		pFilterFlagsDoc->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDocBeforeEdit->GetCount(); ct++)
			pFilterFlagsDoc->Add(pFilterFlagsDocBeforeEdit->Item(ct));

	}
}

void CUsfmFilterPageCommon::LoadProjSFMListBox(enum ListBoxProcess lbProcess)
{
	// Loads into the project list box the markers for the gProjectSfmSetForConfig, setting the
	// filtering status of each marker so that only those that are listed
	// in the gProjectFilterMarkersForConfig string are checked for filtering.

	// Ensure that the arrays are empty before loading them.
	pSfmMarkerAndDescriptionsProj->Clear();
	pFilterFlagsProj->Clear();
	pFilterFlagsProjBeforeEdit->Clear();
	pUserCanSetFilterFlagsProj->Clear();

	MapSfmToUSFMAnalysisStruct* pMap;
	wxString filterMkrStr;
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences
		// using the current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(tempSfmSetAfterEditProj);
	}
	else
	{
		// We're in the Wizard
		// using the current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(tempSfmSetAfterEditProj);
	}

	if (lbProcess == LoadInitialDefaults) // whm added 28Sep06 to enable Undo to work correctly
	{
		//tempFilterMarkersAfterEditProj = gpApp->gProjectFilterMarkersForConfig;
		// No, the above doesn't work when the sfmSet has changed. tempFilterMarkersAfterEditProj
		// needs to be assigned the filter markers for the currently chosen (temp) sfm set for the 
		// project, which is indicated by tempSfmSetAfterEditProj. 
		// This correction could also be propagated back to the MFC version.
		if (pStartWorkingWizard == NULL)
		{
			// We're in Edit Preferences
			// so use pUSFMPageInPrefs
			if (bProjectSfmSetChanged)
			{
				switch (tempSfmSetAfterEditProj)
				{
				case UsfmOnly: tempFilterMarkersAfterEditProj = gpApp->UsfmFilterMarkersStr;
					break;
				case PngOnly: tempFilterMarkersAfterEditProj = gpApp->PngFilterMarkersStr;
					break;
				case UsfmAndPng: tempFilterMarkersAfterEditProj = gpApp->UsfmAndPngFilterMarkersStr;
					break;
				default: tempFilterMarkersAfterEditProj = gpApp->UsfmFilterMarkersStr;
				}
			}
			else
			{
				// project sfm set has not changed in this preferences session so use
				// the list of filter markers in gProjectFilterMarkersForConfig
				tempFilterMarkersAfterEditProj = gpApp->gProjectFilterMarkersForConfig;
			}
#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(8); // location 8
#endif

		}
		else
		{
			// We're in the Wizard
			// so use pUsfmPageWiz
			if (bProjectSfmSetChanged)
			{
				switch (tempSfmSetAfterEditProj)
				{
				case UsfmOnly: tempFilterMarkersAfterEditProj = gpApp->UsfmFilterMarkersStr;
					break;
				case PngOnly: tempFilterMarkersAfterEditProj = gpApp->PngFilterMarkersStr;
					break;
				case UsfmAndPng: tempFilterMarkersAfterEditProj = gpApp->UsfmAndPngFilterMarkersStr;
					break;
				default: tempFilterMarkersAfterEditProj = gpApp->UsfmFilterMarkersStr;
				}
			}
			else
			{
				// project sfm set has not changed in this preferences session so use
				// the list of filter markers in gProjectFilterMarkersForConfig
				tempFilterMarkersAfterEditProj = gpApp->gProjectFilterMarkersForConfig;
			}
#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(9); // location 9
#endif

		}
	}

	filterMkrStr = tempFilterMarkersAfterEditProj;

	SetupFilterPageArrays(pMap, filterMkrStr, useString, pSfmMarkerAndDescriptionsProj, 
		pFilterFlagsProj, pUserCanSetFilterFlagsProj);

	// do not call AddUnknownMarkersToDocArrays here, only for the Doc's list

	wxClientDC aDC((wxWindow*)gpApp->GetMainFrame()->canvas);
	gpApp->FormatMarkerAndDescriptionsStringArray(&aDC, 
			pSfmMarkerAndDescriptionsProj, 2, pUserCanSetFilterFlagsProj);

	if (lbProcess == ReloadAndUpdateFromDocList || lbProcess == ReloadAndUpdateFromFactoryList)
	{
		// If the Usfm Filter page forced a change of sfm set because of a copy button operation, i.e., 
		// lbProcess == ReloadAndUpdateFromDocList or lbProcess == ReloadAndUpdateFromFactoryList, 
		// we start a new baseline by reinitializing (copying) the Proj's filter flags before edit
		// from the Proj's filter flags after edit (which was repopulated by the SetupFilterPageArrays
		// call above).
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
		{
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		}
	}

	LoadListBoxFromArrays(pSfmMarkerAndDescriptionsProj, 
		pFilterFlagsProj, pUserCanSetFilterFlagsProj, pListBoxSFMsProj);

	if (lbProcess == LoadInitialDefaults)
	{
		// Copy the elements of pFilterFlagsProj to pFilterFlagsProjBeforeEdit
		// The calling routine can determine if any filtering changes
		// have been made by comparing the two CUIntArrays. This copy operation
		// in which before and after edit flags are set should only be done when 
		// the dialog is first created and when the user clicks on the Proj list's 
		// Undo button.
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
	}
	else if (lbProcess == UndoEdits)
	{

		pFilterFlagsProj->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
		{
			pFilterFlagsProj->Add(pFilterFlagsProjBeforeEdit->Item(ct));
		}
	}
}

wxString CUsfmFilterPageCommon::GetSetNameStr(enum SfmSet set)
{
	wxString UsfmOnlyStr;
	wxString PngOnlyStr;
	wxString UsfmAndPngStr;
	UsfmOnlyStr = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	PngOnlyStr = _("PNG 1998 Marker Set"); //IDS_PNG_SET_ABBR
	UsfmAndPngStr = _("USFM 2.0 and PNG 1998 Marker Sets"); //IDS_BOTH_SETS_ABBR
	switch (set)
	{
	case UsfmOnly: return UsfmOnlyStr;
	case PngOnly: return PngOnlyStr;
	case UsfmAndPng: return UsfmAndPngStr;
	default: return UsfmOnlyStr;
	}
}

wxString CUsfmFilterPageCommon::GetFilterMkrStrFromFilterArrays(wxArrayString* pSfmMarkerAndDescr, wxArrayInt* pFilterFlags)
{
	wxString wholeMkr, tempStr, testStr;
	bool testFlag = FALSE;
	wholeMkr.Empty();
	tempStr.Empty();
	testStr.Empty();
	int markerCt = pFilterFlags->GetCount();
	wxASSERT(markerCt == (int)pSfmMarkerAndDescr->GetCount());
	for (int ct = 0; ct < markerCt; ct++)
	{
		testStr = pSfmMarkerAndDescr->Item(ct); // for inspection only
		testFlag = pFilterFlags->Item(ct) == 1; // for inspection only
		if (pFilterFlags->Item(ct) == TRUE)
		{
			// the marker is a filter marker, so parse it out and accumulate it in tempStr
			// whm modification 23Sep10. The pSfmMarkerAndDescr item has an initial space,
			// followed by the marker (never has embedded spaces), followed by one or more
			// spaces (to make descriptions part have a fairly even alignment to the right
			// of the markers), followed by the actual description. Here we parse out the
			// marker to get the filter marker string part. It seems that in some cases 
			// there is only a single space between the marker and the following description
			// so we'll deal with parsing a little differently: first, trim initial whitespace
			// from the string, then look for a single, instead of a double space.
			wholeMkr = pSfmMarkerAndDescr->Item(ct);
			wholeMkr.Trim(FALSE);
			int spPos = wholeMkr.Find(_T(' '));
			wxASSERT(spPos != -1);
			wholeMkr = wholeMkr.Mid(0,spPos);
			wholeMkr.Trim(TRUE); // trim right end
			// ensure marker ends with a space
			wholeMkr += _T(' ');
			tempStr += wholeMkr; // add marker to string
		}
	}
	return tempStr;
}


void CUsfmFilterPageCommon::AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker)
{
	// if the wholeMarker does not already exist in filterMkrStr, append it.
	// Assumes wholeMarker begins with backslash, and insures it ends with a delimiting space.
	wholeMarker.Trim(TRUE); // trim right end
	wholeMarker.Trim(FALSE); // trim left end
	wxASSERT(!wholeMarker.IsEmpty());
	// then add the necessary final space
	wholeMarker += _T(' ');
	if (filterMkrStr.Find(wholeMarker) == -1)
	{
		// The wholeMarker doesn't already exist in the string, so append it.
		// NOTE: By appending a marker to the filter marker string we are creating a 
		// string that can no longer be compared with other filter marker strings by
		// means of the == or != operators. Comparison of such filter marker strings will now
		// necessarily require a special function StringsContainSameMarkers() be used
		// in every place where marker strings are compared.
		filterMkrStr += wholeMarker;
	}
}


void CUsfmFilterPageCommon::RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker)
{
	// if the wholeMarker already exists in filterMkrStr, remove it.
	// Assumes wholeMarker begins with backslash, and insures it ends with a delimiting space.
	wholeMarker.Trim(TRUE); // trim right end
	wholeMarker.Trim(FALSE); // trim left end
	wxASSERT(!wholeMarker.IsEmpty());
	// then add the necessary final space
	wholeMarker += _T(' ');
	int posn = filterMkrStr.Find(wholeMarker);
	if (posn != -1)
	{
		// The wholeMarker does exist in the string, so remove it.
		// NOTE: By removing a marker from the filter marker string we are creating a 
		// string that can no longer be compared with other filter marker strings by
		// means of the == or != operators. Comparison of such filter marker strings will now
		// necessarily require a special function StringsContainSameMarkers() be used
		// in every place where marker strings are compared.
		filterMkrStr.Remove(posn, wholeMarker.Length());
	}
}

void CUsfmFilterPageCommon::AdjustFilterStateOfUnknownMarkerStr(wxString& unknownMkrStr, wxString wholeMarker, enum UnkMkrFilterSetting filterSetting)
{
	// The caller insures that the wholeMarker already exists in unknownMkrStr.
	// Assumes wholeMarker begins with backslash.
	wholeMarker.Trim(TRUE); // trim right end
	wholeMarker.Trim(FALSE); // trim left end
	wxASSERT(wholeMarker[0] == gSFescapechar);
	wxASSERT(!wholeMarker.IsEmpty());
	// Unknown marker strings are formatted with one or more markers each having the form of "\xx=0 " or "\xx=1 "
	// At this point we have a whole marker in the form of "\xx". To uniquely find the position of the
	// unknown marker in unknownMkrStr we suffix an = to it
	wholeMarker += _T('=');
	int posn = unknownMkrStr.Find(wholeMarker);
	wxASSERT(posn != -1);
	// at this point whole marker is of the form "\xx=", and the target string in unknownMkrStr
	// should be of the form "\xx=n " so there should be at least 2 valid character positions
	// in unknwonMkrStr beyond posn + wholeMarker.GetLength(). there should be at least
	wxASSERT(posn + wholeMarker.Length() +2 <= unknownMkrStr.Length());
	// program defensively
	if (posn + wholeMarker.Length() +2 <= unknownMkrStr.Length())
	{
		wxASSERT(unknownMkrStr[posn + wholeMarker.Length()] == _T('0') 
			|| unknownMkrStr[posn + wholeMarker.Length()] == _T('1'));
		// Change the filter status indicator according to the filterSetting parameter.
		if (filterSetting == setAsFiltered)
		{
			unknownMkrStr[posn + wholeMarker.Length()] = _T('1');
		}
		else // filterSetting == setAsUnfiltered
		{
			unknownMkrStr[posn + wholeMarker.Length()] = _T('0');
		}
	}
}

void CUsfmFilterPageCommon::DoLbnSelchangeListSfmsDoc()
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pListBoxSFMsDoc->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBoxSFMsDoc))
		return;

	// With the removal of the legacy buttons, all that is needed is to insure we have a valid selection
	// If we get here we know there is at least one item in the list. 
	int nSel;
	nSel = pListBoxSFMsDoc->GetSelection();
	//if (nSel == -1)
	//{
	//	// no selection, so select the first item in list
	//	nSel = 0;
	//}
	pListBoxSFMsDoc->SetSelection(nSel);
}

void CUsfmFilterPageCommon::DoCheckListBoxToggleDoc(wxCommandEvent& event)
{
	// Note: the checkbox that gets toggled, may not be the item currently selected in the
	// listbox, since the user can click directly on a checkbox without first selecting the
	// listbox line containing the checkbox. The code below automatically selects the line
	// if the checkbox is directly clicked on to check or uncheck it.
	// The Linux (GTK) and MAC ports do not yet implement the background coloring of individual items
	// of a wxCheckListBox so we cannot yet represent "invalid" choices in the list by a gray 
	// background under GTK or the MAC. Possible design solutions: 
	// 1. Don't list any markers in the list that we don't want the user to select 
	// 2. List all markers, but the GUI gives a beep or message to the user if he tries to 
	//    check the box of an "invalid" choice.
	// 3. Implement MFC like behavior only for Windows port, but behavior 1 or 2 for the
	//    Linux and Mac ports.
	// A naive user would probably be less confused with solution 1 (don't list any invalid markers),
	// so I've implemented that design solution in LoadActiveSFMListBox().
	int nItem = event.GetInt(); // get the index of the list box item whose box has changed
	//wxString itemString = pListBoxSFMsDoc->GetString(nItem);
	pListBoxSFMsDoc->SetSelection(nItem); // insure that the selection highlights the line having the clicked box

	DoBoxClickedIncludeOrFilterOutDoc(nItem);
}

void CUsfmFilterPageCommon::DoCheckListBoxToggleProj(wxCommandEvent& event)
{
	// Note: the checkbox that gets toggled, may not be the item currently selected in the
	// listbox, since the user can click directly on a checkbox without first selecting the
	// listbox line containing the checkbox. The code below automatically selects the line
	// if the checkbox is directly clicked on to check or uncheck it.
	// The Linux (GTK) and MAC ports do not yet implement the background coloring of individual items
	// of a wxCheckListBox so we cannot yet represent "invalid" choices in the list by a gray 
	// background under GTK or the MAC. Possible design solutions: 
	// 1. Don't list any markers in the list that we don't want the user to select 
	// 2. List all markers, but the GUI gives a beep or message to the user if he tries to 
	//    check the box of an "invalid" choice.
	// 3. Implement MFC like behavior only for Windows port, but behavior 1 or 2 for the
	//    Linux and Mac ports.
	// A naive user would probably be less confused with solution 1 (don't list any invalid markers),
	// so I've implemented that design solution in LoadActiveSFMListBox().
	int nItem = event.GetInt();
	pListBoxSFMsProj->SetSelection(nItem); // insure that the selection highlights the line having the clicked box

	DoBoxClickedIncludeOrFilterOutProj(nItem);
}

void CUsfmFilterPageCommon::DoBoxClickedIncludeOrFilterOutDoc(int lbItemIndex)
{
	// Note: whm 23May07 simplified the usfm filter page by removing the "Filter Out"/
	// "Include For Adaptation" and "Undo" buttons.
	//
	// This method reads the state of the checkbox of the listbox item in
	// the pListBoxSFMsProj, and adjusts the array flags accordingly.
	// DoBoxClickedIncludeOrFilterOutDoc() does nothing to the global USFMAnalysis 
	// structs which is left to the calling routine. If the user cancels the 
	// containing dialog, no changes will be made to the global data structures.
	// The calling routine (OnEditPreferences, or DoStartWorkingWizard) 
	// should check for any changes by storing the contents of 
	// m_filterFlagsProjBeforeEdit and comparing that with m_filterFlagsProj 
	// after the user dismisses the dialog with the OK button. If the comparison
	// shows that changes were made the caller should then make the 
	// required changes to the active USFMAnalysis structs and make the necessary 
	// calls to rebuild the current document.
	
	int curSel = pListBoxSFMsDoc->GetSelection();
	// The caller of DoCheckListBoxToggleDoc() selects the listbox item if the checkbox is 
	// changed without moving the selection (i.e., by a direct click on the checkbox)
	lbItemIndex = lbItemIndex; // avoid compiler warning in release builds
	wxASSERT(curSel == lbItemIndex);

	// we need the marker description to find the items array index and to use for
	// a possible warning message.
	wxString selectedString = pListBoxSFMsDoc->GetStringSelection();
	//int actualArrayIndex = *(int*)pListBoxSFMsDoc->GetClientData(curSel);
	// whm: wx version cannot use the wxCheckListBox's ClientData to map the sorted index of
	// the list box with the unsorted array index of the stored data, so I wrote
	// the following FindArrayString() function to find the string's element index
	// in the unsorted array. FindArrayString finds only exact string matches, and 
	// is case sensitive.
	int actualArrayIndex = gpApp->FindArrayString(selectedString,pSfmMarkerAndDescriptionsDoc); //pListBoxSFMsDoc->FindString(selectedString);
	wxASSERT(actualArrayIndex < (int)pFilterFlagsDoc->GetCount());
	
	wxString checkStr, unkStr;
	// get the list box line item marker and description
	checkStr = pSfmMarkerAndDescriptionsDoc->Item(actualArrayIndex);
	checkStr.Trim(TRUE); // trim right end
	checkStr.Trim(FALSE); // trim left end
	// extract the whole marker part
	checkStr = checkStr.Mid(0,checkStr.Find(_T(' ')));
	
	// insure that the flag stored in the flags array reflects the new visible state of the 
	// checkbox
	if (pListBoxSFMsDoc->IsChecked(curSel))
	{
		// Checkbox was previously unchecked (include for adaptation), now it is 
		// checked (filter out).
		// Before effecting the change, notify user that Filtering a marker 
		// will result in loss of any adaptations previously done of the text 
		// associated with that marker when it was included for adaptation. This
		// warning should only appear when done from Edit Preferences.
		if (pStartWorkingWizard == NULL)
		{
			if (!bFirstWarningGiven)
			{
				bFirstWarningGiven = TRUE;
				wxString warnText;
				// IDS_FILTER_WARNING
				warnText = warnText.Format(_("Warning: You are about to filter out the following marker:\n   \"%s \"\nAny text associated with this marker that was previously included\nin adaptations you have already done, will also be filtered out.\n\nDo you really want to filter out this marker? "), 
					pSfmMarkerAndDescriptionsDoc->Item(actualArrayIndex).c_str());
				int response = wxMessageBox(warnText, _T(""), wxYES_NO | wxICON_WARNING);
				if (response != wxYES) //if (response != IDYES)
				{
					// User aborted so remove the check from the checkbox
					pListBoxSFMsDoc->Check(curSel,FALSE);
					return;
				}
			}
		}
		// update the filter flags data array
		(*pFilterFlagsDoc)[actualArrayIndex] = TRUE;

		// add this whole marker to tempFilterMarkersAfterEditProj
		AddFilterMarkerToString(tempFilterMarkersAfterEditDoc, checkStr); // whm added 16Jun05
	}
	else
	{
		// checkbox was previously checked (filter out), now it is unchecked (include for adaptation)
		// update the filter flags data array
		(*pFilterFlagsDoc)[actualArrayIndex] = FALSE;
		
		// remove this whole marker from tempFilterMarkersAfterEditDoc
		RemoveFilterMarkerFromString(tempFilterMarkersAfterEditDoc, checkStr); // whm added 16Jun05
	}
	
	// Added 31May05. Check if the marker being included for adaptation is an unknown marker, 
	// and if so, set its flag appropriately in m_filterFlagsUnkMkrs

#ifdef _Trace_UnknownMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("In FilterPage User clicked INCLUDE FOR ADAPTING - BEFORE adjusting filter flag:\n");
	TRACE1("               Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1("       Doc's m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
	TRACE1("         Filter pg's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&m_unknownMarkers, &m_filterFlagsUnkMkrs));
	TRACE1(" Filter Pg's m_currentUnknownMarkersStr = %s\n", m_currentUnknownMarkersStr);
#endif

	// Process any unknown markers
	int unkIndex;
	bool bUnkMkrFound = FALSE;
	// compare checkStr with each unknown marker in m_unknownMarkers
	for (unkIndex = 0; unkIndex < (int)m_filterFlagsUnkMkrs.GetCount(); unkIndex++)
	{
		// get the unknown marker
		unkStr = m_unknownMarkers.Item(unkIndex);
		unkStr.Trim(TRUE); // trim right end
		unkStr.Trim(FALSE); // trim left end
		// are they the same marker?
		if (unkStr == checkStr)
		{
			// we found it
			bUnkMkrFound = TRUE;
			break;
		}
	}
	if (bUnkMkrFound)
	{
		// mark the unknown marker's filter flag as unfiltered (FALSE)
		// unkIndex is the index of the found unknown marker
		m_filterFlagsUnkMkrs[unkIndex] = FALSE;
		// Change the found marker's suffix from "=1" to "=0" in m_currentUnknownMarkersStr
		AdjustFilterStateOfUnknownMarkerStr(m_currentUnknownMarkersStr, checkStr, setAsUnfiltered);// whm added 2Jul05

#ifdef _Trace_UnknownMarkers
		TRACE0("In FilterPage UNKNOWN MARKER INCLUDE FOR ADAPTING - AFTER adjusting filter flag:\n");
		TRACE1("          Uknown Marker Now Unfiltered  = %s\n",checkStr);
		TRACE1("               Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
		TRACE1("       Doc's m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
		TRACE1("         Filter pg's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&m_unknownMarkers, &m_filterFlagsUnkMkrs));
		TRACE1(" Filter Pg's m_currentUnknownMarkersStr = %s\n", m_currentUnknownMarkersStr);
#endif

	}
	else
	{
#ifdef _Trace_UnknownMarkers
		TRACE1("In FilterPage MARKER Included for Adapting was NOT an UNKNOWN MARKER - it was %s\n",checkStr);
#endif
	}

	if (pListBoxSFMsDoc->IsEnabled())
		pListBoxSFMsDoc->SetFocus();
	pListBoxSFMsDoc->SetSelection(curSel);

	bDocFilterMarkersChanged = TRUE;
}

void CUsfmFilterPageCommon::DoLbnSelchangeListSfmsProj()
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pListBoxSFMsProj->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBoxSFMsProj))
		return;

	// With the removal of the buttons, all that is needed is to insure we have a valid selection
	// If we get here we know there is at least one item in the list. 
	int nSel;
	nSel = pListBoxSFMsProj->GetSelection();
	//if (nSel == -1)
	//{
	//	// no selection, so select the first item in list
	//	nSel = 0;
	//}
	pListBoxSFMsProj->SetSelection(nSel);
}

void CUsfmFilterPageCommon::DoBoxClickedIncludeOrFilterOutProj(int lbItemIndex)
{
	// Note: whm 23May07 simplified the usfm filter page by removing the "Filter Out"/
	// "Include For Adaptation" and "Undo" buttons.
	//
	// This method reads the state of the checkbox of the listbox item in
	// the pListBoxSFMsProj, but does nothing to the global USFMAnalysis 
	// structs which is left to the calling routine. If the user cancels the 
	// containing dialog, no changes will be made to the global data structures.
	// The calling routine (OnEditPreferences, or DoStartWorkingWizard) 
	// should check for any changes by storing the contents of 
	// m_filterFlagsProjBeforeEdit and comparing that with m_filterFlagsProj 
	// after the user dismisses the dialog with the OK button. If the comparison
	// shows that changes were made the caller should then make the 
	// required changes to the active USFMAnalysis structs and make the necessary 
	// calls to rebuild the current document.
	
	int curSel = pListBoxSFMsProj->GetSelection();
	// The caller of DoCheckListBoxToggleProj() selects the listbox item if the checkbox is 
	// changed without moving the selection (i.e., by a direct click on the checkbox)
	lbItemIndex = lbItemIndex; // avoid compiler warning in release builds
	wxASSERT(curSel == lbItemIndex);

	// we need the marker description to find the items array index and to use for
	// a possible warning message.
	wxString selectedString = pListBoxSFMsProj->GetStringSelection();
	//int actualArrayIndex = *(int*)pListBoxSFMsProj->GetClientData(curSel); // index into m_asSfmFlags
	// whm: wx version cannot use the wxCheckListBox's ClientData to map the sorted index of
	// the list box with the unsorted array index of the stored data, so I wrote
	// the following FindArrayString() function to find the string's element index
	// in the unsorted array. FindArrayString finds only exact string matches, and
	// is case sensitive.
	int actualArrayIndex = gpApp->FindArrayString(selectedString,pSfmMarkerAndDescriptionsProj); //pListBoxSFMsProj->FindString(selectedString); // index into m_asSfmFlags
	wxASSERT(actualArrayIndex < (int)pFilterFlagsProj->GetCount());
	
	wxString checkStr;
	// get the list box line item marker and description
	checkStr = pSfmMarkerAndDescriptionsProj->Item(actualArrayIndex);
	checkStr.Trim(TRUE); // trim right end
	checkStr.Trim(FALSE); // trim left end
	// extract the whole marker part
	checkStr = checkStr.Mid(0,checkStr.Find(_T(' ')));
	
	// insure that the flag stored in the flags array reflects the new visible state of the 
	// checkbox
	if (pListBoxSFMsProj->IsChecked(curSel))
	{
		// Checkbox was previously unchecked (include for adaptation), now it is 
		// checked (filter out).
		// no processing of unknown markers in Proj

		// update the filter flags data array
		(*pFilterFlagsProj)[actualArrayIndex] = TRUE;

		// add this whole marker to tempFilterMarkersAfterEditProj
		AddFilterMarkerToString(tempFilterMarkersAfterEditProj, checkStr); // whm added 16Jun05
	}
	else
	{
		// checkbox was previously checked (filter out), now it is unchecked (include for adaptation)
		// update the filter flags data array
		(*pFilterFlagsProj)[actualArrayIndex] = TRUE;

		// remove this whole marker from tempFilterMarkersAfterEditProj
		RemoveFilterMarkerFromString(tempFilterMarkersAfterEditProj, checkStr); // whm added 16Jun05
	}

	if (pListBoxSFMsProj->IsEnabled())
		pListBoxSFMsProj->SetFocus();
	pListBoxSFMsProj->SetSelection(curSel);

	bProjectFilterMarkersChanged = TRUE;
}


void CUsfmFilterPageCommon::DoBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& WXUNUSED(event))
{

	// Check for this session changes in the usfm filter page's Doc Filter inventory
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
    // Warning: You just made changes to the document's inventory of filter markers in the
    // Filtering page \nwhere the %1 was in effect. If you make this change the document
    // will use \nthe Filtering page's settings for the %2. Do you want to make this
    // change?
	newSet = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == PngOnly)
			{
				pRadioPngOnly->SetValue(TRUE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}

		// BEW added 11Oct10, to prevent SFM set change to UsfmOnly if the document is
		// marked up for PngOnly (i.e. the PNG 1998 SFM set)
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// the document is not yet defined - don't allow any change at this point
			// (this block can't be entered, as Bill has made the radio buttons for the
			// document's set be disabled when no doc is open; but we'll keep the code for
			// this block for safety's sake)
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == PngOnly)
			{
				pRadioPngOnly->SetValue(TRUE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
						// to the value it had before the button click
			return;
		}
		else
		{
			// there is a document defined
			bool bItsIndeterminate = FALSE;
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
			if (!bItsUSFM && !bItsIndeterminate)
			{
				SfmSet currentSet = gpApp->gCurrentSfmSet;
				bDocSfmSetChanged = FALSE;
				wxString msg2;
				msg2 = msg2.Format(_(
"The currently open document is marked up using the PNG 1998 Marker Set.\nChanging to the %s is not allowed.\n(Because doing so may result in a badly formed document.)"),
				newSet.c_str());
				wxMessageBox(msg2, _T(""), wxICON_WARNING);
				// restore the radio button to what it was before the click
				if (currentSet == PngOnly)
				{
					pRadioPngOnly->SetValue(TRUE);
					pRadioUsfmAndPng->SetValue(FALSE);
					pRadioUsfmOnly->SetValue(FALSE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnly->SetValue(FALSE);
					pRadioUsfmAndPng->SetValue(TRUE);
					pRadioUsfmOnly->SetValue(FALSE);
				}
				tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
						// to the value it had before the button click
				return;
			}
            // if it's not PngOnly, then changing to UsfmOnly or UsfmAndPng is okay, that's
            // also true when the result was indeterminate (FALSE is returned for bItsUSFM
            // in that case too), so let processing continue
		}

		// update tempSfmSetAfterEditDoc with the new sfm set
		tempSfmSetAfterEditDoc = UsfmOnly;
        // Call the usfm filter page's SetupFilterPageArrays to update the filter page's
        // arrays with the new sfm set. Since a new sfm set also means the usfm filter
        // page's tempFilterMarkersAfterEditDoc has markers from the previous sfm set, we
        // need to use the UsfmFilterMarkersStr on the App for building our marker arrays.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		// Initialize the Doc arrays
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

#ifdef __WXDEBUG__
	//gpApp->ShowFilterMarkers(10); // location 10
#endif

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM and Filtering Page User clicked UsfmOnly - Now calling Usfm Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);
 
#ifdef __WXDEBUG__
//	gpApp->ShowFilterMarkers(17); // location 17
#endif

        // At this point the m_filterFlagsDocBeforeEdit CUIntArray and the
        // tempFilterMarkersBeforeEditDoc are populated with data based on a different sfm
        // set, so, in order to be able to use them to test for "before edit/after edit"
        // changes, we need to initialize them again. The pFilterFlagsDocBeforeEdit can be
        // copied from the pFilterFlagsDoc array. The tempFilterMarkersBeforeEditDoc is
        // assigned the string of filter markers using GetFilterMkrStrFromFilterArrays,
        // which gets a filter marker string composed from the current arrays (which will
        // include any unknown filter markers because of the AddUnknownMarkersToDocArrays
        // call above).
		int ct;
		pFilterFlagsDocBeforeEdit->Clear();
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadDocSFMListBox(LoadInitialDefaults);

#ifdef __WXDEBUG__
	//gpApp->ShowFilterMarkers(18); // location 18
#endif

	}
	else
	{
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == PngOnly)
			{
				pRadioPngOnly->SetValue(TRUE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}

		// update the tempSfmSetAfterEditDoc with the new sfm set
		tempSfmSetAfterEditDoc = UsfmOnly;
		// See note above before SetupFilterPageArrays call.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

#ifdef __WXDEBUG__
	//gpApp->ShowFilterMarkers(11); // location 11
#endif

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);

#ifdef __WXDEBUG__
	//gpApp->ShowFilterMarkers(19); // location 19
#endif

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
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;

#ifdef __WXDEBUG__
	//gpApp->ShowFilterMarkers(20); // location 20
#endif

	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page DOC Sfm Set changed to UsfmOnly\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& WXUNUSED(event))
{
	// Check for this session changes in the usfm filter page's Doc Filter inventory
	// If changes were made this session there before changing the sfm set here now,
	// warn the user that the change will activate the PngOnly filter settings
	// in effect there in the Filtering page.
	wxString msg, newSet;
	// IDS_WARNING_JUST_CHANGED_FILT_MKRS
    // Warning: You just made changes to the document's inventory of filter markers in the
    // Filtering page \nwhere the %1 was in effect. If you make this change the document
    // will use \nthe Filtering page's settings for the %2. Do you want to make this
    // change?
	newSet = GetSetNameStr(PngOnly); // BEW 14Dec10, use this rather than a literal
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		// we are in Preferences...
		
		if (bDocFilterMarkersChanged)
		{
            // BEW 11Oct10 (actually 5Jan2011). Changed to prevent the user from trying to
            // change filter settings and the SFM set at the one time. This was a
            // potentially damaging; it wasn't enough to warn the user, because he couldn't
            // be expected to know the implications of the proceed versus cancel choices.
            // The OnOK() function will attempt SFM set change first, then attempt
            // filtering changes. What I'm doing now is to provide some checking for
            // attempts to do both jobs at the one time - these checks will be at two
            // places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
            // NOTE: VisStudio DEBUGGER BUG -- if a break point is put in this function, on
            // exit the handler keeps being reentered (the event is not consumed) - at
            // least four times before an OK or Cancel click gets 'seen'; remove the
            // breakpoint and all is well!
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(TRUE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}

		// BEW added 11Oct10, to prevent SFM set change to PngOnly if the document is
		// marked up for USFM
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// the document is not yet defined - don't allow any change at this point
			// (this block can't be entered, as Bill has made the radio buttons for the
			// document's set be disabled when no doc is open; but we'll keep the code for
			// this block for safety's sake)
			wxBell();
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(TRUE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}
		else
		{
			// there is a document defined
			bool bItsIndeterminate = FALSE;
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
			if (bItsUSFM && !bItsIndeterminate)
			{
				SfmSet currentSet = gpApp->gCurrentSfmSet;
				bDocSfmSetChanged = FALSE;
				wxString msg2;
				msg2 = msg2.Format(_(
"The currently open document is marked up as USFM.\nChanging to the %s is not allowed.\n(Because doing so may result in a badly formed document.)"),
				newSet.c_str());
				wxMessageBox(msg2, _T(""), wxICON_WARNING);
				// restore the radio button to what it was before the click
				if (currentSet == UsfmOnly)
				{
					pRadioPngOnly->SetValue(FALSE);
					pRadioUsfmAndPng->SetValue(FALSE);
					pRadioUsfmOnly->SetValue(TRUE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnly->SetValue(FALSE);
					pRadioUsfmAndPng->SetValue(TRUE);
					pRadioUsfmOnly->SetValue(FALSE);
				}
				tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
						// to the value it had before the button click
				return;
			}
            // if it's not USFM, then changing to PngOnly or UsfmAndPng is okay, that's
            // also true when the result was indeterminate (FALSE is returned for bItsUSFM
            // in that case too), so let processing continue
		}

		tempSfmSetAfterEditDoc = PngOnly;
		// Call the usfm filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM and Filtering Page User clicked PngOnly - Now calling Usfm Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		//pFilterFlagsDocBeforeEdit->Copy(*pFilterPgInPrefs->pFilterFlagsDoc);
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(TRUE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(TRUE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}
		tempSfmSetAfterEditDoc = PngOnly;
		// See note above.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;
	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page DOC Sfm Set changed to PngOnly\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedRadioUseBothSetsDoc(wxCommandEvent& WXUNUSED(event))
{
	// Check for this session changes in the usfm filter page's Doc Filter inventory
	// If changes were made this session there before changing the sfm set here now,
	// warn the user that the change will activate the UsfmAndPng filter settings
	// in effect there in the Filtering page.
	wxString msg, newSet;
	newSet = GetSetNameStr(UsfmAndPng); // BEW 14Dec10, use this rather than a literal
	bDocSfmSetChanged = TRUE;
	if (pStartWorkingWizard == NULL)
	{
		// We are in Preferences

		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(TRUE);
			}
			else if (currentSet == PngOnly)
			{
				pRadioPngOnly->SetValue(TRUE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}

        // BEW added 11Oct10, to warn about the dangers when choosing the combined marker
        // sets. Changing to the combined sets is always allowed. When the test for Usfm
        // being the current markup scheme in a document is indeterminate (i.e.
        // bItsIndeterminate is TRUE), the user should see no warning because in the latter
        // scenario we know for sure that there is no \fe marker in the data and this is
        // the only marker incompatible between sets (because it is "footnote end" in the
        // PngOnly set, but "endnote beginning" in UsfmOnly set, and so getting it wrong
        // when present will definitely cause a malformed document parse - the app assumes
        // its USFM endnote begin-marker when both sets are chosen combined). If
        // bItsIndeterminate is FALSE, then it is one of the PngOnly or UsfmOnly sets in
        // the document markup, and so changing to the combined sets is hazardous (because
        // there may be a \fe in the data) -- in that case we must warn the user of the
        // hazard. When no document is open there is no way to know anything about \fe or
        // indeterminate-ness, so again we should warn the user of the potential for a
        // malformed document, for same reason
        bool bGiveAWarning = FALSE;
		SfmSet currentSet = gpApp->gCurrentSfmSet;
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// the document is not yet defined - allow the change but give a warning
			// (this block can't be entered, as Bill has made the radio buttons for the
			// document's set be disabled when no doc is open; but we'll keep the code for
			// this block for safety's sake)
			bGiveAWarning = TRUE;
		}
		bool bItsIndeterminate = TRUE; // use TRUE as the default, because if it stays
									   // TRUE then no message needs to be shown & the
									   // set change can just go ahead
		if (!gpApp->m_pSourcePhrases->IsEmpty())
		{
			// there is a document defined
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
			bItsUSFM = bItsUSFM; // added to avoid a compiler warning
			// we care here only about the function's bItsIndeterminate return value
		}		
		if ((!bItsIndeterminate || bGiveAWarning) && !bWarningShownAlready)
		{
            // do the warning (which allows cancellation) only when the current document's
            // set is determinate (ie. unique markers from PngOnly set, or unique markers
            // from USFM set), or when there is no document open
			// Note: we have to put a test here also for the warning message already
			// having been shown because the project setting was similarly changed and the
			// message shown at that time - we need an additional boolean flag in the
			// UsfmFilterPage.h class definition to handle this
			wxString msg2;
			if (bGiveAWarning)
			{
				// no document is open -- a different message is required
				msg2 = msg2.Format(_(
"A document is not open.\nChanging to the %s is allowed.\nHowever, it is risky. If it contains an \\fe marker, this marker is interpretted differently in each marker set. In the USFM marker set it indicates an endnote follows. In the PNG 1998 marker set it indicates a footnote precedes.\nAdapt It will assume it marks an endnote; but if this assumption is wrong, a slightly malformed (but adaptable) document will result."),
				newSet.c_str());
			}
			else
			{
				// a document is open, and it's determinate that it is PngOnly or UsfmOnly
				msg2 = msg2.Format(_(
"The open document contains markers unique to either the USFM marker set, or unique to the PNG 1998 marker set.\nChanging to the %s is allowed.\nHowever, it is risky. If it contains an \\fe marker, this marker is interpretted differently in each marker set. In the USFM marker set it indicates an endnote follows. In the PNG 1998 marker set it indicates a footnote precedes.\nAdapt It will assume it marks an endnote; but if this assumption is wrong, a slightly malformed (but adaptable) document will result."),
				newSet.c_str());
			}
			int nResult = wxMessageBox(msg2, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
			{
				if (currentSet == UsfmOnly)
				{
					pRadioPngOnly->SetValue(FALSE);
					pRadioUsfmAndPng->SetValue(FALSE);
					pRadioUsfmOnly->SetValue(TRUE);
				}
				else if (currentSet == PngOnly)
				{
					pRadioPngOnly->SetValue(TRUE);
					pRadioUsfmAndPng->SetValue(FALSE);
					pRadioUsfmOnly->SetValue(FALSE);
				}
				bDocSfmSetChanged = FALSE;
				bWarningShownAlready = FALSE; // restore default value
				tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
						// to the value it had before the button click
				return;
			}
            // if control gets to here, we've got to let the change of set happen, but when
            // the potential for \fe interpretation conflict is real, we've at least warned
            // the user & if a mess results because he went ahead and shouldn't have, too
            // bad
		}
		bWarningShownAlready = FALSE; // restore default value

		tempSfmSetAfterEditDoc = UsfmAndPng;
		// Call the usfm filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set.
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
		// only need to fill the arrays here, not display them. All of the arrays will be
		// recalculated/filled again if the user clicks on the Filtering tab.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

#ifdef _Trace_UnknownMarkers
		TRACE0("In USFM and Filtering Page User clicked UsfmAndPng - Now calling Usfm Filter Page's AddUnknownMarkersToDocArrays:\n");
#endif

		// reinventory and add any unknown markers to the Doc's filter arrays
		AddUnknownMarkersToDocArrays();

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnly->SetValue(FALSE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(TRUE);
			}
			else if (currentSet == PngOnly)
			{
				pRadioPngOnly->SetValue(TRUE);
				pRadioUsfmAndPng->SetValue(FALSE);
				pRadioUsfmOnly->SetValue(FALSE);
			}
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditDoc = tempSfmSetBeforeEditDoc; // restore this also
									// to the value it had before the button click
			return;
		}

		tempSfmSetAfterEditDoc = UsfmAndPng;
		// See note above.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditDoc),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsDoc,
			&m_filterFlagsDoc,
			&m_userCanSetFilterFlagsDoc);

		// From the wizard no doc is loaded so there are no unknown markers to add to the Doc's 
		// filter arrays, so we don't call pFilterPageWiz->AddUnknownMarkersToDocArrays() here.

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsDoc, 2,
				&m_userCanSetFilterFlagsDoc);

		// At this point the m_filterFlagsDocBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditDoc
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsDocBeforeEdit can be copied from the pFilterFlagsDoc array.
		// The tempFilterMarkersBeforeEditDoc is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsDocBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsDoc->GetCount(); ct++)
			pFilterFlagsDocBeforeEdit->Add(pFilterFlagsDoc->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditDoc 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsDoc,
																&m_filterFlagsDoc);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditDoc = tempFilterMarkersBeforeEditDoc;
	}

	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page DOC Sfm Set changed to UsfmAndPng\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	wxString newSet = GetSetNameStr(UsfmOnly); // BEW 14Dec10, use this rather than a literal
	tempSfmSetAfterEditProj = UsfmOnly;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We're in Preferences
		
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == PngOnly)
			{
				pRadioPngOnlyProj->SetValue(TRUE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(TRUE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

        // BEW added 11Oct10, to prevent SFM set change to UsfmOnly if the document is
        // marked up for the PNG 1998 SFM set; allow it if no doc is open, but warn user of
        // the need for SFM set and the project's documents internal markup to match
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// a document is not open - we must allow any change at this point, but
			// warn the user of the danger of a bad choice & allow Cancel
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			wxString msg2;
			msg2 = msg2.Format(_(
"There is no currently open document.\nTherefore changing to the %s is allowed.\nHowever, only do so if you are confident that the input files for creating documents for this project are marked up with the marker set you are now choosing.\n Cancel if you are unsure."),
			newSet.c_str());
			int nResult = wxMessageBox(msg2, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
			{
				if (currentSet == PngOnly)
				{
					pRadioPngOnlyProj->SetValue(TRUE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(TRUE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
				bProjectSfmSetChanged = FALSE;
				bDocSfmSetChanged = FALSE;
                // clicking the button will have put UsfmOnly into tempSfmSetAfterEditProj
                // and this will be wrongly assigned to gProjectSfmSetForConf in the OnOK()
                // function of the Prefs page (if the user clicks OK button rather than
                // Cancel there), which would result in the cancel done here not having any
                // effect, as UsfmOnly would then still be the new set value - so we must
                // restore the value that tempSfmSetAfterEditProj had before the click,
                // which is the value still in tempSfmSetBeforeEditProj
				tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; 
				return;
			}
			// if control has not returned, the set change will be made
		}
		else
		{
			// there is a document defined
			bool bItsIndeterminate = FALSE;
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
            // If the document's markup scheme is not USFM (it's most likely then PNG 1998
            // SFM set), and it's' not the case that there are no markers diagnostic of one
            // of the sets (which would make it okay to effect the set change), then
            // changing to Ubs's USfM set should be prevented
			if (!bItsUSFM && !bItsIndeterminate)
			{
				SfmSet currentSet = gpApp->gCurrentSfmSet;
				bDocSfmSetChanged = FALSE;
				bProjectSfmSetChanged = FALSE;
				wxString docSet = GetSetNameStr(PngOnly); // BEW 14Dec10, use this rather than a literal
				wxString msg2;
				msg2 = msg2.Format(_(
"The currently open document is marked up with markers from the %s.\nChanging to the %s is unlikely to be helpful.\n(This may result in a badly formed document, and similarly for other documents in this project which are marked up the same way.)"),
				docSet.c_str(), newSet.c_str());
				wxMessageBox(msg2, _T(""), wxICON_WARNING);
				// restore the radio button to what it was before the click
				if (currentSet == PngOnly)
				{
					pRadioPngOnlyProj->SetValue(TRUE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(TRUE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
                // clicking the button will have put UsfmOnly into tempSfmSetAfterEditProj
                // and this will be wrongly assigned to gProjectSfmSetForConf in the OnOK()
                // function of the Prefs page (if the user clicks OK button rather than
                // Cancel there), which would result in the cancel done here not having any
                // effect, as UsfmOnly would then still be the new set value - so we must
                // restore the value that tempSfmSetAfterEditProj had before the click,
                // which is the value still in tempSfmSetBeforeEditProj
				tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; 
				return;
			}
            // if the document markup is USFM, then changing to UsfmOnly or UsfmAndPng is
            // okay, that's also true if the result of the test was indeterminate (it's
            // indeterminate when the the markers in the doc, such as they are, are in both
            // marker sets and there are no footnotes endnotes or cross references -- in
            // that case, it wouldn't matter what marker set is used because the formatting
            // would be correct), so let processing continue
		}

		// Call the usfm filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (UsfmOnly).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
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

		// Set up the filter arrays based on the usfm filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(12); // location 12
#endif

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(13); // location 13
#endif

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays.
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;

		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadProjSFMListBox(LoadInitialDefaults);
		
		// A change in the Project's sfm set to UsfmOnly also sets the Doc's sfm set to UsfmOnly.
		// We can simulate a selection of UsfmOnly for the Doc by calling OnBnClickedRadioUseUbsSetOnlyDoc.
		DoBnClickedRadioUseUbsSetOnlyDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == PngOnly)
			{
				pRadioPngOnlyProj->SetValue(TRUE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(TRUE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting. It is
		// "forced" because the Doc's buttons are disabled and are thus not user changeable.
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmFilterMarkersStr, // use the standard list of Usfm basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(14); // location 14
#endif

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(15); // location 15
#endif

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to UsfmOnly also sets the Doc's sfm set to UsfmOnly.
		// We can simulate a selection of UsfmOnly for the Doc by calling OnBnClickedRadioUseUbsSetOnlyDoc.
		DoBnClickedRadioUseUbsSetOnlyDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	UpdateButtons();

#ifdef __WXDEBUG__
	gpApp->ShowFilterMarkers(16); // location 16
#endif

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page PROJ Sfm Set changed to UsfmOnly\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	wxString newSet = GetSetNameStr(PngOnly); // BEW 14Dec10, use this rather than a literal
	tempSfmSetAfterEditProj = PngOnly;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We're in Preferences
		
		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(TRUE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(TRUE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

		// BEW added 11Oct10, to prevent SFM set change to PngOnly if the document is
		// marked up for USFM; allow it if no doc is open, but warn user of the need for
		// SFM set and the project's documents internal markup to match
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// a document is not open - we must allow any change at this point, but
			// warn the user of the danger of a bad choice & allow Cancel
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			wxString msg2;
			msg2 = msg2.Format(_(
"There is no currently open document.\nTherefore changing to the %s is allowed.\nHowever, only do so if you are confident that the input files for creating documents for this project are marked up with the marker set you are now choosing.\n Cancel if you are unsure."),
			newSet.c_str());
			int nResult = wxMessageBox(msg2, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
			{
				if (currentSet == UsfmOnly)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(TRUE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(TRUE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
				bProjectSfmSetChanged = FALSE;
				bDocSfmSetChanged = FALSE;
                // clicking the button will have put PngOnly into tempSfmSetAfterEditProj
                // and this will be wrongly assigned to gProjectSfmSetForConf in the OnOK()
                // function of the Prefs page (if the user clicks OK button rather than
                // Cancel there), which would result in the cancel done here not having any
                // effect, as PngOnly would then still be the new set value - so we must
                // restore the value that tempSfmSetAfterEditProj had before the click,
                // which is the value still in tempSfmSetBeforeEditProj
				tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; 
				return;
			}
			// if control has not returned, the set change will be made
		}
		else
		{
			// there is a document defined
			bool bItsIndeterminate = FALSE;
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
			if (bItsUSFM && !bItsIndeterminate)
			{
				SfmSet currentSet = gpApp->gCurrentSfmSet;
				bDocSfmSetChanged = FALSE;
				bProjectSfmSetChanged = FALSE;
				wxString msg2;
				msg2 = msg2.Format(_(
"The currently open document is marked up as USFM.\nChanging to the %s is unlikely to be helpful.\n(This may result in a badly formed document, and similarly for other documents in this project which are marked up the same way.)"),
				newSet.c_str());
				wxMessageBox(msg2, _T(""), wxICON_WARNING);
				// restore the radio button to what it was before the click
				if (currentSet == UsfmOnly)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(TRUE);
				}
				else if (currentSet == UsfmAndPng)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(TRUE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
                // clicking the button will have put PngOnly into tempSfmSetAfterEditProj
                // and this will be wrongly assigned to gProjectSfmSetForConf in the OnOK()
                // function of the Prefs page (if the user clicks OK button rather than
                // Cancel there), which would result in the cancel done here not having any
                // effect, as PngOnly would then still be the new set value - so we must
                // restore the value that tempSfmSetAfterEditProj had before the click,
                // which is the value still in tempSfmSetBeforeEditProj
				tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; 
				return;
			}
            // if it's not USFM, then changing to PngOnly or UsfmAndPng is okay, that's
            // also true when the result was indeterminate (FALSE is returned for bItsUSFM
            // in that case too), so let processing continue
		}

		// Call the usfm filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (PngOnly).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
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

		// Set up the filter arrays based on the usfm filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays.
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadProjSFMListBox(LoadInitialDefaults);

		// A change in the Project's sfm set to PngOnly also sets the Doc's sfm set to PngOnly.
		// We can simulate a selection of PngOnly for the Doc by calling OnBnClickedRadioUseSilpngSetOnlyDoc.
		DoBnClickedRadioUseSilpngSetOnlyDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(TRUE);
			}
			else if (currentSet == UsfmAndPng)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(TRUE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->PngFilterMarkersStr, // use the standard list of Png basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to PngOnly also sets the Doc's sfm set to PngOnly.
		// We can simulate a selection of PngOnly for the Doc by calling OnBnClickedRadioUseSilpngSetOnlyDoc.
		DoBnClickedRadioUseSilpngSetOnlyDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page PROJ Sfm Set changed to PngOnly\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	wxString newSet = GetSetNameStr(UsfmAndPng); // BEW 14Dec10, use this rather than a literal
	tempSfmSetAfterEditProj = UsfmAndPng;
	bProjectSfmSetChanged = TRUE;
	bDocSfmSetChanged = TRUE; // the doc sfm set changes when the project sfm set changes
	if (pStartWorkingWizard == NULL)
	{
		// We are in Preferences

		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(TRUE);
			}
			else if (currentSet == PngOnly)
			{
				pRadioPngOnlyProj->SetValue(TRUE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

        // BEW added 11Oct10, to warn about the dangers when choosing the combined marker
        // sets. Changing to the combined sets is always allowed. When the test for Usfm
        // being the current markup scheme in a document is indeterminate (i.e.
        // bItsIndeterminate is TRUE), the user should see no warning because in the latter
        // scenario we know for sure that there is no \fe marker in the data and this is
        // the only marker incompatible between sets (because it is "footnote end" in the
        // PngOnly set, but "endnote beginning" in UsfmOnly set, and so getting it wrong
        // when present will definitely cause a malformed document parse - the app assumes
        // its USFM endnote begin-marker when both sets are chosen combined). If
        // bItsIndeterminate is FALSE, then it is one of the PngOnly or UsfmOnly sets in
        // the document markup, and so changing to the combined sets is hazardous (because
        // there may be a \fe in the data) -- in that case we must warn the user of the
        // hazard. When no document is open there is no way to know anything about \fe or
        // indeterminate-ness, so again we should warn the user of the potential for a
        // malformed document, for same reason
        bool bGiveAWarning = FALSE;
		SfmSet currentSet = gpApp->gCurrentSfmSet;
		if (gpApp->m_pSourcePhrases->IsEmpty())
		{
			// the document is not yet defined - allow the change but give a warning
			// (this block can't be entered, as Bill has made the radio buttons for the
			// document's set be disabled when no doc is open; but we'll keep the code for
			// this block for safety's sake)
			bGiveAWarning = TRUE;
		}
		bool bItsIndeterminate = TRUE; // use TRUE as the default, because if it stays
									   // TRUE then no message needs to be shown & the
									   // set change can just go ahead
		if (!gpApp->m_pSourcePhrases->IsEmpty())
		{
			// there is a document defined
			bool bItsUSFM = IsUsfmDocument(gpApp->m_pSourcePhrases, &bItsIndeterminate);
			bItsUSFM = bItsUSFM; // added to avoid a compiler warning
			// we care here only about the function's bItsIndeterminate return value
		}		
		if (!bItsIndeterminate || bGiveAWarning)
		{
            // do the warning (which allows cancellation) only when the current document's
            // set is determinate (ie. unique markers from PngOnly set, or unique markers
            // from USFM set), or when there is no document open
			wxString msg2;
			if (bGiveAWarning)
			{
				// no document is open -- a different message is required
				msg2 = msg2.Format(_(
"A document is not open.\nChanging to the %s is allowed.\nHowever, it is risky for this document and for any new ones created with the changed setting. If a document contains an \\fe marker, this marker is interpretted differently in each marker set. In the USFM marker set it indicates an endnote follows. In the PNG 1998 marker set it indicates a footnote precedes.\nAdapt It will assume it marks an endnote; but if this assumption is wrong, a slightly malformed (but adaptable) document will result."),
				newSet.c_str());
			}
			else
			{
				// a document is open, and it's determinate that it is PngOnly or UsfmOnly
				msg2 = msg2.Format(_(
"The open document contains markers unique to either the USFM marker set, or unique to the PNG 1998 marker set.\nChanging to the %s is allowed.\nHowever, it is risky for this document and for any new ones created with the changed setting. If a document contains an \\fe marker, this marker is interpretted differently in each marker set. In the USFM marker set it indicates an endnote follows. In the PNG 1998 marker set it indicates a footnote precedes.\nAdapt It will assume it marks an endnote; but if this assumption is wrong, a slightly malformed (but adaptable) document will result."),
				newSet.c_str());
			}
			int nResult = wxMessageBox(msg2, _T(""), wxOK | wxCANCEL);
			if (nResult == wxCANCEL)
			{
				if (currentSet == UsfmOnly)
				{
					pRadioPngOnlyProj->SetValue(FALSE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(TRUE);
				}
				else if (currentSet == PngOnly)
				{
					pRadioPngOnlyProj->SetValue(TRUE);
					pRadioUsfmAndPngProj->SetValue(FALSE);
					pRadioUsfmOnlyProj->SetValue(FALSE);
				}
				bProjectSfmSetChanged = FALSE;
				bDocSfmSetChanged = FALSE;
                // clicking the button will have put UsfmAndPng into tempSfmSetAfterEditProj
                // and this will be wrongly assigned to gProjectSfmSetForConf in the OnOK()
                // function of the Prefs page (if the user clicks OK button rather than
                // Cancel there), which would result in the cancel done here not having any
                // effect, as UsfmAndPng would then still be the new set value - so we must
                // restore the value that tempSfmSetAfterEditProj had before the click,
                // which is the value still in tempSfmSetBeforeEditProj
				tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; 
				return;
			}
			
            // if control gets to here, we've got to let the change of set happen, but when
            // the potential for \fe interpretation conflict is real, we've at least warned
            // the user & if a mess results because he went ahead and shouldn't have, too
            // bad
            bWarningShownAlready = TRUE; // suppress warning in the change of setting for
									// the current doc (becaused user hasn't cancelled
									// doing the change here in the project level's handler)
		}

		// Call the usfm filter page's SetupFilterPageArrays to update the filter page's arrays
		// with the new sfm set (UsfmAndPng).
		// Note: The last NULL parameter is necessary because we cannot get a DC for the
		// list control because the usfm filter page may not exist yet. It doesn't matter, we
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

		// Set up the filter arrays based on the usfm filter page's tempFilterMarkersAfterEditProj
		// because we always want the arrays and list boxes to reflect what the user has
		// selected/filtered during a given session of Preferences. tempFilterMarkersAfterEditProj
		// always contains the currently selected filter markers that are displayed in the
		// project's list box.
		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;
		
		// whm added 20Sep06 because filterPage's InitDialog is not called when filtering tab is
		// selected in prefs
		LoadProjSFMListBox(LoadInitialDefaults);

		// A change in the Project's sfm set to UsfmAndPng also sets the Doc's sfm set to UsfmAndPng.
		// We can simulate a selection of UsfmAndPng for the Doc by calling OnBnClickedRadioUseBothSetsDoc.
		DoBnClickedRadioUseBothSetsDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	else
	{
		// We're using the wizard.
		// See notes above.

		if (bDocFilterMarkersChanged)
		{
			// BEW 5Jan2011. Changed to prevent the user from trying to change filter
            // settings and the SFM set at the one time. This was a potentially damaging;
            // it wasn't enough to warn the user, because he couldn't be expected to know
            // the implications of the proceed versus cancel choices. The OnOK() function
            // will attempt SFM set change first, then attempt filtering changes. What I'm
            // doing now is to provide some checking for attempts to do both jobs at the
            // one time - these checks will be at two places:
            // (1) in the DoBnClickedRadioUse...() functions as here (there are 6 of these)
            // - we check for bDocFilterMarkersChanged set TRUE and if so, we bail out of
            // the attempt to change the SFM set, and tell user to get the filter changes
            // done first, then come back into Preferences to get his wanted set change
			// done (it might be rejected though if the doc is found not to have markup in
			// the targetted SFM set -- see the code further below) 
            // (2) in OnOK() we check for the same flag being TRUE AND also one or both of
            // the flags bProjectSfmSetChanged and bDocSfmSetChanged being TRUE -- if that
            // is the case, we suppress (and restore pre-edit filter settings) the filtered
            // marker changes being attempted, tell the user about that, and just allow the
            // set change to get done. (Set change should have priority, because filtering
            // certain markers relies on those markers existing, which is not necessarily
            // the case if the SFM set is changed.)
			wxMessageBox(msgCannotFilterAndChangeSFMset, _T(""), wxICON_WARNING);
			// now restore the set radio buttons to what they were before the click, and
			// restore necessary pre-edit local variables, and leave the page ready for
			// the filter changes to be done if the user clicks its OK button
			SfmSet currentSet = gpApp->gCurrentSfmSet;
			if (currentSet == UsfmOnly)
			{
				pRadioPngOnlyProj->SetValue(FALSE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(TRUE);
			}
			else if (currentSet == PngOnly)
			{
				pRadioPngOnlyProj->SetValue(TRUE);
				pRadioUsfmAndPngProj->SetValue(FALSE);
				pRadioUsfmOnlyProj->SetValue(FALSE);
			}
			bProjectSfmSetChanged = FALSE;
			bDocSfmSetChanged = FALSE;
			tempSfmSetAfterEditProj = tempSfmSetBeforeEditProj; // restore this also
									// to the value it had before the button click
			return;
		}

		// In the wizard we force the Doc sfm set setting to follow the Proj sfm set setting
		tempSfmSetAfterEditDoc = tempSfmSetAfterEditProj;

		SetupFilterPageArrays(gpApp->GetCurSfmMap(tempSfmSetAfterEditProj),
			gpApp->UsfmAndPngFilterMarkersStr, // use the standard list of UsfmAndPng basic filter markers on the App
			useString,
			&m_SfmMarkerAndDescriptionsProj,
			&m_filterFlagsProj,
			&m_userCanSetFilterFlagsProj);

		gpApp->FormatMarkerAndDescriptionsStringArray(NULL, 
				&m_SfmMarkerAndDescriptionsProj, 2,
				&m_userCanSetFilterFlagsProj);

		// At this point the m_filterFlagsProjBeforeEdit CUIntArray and the tempFilterMarkersBeforeEditProj
		// are populated with data based on a different sfm set, so, in order to be able to use them to
		// test for "before edit/after edit" changes, we need to initialize them again. The 
		// pFilterFlagsProjBeforeEdit can be copied from the pFilterFlagsProj array.
		// The tempFilterMarkersBeforeEditProj is assigned the string of filter markers using 
		// GetFilterMkrStrFromFilterArrays, which gets a filter marker string composed from the current
		// arrays (which will include any unknown filter markers because of the AddUnknownMarkersToDocArrays
		// call above).
		pFilterFlagsProjBeforeEdit->Clear();
		int ct;
		for (ct = 0; ct < (int)pFilterFlagsProj->GetCount(); ct++)
			pFilterFlagsProjBeforeEdit->Add(pFilterFlagsProj->Item(ct));
		// Get a new filter marker string from the newly built arrays 
		tempFilterMarkersBeforeEditProj 
			= GetFilterMkrStrFromFilterArrays(&m_SfmMarkerAndDescriptionsProj,
																&m_filterFlagsProj);
		// Since we are starting a new baseline for filter changes, make the "after" edit = the "before" edit
		// established above.
		tempFilterMarkersAfterEditProj = tempFilterMarkersBeforeEditProj;
		
		// A change in the Project's sfm set to UsfmAndPng also sets the Doc's sfm set to UsfmAndPng.
		// We can simulate a selection of UsfmAndPng for the Doc by calling OnBnClickedRadioUseBothSetsDoc.
		DoBnClickedRadioUseBothSetsDoc(event); // this will set up the Doc's arrays in Usfm Filter Page.
	}
	UpdateButtons();

#ifdef _Trace_FilterMarkers
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	TRACE0("SET CHANGE: In USFM and Filtering page PROJ Sfm Set changed to UsfmAndPng\n");
#endif
}

void CUsfmFilterPageCommon::DoBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event))
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

void CUsfmFilterPageCommon::UpdateButtons()
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

	// update the change-fixed-space-to-regular-space check box
	pChangeFixedSpaceToRegular->SetValue(bChangeFixedSpaceToRegularSpace);
}



///////////////////// CUsfmFilterPageWiz /////////////////////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS( CUsfmFilterPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CUsfmFilterPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CUsfmFilterPageWiz::InitDialog)// not strictly necessary for dialogs based on wxDialog
	//EVT_SET_FOCUS(CUsfmFilterPageWiz::OnSetFocus) // doesn't get called in a wizard page
    EVT_WIZARD_PAGE_CHANGING(-1, CUsfmFilterPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CUsfmFilterPageWiz::OnWizardCancel)
	EVT_LISTBOX(IDC_LIST_SFMS, CUsfmFilterPageWiz::OnLbnSelchangeListSfmsDoc)
	// The next two should be handled from this event table
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_PROJ, CUsfmFilterPageWiz::OnCheckListBoxToggleProj)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CUsfmFilterPageWiz::OnLbnSelchangeListSfmsProj)
	
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY, CUsfmFilterPageWiz::OnBnClickedRadioUseUbsSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY, CUsfmFilterPageWiz::OnBnClickedRadioUseSilpngSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS, CUsfmFilterPageWiz::OnBnClickedRadioUseBothSetsDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_PROJ, CUsfmFilterPageWiz::OnBnClickedRadioUseUbsSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ, CUsfmFilterPageWiz::OnBnClickedRadioUseSilpngSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_PROJ, CUsfmFilterPageWiz::OnBnClickedRadioUseBothSetsProj)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM, CUsfmFilterPageWiz::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
END_EVENT_TABLE()

CUsfmFilterPageWiz::CUsfmFilterPageWiz()
{
}

CUsfmFilterPageWiz::CUsfmFilterPageWiz(wxWizard* parent) // dialog constructor
{
	Create( parent );

	usfm_filterPgCommon.DoSetDataAndPointers();

}

CUsfmFilterPageWiz::~CUsfmFilterPageWiz() // destructor
{
	
}

bool CUsfmFilterPageWiz::Create( wxWizard* parent)
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

void CUsfmFilterPageWiz::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	usfm_filterPgCommon.pUsfmFilterPageSizer = UsfmFilterPageFunc(this, TRUE, TRUE);
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(usfm_filterPgCommon.pFilterPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CUsfmFilterPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pCaseEquivPageWiz; 
}
wxWizardPage* CUsfmFilterPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pDocPage;
}

void CUsfmFilterPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}
	
void CUsfmFilterPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	usfm_filterPgCommon.DoInit();
}

void CUsfmFilterPageWiz::OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event))
{
	usfm_filterPgCommon.DoLbnSelchangeListSfmsDoc();
}
void CUsfmFilterPageWiz::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoCheckListBoxToggleDoc(event);
}

void CUsfmFilterPageWiz::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoCheckListBoxToggleProj(event);
}

void CUsfmFilterPageWiz::OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event))
{
	usfm_filterPgCommon.DoLbnSelchangeListSfmsProj();
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseBothSetsDoc(event);
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseUbsSetOnlyProj(event);
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CUsfmFilterPageWiz::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseBothSetsProj(event);
}

void CUsfmFilterPageWiz::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}

void CUsfmFilterPageWiz::OnWizardPageChanging(wxWizardEvent& event) 
{
	// Do any required changes to sfm and filter settings.
	// Note: We should not need to call RetokenizeText() from here because the 
	// filterPage is only accessible within the wizard after the user clicks on 
	// <New Project> from the projectPage, and no document will be open, since 
	// the docPage is the last page in the wizard, appearing after this page.
	// Note: Should the call to gpApp->DoUsfmFilterChanges() be done in the DocPage's
	// OnWizardFinish routine rather than here???

#ifdef _Trace_FilterMarkers
	TRACE0("In Usfm Filter page's OnWizardNext BEFORE DoUsfmFilterChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	usfm_filterPgCommon.bSFMsetChanged = FALSE; // supplied for param list, used by filterPage to see if sfm set was changed
	// Note: DoUsfmSetChanges below updates the global sfm set and the global filter marker strings
	// for any changes the user has made in document and/or project settings
	gpApp->DoUsfmSetChanges(&usfm_filterPgCommon, usfm_filterPgCommon.bSFMsetChanged, NoReparse); // NoReparse within wizard since no doc is open there
	
	gpApp->DoUsfmFilterChanges(&usfm_filterPgCommon, NoReparse); // NoReparse within wizard since no doc is open there

#ifdef _Trace_FilterMarkers
	TRACE0("In Usfm Filter page's OnWizardNext AFTER DoUsfmFilterChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	bool bMovingForward = event.GetDirection();
	if (bMovingForward)
	{
		// Movement through wizard pages is sequential - the next page is the docPage.
		// The pDocPage's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pDocPage->InitDialog(idevent);
	}
}

///////////////////////////////////// CUsfmFilterPagePrefs ///////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS( CUsfmFilterPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CUsfmFilterPagePrefs, wxPanel)
	EVT_INIT_DIALOG(CUsfmFilterPagePrefs::InitDialog)// not strictly necessary for dialogs based on wxDialog
	//EVT_SET_FOCUS(CUsfmFilterPagePrefs::OnSetFocus) // doesn't get called in a wizard page
	EVT_LISTBOX(IDC_LIST_SFMS, CUsfmFilterPagePrefs::OnLbnSelchangeListSfmsDoc)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CUsfmFilterPagePrefs::OnLbnSelchangeListSfmsProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY, CUsfmFilterPagePrefs::OnBnClickedRadioUseUbsSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY, CUsfmFilterPagePrefs::OnBnClickedRadioUseSilpngSetOnlyDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS, CUsfmFilterPagePrefs::OnBnClickedRadioUseBothSetsDoc)
	EVT_RADIOBUTTON(IDC_RADIO_USE_UBS_SET_ONLY_PROJ, CUsfmFilterPagePrefs::OnBnClickedRadioUseUbsSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ, CUsfmFilterPagePrefs::OnBnClickedRadioUseSilpngSetOnlyProj)
	EVT_RADIOBUTTON(IDC_RADIO_USE_BOTH_SETS_PROJ, CUsfmFilterPagePrefs::OnBnClickedRadioUseBothSetsProj)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM, CUsfmFilterPagePrefs::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
END_EVENT_TABLE()

CUsfmFilterPagePrefs::CUsfmFilterPagePrefs()
{
}

CUsfmFilterPagePrefs::CUsfmFilterPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );

	usfm_filterPgCommon.DoSetDataAndPointers();
}

CUsfmFilterPagePrefs::~CUsfmFilterPagePrefs() // destructor
{
	
}

bool CUsfmFilterPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CUsfmFilterPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	usfm_filterPgCommon.pUsfmFilterPageSizer = UsfmFilterPageFunc(this, TRUE, TRUE);
	usfm_filterPgCommon.pUsfmFilterPageSizer->Layout();
}
	
void CUsfmFilterPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	usfm_filterPgCommon.DoInit();
	gpApp->m_pLayout->m_bFilteringChanged = FALSE; // initialize
	gpApp->m_pLayout->m_bUSFMChanged = FALSE; // initialize
}

void CUsfmFilterPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// wx version notes: Any changes made to the logic in OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CUsfmFilterPagePrefs::OnWizardPageChanging() 
	// is called when page changes or Finish button is pressed. 
	//
	// CUsfmFilterPagePrefs::OnOK() is always called when the user dismisses the EditPreferencesDlg
	// dialog with the OK button. 
	//
	// We've modified the normal property sheet behavior which was that the page's OnOK() 
	// method would be called when the user clicks on OK in the property sheet, but only 
	// if the user accesses the filterPage via the tab (which also calls OnInitDialog)
	// before clicking the property sheet's OK button. Now CUsfmFilterPagePrefs::OnOK is always
	// called whenever EditPreferencesDlg OK button is pressed.

	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling of this CUsfmFilterPagePrefs::OnOK() from there.


	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc != NULL);

	// check for changes in usfmPage and effect them
	// BEW addition, 10Jun05

	// The SFM set may have been changed by the user in the USFMPage. DoUsfmSetChanges() will test
	// for this. What is done after it returns depends on whether or not the set has been changed.
	
#ifdef _Trace_FilterMarkers
	TRACE0("In Edit Preferences AFTER OK but BEFORE DoUsfmSetChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif
	
#ifdef _Trace_UnknownMarkers
	TRACE0("In OnEditPreferences BEFORE Doc rebuild BEFORE DoUsfmSetChanges call:\n");
	TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif	

	// whm the following DoUsfmSetChanges() call brought from the old usfmPage's OnOK(). It is placed
	// before the other old filterPage's code (including its calls to DoUsfmFilterChanges() since when
	// the usfmPage and filterPage were separate, the usfmPage's OnOK() executed first before the 
	// filterPage's OnOK().
	usfm_filterPgCommon.bSFMsetChanged = FALSE; // supplied for param list, used by filterPage to see if sfm set was changed
	// Note: DoUsfmSetChanges below updates the global sfm set and the global filter marker strings
	// for any changes the user has made in document and/or project settings. It also will set the 
	// usfm_filterPgCommon's bSFMsetChanged flag to true if there was a set change which will insure that
	// the filterPage's OnOK handler calls DoUsfmFilterChanges() with NoReparse parameter.
	gpApp->DoUsfmSetChanges(&usfm_filterPgCommon, usfm_filterPgCommon.bSFMsetChanged, DoReparse); // DoReparse for edit prefs

	// Suppose a set change was asked for... Even if he also altered the filtering settings
	// for the new SFM set, the dominant effect would be the altering of the SFM set - we would then
	// effect the filtering changes along the way as a byproduct of doing the document rebuild for the
	// SFM set change - because we implement the process as one pass to do unfiltering using the original
	// SMF set, and then a second pass (with USFMAnalysis struct updating) to do the filtering required
	// for rebuilding using the chosen SFM set - and this filtering would also include any filtering
	// changes which the user may have requested in addition to the defaults for the new SFM set.
	// Therefore, we would not need to invoke the doc rebuild within DoUsfmFilterChanges when the SFM set
	// has been changed, but we would need to call it so that GUI filtering apparatus would get updated.
	
	wxASSERT(pStartWorkingWizard == NULL);
	//wxASSERT(pUSFMPageInPrefs != NULL);
	// This filterPage::OnOK() handler should only be called from edit preferences
	// Since the usfmPage's OnOK handler is always called before this one, we don't need to call
	// DoUsfmSetChanges() from here. We can inspect bSFMsetChanged there to see if any sfm set change
	// occurred. If so, we call DoUsfmFilterChanges with the NoReparse parameter. For a case where there
	// only was a filtering change we call DoUsfmFilterChanges with the DoReparse parameter.
	// whm 5Oct10 modified to allow for pUSFMPageInPrefs being NULL in some profiles
	if (usfm_filterPgCommon.bSFMsetChanged)
	{

#ifdef _Trace_FilterMarkers
		TRACE0("In Edit Preferences AFTER DoUsfmSetChanges SET CHANGE = TRUE, BEFORE DoUsfmFilterChanges call:\n");
		TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
		TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
		TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
		TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif
	
#ifdef _Trace_UnknownMarkers
		TRACE0("In OnEditPreferences AFTER DoUsfmSetChanges SET CHANGE = TRUE, BEFORE DoUsfmFilterChanges call:\n");
		TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
		TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif

		// check for changes in filterPage and update the structs, strings, arrays and rapid access strings,
		// but do not do any document rebuild (here called a 'reparse' because at the time we were designing
		// this stuff we were thinking rebuilding would be done by reparsing the source, but subsequently we
		// worked out that that is error prone and we instead rebuild the doc from itself without looking
		// again at the source text) because the needed rebuild (including filtering changes) will have been
		// done by the preceding DoUsfmSetChanges() call.
		
		//gpApp->DoUsfmFilterChanges(&usfm_filterPgCommon, NoReparse); // calling it here
																	   //is deprecated
        // BEW 5Jan11, to prevent the dialog from being used to make a filtering change and
        // a set change at the one time. It's likely to result in a mess. (See comments in
        // the DoBnClickedRadioUse...OnlyDoc() functions or their ...Proj() equivalents
        // earlier in this file for why. Here we check for filtering changes wanted, warn the user
        // if the two can't be done together, and allow only the set change to happen
		if (usfm_filterPgCommon.bDocFilterMarkersChanged || usfm_filterPgCommon.bProjectFilterMarkersChanged)
		{
			wxString msgForIllegalJob = _(
"Trying to change the standard format marker (SFM) set at the same time as trying to change the marker filtering settings is illegal.\nThe filtering changes will be ignored. Only the SFM set change will be done now.\nTo get the filtering changes done, return to this page after the SFM set changes are completed and then set up the filtering changes a second time.");
			wxMessageBox(msgForIllegalJob, _T(""), wxICON_WARNING);

			// undo the filter changes here
			if (usfm_filterPgCommon.bDocSfmSetChanged)
			{
				usfm_filterPgCommon.tempFilterMarkersAfterEditDoc = 
					usfm_filterPgCommon.tempFilterMarkersBeforeEditDoc;
				gpApp->gCurrentFilterMarkers = usfm_filterPgCommon.tempFilterMarkersAfterEditDoc;
				gpApp->m_filterMarkersAfterEdit = usfm_filterPgCommon.tempFilterMarkersAfterEditDoc;
			}
			if (usfm_filterPgCommon.bProjectSfmSetChanged)
			{
				usfm_filterPgCommon.tempFilterMarkersAfterEditProj = 
					usfm_filterPgCommon.tempFilterMarkersBeforeEditProj;
				gpApp->gProjectFilterMarkersForConfig = usfm_filterPgCommon.tempFilterMarkersAfterEditProj;
			}
		}
	}
	else
	{

#ifdef _Trace_FilterMarkers
		TRACE0("In Edit Preferences AFTER DoUsfmSetChanges SET CHANGE = FALSE, BEFORE DoUsfmFilterChanges call:\n");
		TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
		TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
		TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
		TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

#ifdef _Trace_UnknownMarkers
		TRACE0("In OnEditPreferences AFTER DoUsfmSetChanges SET CHANGE = FALSE, BEFORE DoUsfmFilterChanges call:\n");
		TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
		TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif


		// there was no change made to the SFM set, so we only have to check for filtering changes, and
		// do the reparse if changes had been made
		gpApp->DoUsfmFilterChanges(&usfm_filterPgCommon, DoReparse);
	}

#ifdef _Trace_FilterMarkers
	TRACE0("In Edit Preferences AFTER DoUsfmSetChanges and/or DoUsfmFilterChanges BEFORE GetUnknownMarkersFromDoc call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

#ifdef _Trace_UnknownMarkers
	TRACE0("In OnEditPreferences AFTER DoUsfmSetChanges and/or DoUsfmFilterChanges BEFORE GetUnknownMarkersFromDoc call:\n");
	TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif

	// whm added 27Jun05. After any doc rebuild is finished, we need to insure that the 
	// unknown marker arrays and m_currentUnknownMarkerStr are up to date from what is 
	// now the situation in the Doc. Use preserveUnkMkrFilterStatusInDoc to cause
	// GetUnknownMarkersFromDoc to preserve the filter state of an unknown
	// marker in the Doc, i.e., set m_filterFlagsUnkMkrs to TRUE if the unknown marker
	// in the Doc was within \~FILTER ... \~FILTER* brackets, otherwise the flag is FALSE.
	pDoc->GetUnknownMarkersFromDoc(gpApp->gCurrentSfmSet, &gpApp->m_unknownMarkers, 
		&gpApp->m_filterFlagsUnkMkrs, gpApp->m_currentUnknownMarkersStr, 
		preserveUnkMkrFilterStatusInDoc);

#ifdef _Trace_UnknownMarkers
	TRACE0("In OnEditPreferences AFTER Doc rebuild and AFTER GetUnknownMarkersFromDoc call:\n");
	TRACE1("     Doc's unk mkrs fm arrays = %s\n", pDoc->GetUnknownMarkerStrFromArrays(&pDoc->m_unknownMarkers, &pDoc->m_filterFlagsUnkMkrs));
	TRACE1("   m_currentUnknownMarkersStr = %s\n", pDoc->m_currentUnknownMarkersStr);
#endif
}


void CUsfmFilterPagePrefs::OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event))
{
	usfm_filterPgCommon.DoLbnSelchangeListSfmsDoc();
}
void CUsfmFilterPagePrefs::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoCheckListBoxToggleDoc(event);
}

void CUsfmFilterPagePrefs::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoCheckListBoxToggleProj(event);
}

void CUsfmFilterPagePrefs::OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event))
{
	usfm_filterPgCommon.DoLbnSelchangeListSfmsProj();
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseUbsSetOnlyDoc(event);
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseSilpngSetOnlyDoc(event);
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseBothSetsDoc(event);
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseUbsSetOnlyProj(event);
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseSilpngSetOnlyProj(event);
}

void CUsfmFilterPagePrefs::OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedRadioUseBothSetsProj(event);
}

void CUsfmFilterPagePrefs::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event)
{
	usfm_filterPgCommon.DoBnClickedCheckChangeFixedSpacesToRegularSpaces(event);
}
