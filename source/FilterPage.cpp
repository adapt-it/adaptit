/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FilterPage.cpp
/// \author			Bill Martin
/// \date_created	18 April 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CFilterPageWiz and CFilterPagePrefs classes. 
/// It also defines a third class CFilterPageCommon which
/// contains the routines common to the above two classes.
/// The CFilterPageWiz class creates a wizard page for the StartWorkingWizard; the 
/// CFilterPagePrefs creates a identical panel as a tabbed page in the 
/// EditPreferencesDlg. Each form allows the user to change the filtering state 
/// of standard format markers used in the current document and/or whole project. 
/// The interface resources for the page/panel are defined in FilterPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// Note: The filter page and the usfm page, although separate tabs/pages in the 
/// preferences property sheet and the startup wizard, are closely related and the
/// setting changes made in one page/tab affects the other page/tab. The dependencies
/// that one page/tab has for the other page/tab, and the need to share their 
/// functionality in wizard pages as well as in property sheet preference tabs somewhat 
/// complicates the classes and their implementation. Another complicating factor is
/// the fact that in the wxWidgets' implementation of wxCheckListBox, client data is 
/// reserved for internal use, and cannot be used to associate the list item with its
/// underlying data representation.
/// \derivation		CFilterPageWiz is derived from wxWizardPage, CFilterPagePrefs from wxPanel, and CFilterPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in FilterPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FilterPage.h"
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
#include  "wx/checklst.h"

#include <wx/wizard.h>

#include "Adapt_It.h"
#include "FilterPage.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "MainFrm.h"
#include "USFMPage.h"
#include "DocPage.h"
#include "StartWorkingWizard.h"
#include "helpers.h"

#if !wxUSE_CHECKLISTBOX
    #error "This program can't be built without wxUSE_CHECKLISTBOX set to 1"
#endif // wxUSE_CHECKLISTBOX

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar;

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard; //extern CPropertySheet* pStartWorkingWizard;

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp.
extern CUSFMPageWiz* pUsfmPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage;

//extern CFilterPagePrefs* pFilterPageInPrefs; // set the App's pointer to the filterPage

//extern CFilterPageWiz* pFilterPageInWizard; // set the App's pointer to the filterPage

/// This global is defined in Adapt_It.cpp.
extern CUSFMPagePrefs* pUSFMPageInPrefs; // set the App's pointer to the filterPage

// This global is defined in Adapt_It.cpp.
//extern CUSFMPageWiz* pUSFMPageInWizard; // set the App's pointer to the filterPage

// CFilterPageCommon Class /////////////////////////////////////////////////////
void CFilterPageCommon::DoSetDataAndPointers()
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

	pSfmMarkerAndDescriptionsFactory = &m_SfmMarkerAndDescriptionsFactory;
	pFilterFlagsFactory = &m_filterFlagsFactory;
	pUserCanSetFilterFlagsFactory = &m_userCanSetFilterFlagsFactory;

	bOnInitDlgHasExecuted = FALSE;

#ifdef _Trace_UnknownMarkers
	TRACE0("In Filter Page's constructor copying Doc's unk mkr arrays to local filter page's arrays:\n");
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

	// Can't call InitDialog() from within CFilterPageWiz's constructor because InitDialog below
	// references pUSFMPageInWizard which may not get created until CUSFMPage's constructor
	// executes. Same is true for the CUSFMPage's constructor can't call its InitDialog within
	// its constructor. Otherwise we set up reciprocal dependencies that cannot be met.
	//wxInitDialogEvent event = wxEVT_INIT_DIALOG;
	//this->InitDialog(event);
}

void CFilterPageCommon::DoInit()
{
	// get pointers to our controls
	pListBoxSFMsDoc = (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS);
	wxASSERT(pListBoxSFMsDoc != NULL);
	
	pListBoxSFMsProj =  (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS_PROJ);
	wxASSERT(pListBoxSFMsProj != NULL);
	
	pListBoxSFMsFactory =  (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS_FACTORY);
	wxASSERT(pListBoxSFMsFactory != NULL);

	pTextCtrlAsStaticFilterPage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_FILTERPAGE);
	wxASSERT(pTextCtrlAsStaticFilterPage != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticFilterPage->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticFilterPage->SetBackgroundColour(gpApp->sysColorBtnFace);

	wxString normalText1, normalText2;
	// IDS_FILTER_INFO_1
	normalText1 = _("To filter out (hide) the text associated with a marker, check the box next to the marker. To adapt the text associated with a marker, un-check the box next to the marker."); //.Format();
	// IDS_FILTER_INFO_2
	normalText2 = _("When checked, the marker and associated text will be filtered out (hidden) from the adapted document."); //.Format();

	if (pStartWorkingWizard == NULL)
	{
		// We are in Edit Preferences with an existing document open
		if (pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditProj == pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditDoc)
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
				GetSetNameStr(pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditDoc).c_str(), 
				GetSetNameStr(pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditProj).c_str());
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
		pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditDoc = pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditProj;
		tempFilterMarkersAfterEditDoc = tempFilterMarkersAfterEditProj;

		// We also want to disable the Filtering page's Document related list and buttons, 
		// because we do not allow the user to change the document to use anything other than the project 
		// settings. This disabled state for the document remains in effect for the life of the wizard.
		pListBoxSFMsDoc->Enable(FALSE);
	}

	LoadFactorySFMListBox();
	LoadProjSFMListBox(LoadInitialDefaults);
	if (pStartWorkingWizard == NULL)
	{
		LoadDocSFMListBox(LoadInitialDefaults);
	}
	else
	{
		// In the wizard there is no doc open so load only a message into the pListBoxSFMsDoc
		// saying that "(NO DOCUMENTS CREATED YET IN THIS PROJECT)"
		pListBoxSFMsDoc->Clear();
		pListBoxSFMsDoc->Append(_("(NO DOCUMENT AVAILABLE IN THIS PROJECT - NO FILTER SETTINGS TO DISPLAY)"));
	}
	
	// initially have focus on the Doc list box
	if (pStartWorkingWizard == NULL)
	{
		// We are in Edit Preferences with an existing document open
		// The Doc's controls are disabled, so set focus on the Proj list box
		if (pListBoxSFMsProj->IsEnabled())
			pListBoxSFMsProj->SetFocus();
	}
	else
	{
		// We are in the start working wizard with no document open, set
		// focus on the Doc's list box
		if (pListBoxSFMsDoc->IsEnabled())
			pListBoxSFMsDoc->SetFocus();
	}

	bOnInitDlgHasExecuted = TRUE;
	bFirstWarningGiven = FALSE;

#ifdef _Trace_FilterMarkers
	TRACE0("In Filter page's OnInitDialog AFTER all List Boxes Loaded:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	pFilterPageSizer->Layout(); // only needed here to insure the layout of the help text in read-only edit is ok
}

void CFilterPageCommon::AddUnknownMarkersToDocArrays()
{
	// Add to the arrays any unknown markers listed for the Doc
	// NOTE 16Jun05: The USFM page may change the sfm set and call AddUnknownMarkersToDocArrays
	// from the USFM page. This requires we get from the document a new inventory of 
	// m_unknownMarkers and related arrays before we add them to the Doc's list box.

	wxString lbStr;
	// Get the currently selected sfm set in USFM page
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	enum SfmSet useSfmSet;
	if (pStartWorkingWizard == NULL)
	{
		useSfmSet = pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditDoc;
	}
	else
	{
		useSfmSet = pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditDoc;
	}

#ifdef _Trace_UnknownMarkers
	TRACE0("In Filter Page's AddUnknownMarkersToDocArrays BEFORE GetUnknownMarkersFromDoc call:\n");
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
	TRACE0("In Filter Page's AddUnknownMarkersToDocArrays AFTER GetUnknownMarkersFromDoc call:\n");
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

void CFilterPageCommon::SetupFilterPageArrays(MapSfmToUSFMAnalysisStruct* pMap,
		wxString filterMkrStr,
		enum UseStringOrMap useStrOrMap,
		wxArrayString* pSfmMarkerAndDescr,
		wxArrayInt* pFilterFlags,
		wxArrayInt* pUserCanSetFlags)
{
	// SetupFilterPageArrays is called once for each of the three sets of marker arrays: Doc, Proj, and Factory.
	// It initializes and populates the pSfmMarkerAndDescr, pFilterFlags and pUserCanSetFlags arrays with 
	// the data that LoadListBoxFromArrays() subsequently uses to fill the filter page's list boxes (doc, proj 
	// and factory). The arrays contain information on all the markers for the active temporary sfm set. 
	//
	// whm 25May07 modified the wx version removing the "Show All" button. We still set up the filter page
	// arrays the way they were done previously so that they contain information on all markers in the
	// given sfm set. However, when loading the markers in the list boxes (see LoadListBoxFromArrays below)
	// only markers that have pSfm->userCanSetFilter == TRUE are actually loaded into the list boxes for view
	// by the user. We need the arrays to contain all the markers so we can easily build the 
	// gProjectFilterMarkersForConfig string, etc.
	//
	// Now, in the wx version, individual elements of the filter flags array are changed by simply checking
	// or unchecking the list items' check boxes. When called from the USFM page the pListBox 
	// parameter is NULL as there is no DC available (which doesn't matter as the Filter page is not
	// visible when the USFM page is displaying).
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
		// the filter page's AddUnknownMarkersToDocArrays()
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

void CFilterPageCommon::LoadListBoxFromArrays(wxArrayString* pSfmMarkerAndDescr, 
		wxArrayInt* pFilterFlags, wxArrayInt* pUserCanSetFlags, wxCheckListBox* pListBox)
{
	pListBox->Clear(); // clears all items out of list
	// Populate the given input list box (doc, proj or factory). Uses the data stored in the 
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


void CFilterPageCommon::LoadDocSFMListBox(enum ListBoxProcess lbProcess)
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

	// Insure that the arrays are empty before loading them.
	pSfmMarkerAndDescriptionsDoc->Clear();
	pFilterFlagsDoc->Clear();
	pFilterFlagsDocBeforeEdit->Clear();
	pUserCanSetFilterFlagsDoc->Clear();

	MapSfmToUSFMAnalysisStruct* pMap;
	wxString filterMkrStr;
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences 
		// Using USFM page's current sfm set get the appropriate USFMAnalysis struct map
		pMap = gpApp->GetCurSfmMap(pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditDoc);
	}
	else
	{
		// We're in the Wizard
		// using USFM page's current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditDoc);
	}

	if (lbProcess == LoadInitialDefaults) // whm added 28Sep06 to make Undo work properly
	{
		//tempFilterMarkersAfterEditDoc = gpApp->gCurrentFilterMarkers;
		// No, the above doesn't work when the sfmSet has changed. tempFilterMarkersAfterEditDoc
		// needs to be assigned the filter markers for the currently chosen (temp) sfm set for the 
		// Doc, which is indicated by tempSfmSetAfterEditDoc in the CUSFMPage. 
		// This correction could also be propagated back to the MFC version.
		if (pStartWorkingWizard == NULL)
		{
			// We're in Edit Preferences
			// so use pUSFMPageInPrefs
			if (pUSFMPageInPrefs != NULL && pUSFMPageInPrefs->usfmPgCommon.bDocSfmSetChanged)
			{
				switch (pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditDoc)
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
		}
		else
		{
			// We're in the Wizard
			// so use pUsfmPageWiz
			if (pUsfmPageWiz != NULL && pUSFMPageInPrefs->usfmPgCommon.bDocSfmSetChanged)
			{
				switch (pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditDoc)
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

	// If the filter page is called from the wizard no doc is open so we cannot
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

void CFilterPageCommon::LoadProjSFMListBox(enum ListBoxProcess lbProcess)
{
	// Loads into the project list box the markers for the gProjectSfmSetForConfig, setting the
	// filtering status of each marker so that only those that are listed
	// in the gProjectFilterMarkersForConfig string are checked for filtering.

	// Insure that the arrays are empty before loading them.
	pSfmMarkerAndDescriptionsProj->Clear();
	pFilterFlagsProj->Clear();
	pFilterFlagsProjBeforeEdit->Clear();
	pUserCanSetFilterFlagsProj->Clear();

	MapSfmToUSFMAnalysisStruct* pMap;
	wxString filterMkrStr;
	if (pStartWorkingWizard == NULL)
	{
		// We're in Edit Preferences
		// using USFM page's current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditProj);
	}
	else
	{
		// We're in the Wizard
		// using USFM page's current sfm set get the appropriate USFMAnalysis struct map 
		pMap = gpApp->GetCurSfmMap(pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditProj);
	}

	if (lbProcess == LoadInitialDefaults) // whm added 28Sep06 to enable Undo to work correctly
	{
		//tempFilterMarkersAfterEditProj = gpApp->gProjectFilterMarkersForConfig;
		// No, the above doesn't work when the sfmSet has changed. tempFilterMarkersAfterEditProj
		// needs to be assigned the filter markers for the currently chosen (temp) sfm set for the 
		// project, which is indicated by tempSfmSetAfterEditProj in the CUSFMPage. 
		// This correction could also be propagated back to the MFC version.
		if (pStartWorkingWizard == NULL)
		{
			// We're in Edit Preferences
			// so use pUSFMPageInPrefs
			if (pUSFMPageInPrefs != NULL && pUSFMPageInPrefs->usfmPgCommon.bProjectSfmSetChanged)
			{
				switch (pUSFMPageInPrefs->usfmPgCommon.tempSfmSetAfterEditProj)
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
		}
		else
		{
			// We're in the Wizard
			// so use pUsfmPageWiz
			if (pUsfmPageWiz != NULL && pUsfmPageWiz->usfmPgCommon.bProjectSfmSetChanged)
			{
				switch (pUsfmPageWiz->usfmPgCommon.tempSfmSetAfterEditProj)
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
		// If the Filter page forced a change of sfm set because of a copy button operation, i.e., 
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

void CFilterPageCommon::LoadFactorySFMListBox()
{
	// Loads into the Factory list box the markers for the gFactoryFilterMarkersStr, 
	// setting the filtering status of each marker so that only those that are listed
	// in the gFactoryFilterMarkersStr string are checked for filtering. The factory
	// filtering defaults were set in the App's InitInstance method. The Factory default
	// for sfm set is stored in gFactorySfmSet and was also set to UsfmOnly in 
	// InitInstance.

	// Insure that the arrays are empty before loading them.
	pSfmMarkerAndDescriptionsFactory->Clear();
	pFilterFlagsFactory->Clear();
	// no filter flags before edit for factory list
	pUserCanSetFilterFlagsFactory->Clear();

	MapSfmToUSFMAnalysisStruct* pMap;
	// Always use the Usfm map for the unchanging factory defaults
	pMap = gpApp->m_pUsfmStylesMap;

	SetupFilterPageArrays(pMap, gpApp->gFactoryFilterMarkersStr, useString, pSfmMarkerAndDescriptionsFactory, 
		pFilterFlagsFactory, pUserCanSetFilterFlagsFactory);

	// do not call AddUnknownMarkersToDocArrays here, only for the Doc's list

	wxClientDC aDC((wxWindow*)gpApp->GetMainFrame()->canvas);
	gpApp->FormatMarkerAndDescriptionsStringArray(&aDC, 
			pSfmMarkerAndDescriptionsFactory, 2, pUserCanSetFilterFlagsFactory);

	LoadListBoxFromArrays(pSfmMarkerAndDescriptionsFactory, 
		pFilterFlagsFactory, pUserCanSetFilterFlagsFactory, pListBoxSFMsFactory);

	// No "Filter Out" or "Include for Adapting" buttons in factory list

}

wxString CFilterPageCommon::GetSetNameStr(enum SfmSet set)
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

wxString CFilterPageCommon::GetFilterMkrStrFromFilterArrays(wxArrayString* pSfmMarkerAndDescr, wxArrayInt* pFilterFlags)
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
			wholeMkr = pSfmMarkerAndDescr->Item(ct);
			int spPos = wholeMkr.Find(_T("  "));
			wxASSERT(spPos != -1);
			wholeMkr = wholeMkr.Mid(0,spPos);
			wholeMkr.Trim(TRUE); // trim right end
			wholeMkr.Trim(FALSE); // trim left end
			// ensure marker ends with a space
			wholeMkr += _T(' ');
			tempStr += wholeMkr; // add marker to string
		}
	}
	return tempStr;
}


void CFilterPageCommon::AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker)
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


void CFilterPageCommon::RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker)
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

void CFilterPageCommon::AdjustFilterStateOfUnknownMarkerStr(wxString& unknownMkrStr, wxString wholeMarker, enum UnkMkrFilterSetting filterSetting)
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

void CFilterPageCommon::DoLbnSelchangeListSfmsDoc()
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

void CFilterPageCommon::DoCheckListBoxToggleDoc(wxCommandEvent& event)
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

void CFilterPageCommon::DoCheckListBoxToggleProj(wxCommandEvent& event)
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

void CFilterPageCommon::DoCheckListBoxToggleFactory(wxCommandEvent& event)
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
	wxString itemString = pListBoxSFMsFactory->GetString(nItem);
	pListBoxSFMsFactory->SetSelection(nItem); // insure that the selection highlights the line having the clicked box
	wxASSERT(nItem != -1);
	wxASSERT(nItem < (int)pFilterFlagsFactory->GetCount());
	// whm: wx version cannot use the wxCheckListBox's ClientData to map the sorted index of
	// the list box with the unsorted array index of the stored data, so I wrote
	// the following FindArrayString() function to find the string's element index
	// in the unsorted array. FindArrayString finds only exact string matches, incl case.
	int actualArrayIndex = gpApp->FindArrayString(itemString,pSfmMarkerAndDescriptionsFactory); //pListBoxSFMsDoc->FindString(selectedString); // get index into m_filterFlagsDoc
	wxASSERT(actualArrayIndex < (int)pFilterFlagsFactory->GetCount());
	// insure that the default factory setting of the selected item remains
	bool bDefaultIsChecked = (pFilterFlagsFactory->Item(actualArrayIndex) == 1);
	if (pListBoxSFMsFactory->IsChecked(nItem) != bDefaultIsChecked)
	{
		pListBoxSFMsFactory->Check(nItem,pFilterFlagsFactory->Item(actualArrayIndex) == 1);
		::wxBell();
	}
}

void CFilterPageCommon::DoBoxClickedIncludeOrFilterOutDoc(int lbItemIndex)
{
	// Note: whm 23May07 simplified the filter page by removing the "Filter Out"/
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

void CFilterPageCommon::DoLbnSelchangeListSfmsProj()
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

void CFilterPageCommon::DoBoxClickedIncludeOrFilterOutProj(int lbItemIndex)
{
	// Note: whm 23May07 simplified the filter page by removing the "Filter Out"/
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


// /////////////////// CFilterPageWiz /////////////////////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS( CFilterPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CFilterPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CFilterPageWiz::InitDialog)// not strictly necessary for dialogs based on wxDialog
	//EVT_SET_FOCUS(CFilterPageWiz::OnSetFocus) // doesn't get called in a wizard page
    EVT_WIZARD_PAGE_CHANGING(-1, CFilterPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CFilterPageWiz::OnWizardCancel)
	EVT_LISTBOX(IDC_LIST_SFMS, CFilterPageWiz::OnLbnSelchangeListSfmsDoc)
	// The next two should be handled from this event table
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_PROJ, CFilterPageWiz::OnCheckListBoxToggleProj)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS_FACTORY, CFilterPageWiz::OnCheckListBoxToggleFactory)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CFilterPageWiz::OnLbnSelchangeListSfmsProj)
	EVT_LISTBOX(IDC_LIST_SFMS_FACTORY, CFilterPageWiz::OnLbnSelchangeListSfmsFactory)
END_EVENT_TABLE()

CFilterPageWiz::CFilterPageWiz()
{
}

CFilterPageWiz::CFilterPageWiz(wxWizard* parent) // dialog constructor
{
	Create( parent );

	filterPgCommon.DoSetDataAndPointers();

}

CFilterPageWiz::~CFilterPageWiz() // destructor
{
	
}

bool CFilterPageWiz::Create( wxWizard* parent)
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

void CFilterPageWiz::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	filterPgCommon.pFilterPageSizer = FilterPageFunc(this, TRUE, TRUE);
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(filterPgCommon.pFilterPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CFilterPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pUsfmPageWiz; 
}
wxWizardPage* CFilterPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pDocPage;
}

void CFilterPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}
	
void CFilterPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	filterPgCommon.DoInit();
}

void CFilterPageWiz::OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event))
{
	filterPgCommon.DoLbnSelchangeListSfmsDoc();
}
void CFilterPageWiz::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleDoc(event);
}

void CFilterPageWiz::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleProj(event);
}

void CFilterPageWiz::OnCheckListBoxToggleFactory(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleFactory(event);
}

void CFilterPageWiz::OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event))
{
	filterPgCommon.DoLbnSelchangeListSfmsProj();
}

void CFilterPageWiz::OnLbnSelchangeListSfmsFactory(wxCommandEvent& WXUNUSED(event))
{
	// The factory list is read only, if the user clicked on the checkbox
	// OnCheckListBoxToggleFactory() will issue a beep and insure that the 
	// checkbox state doesn't change
}

void CFilterPageWiz::OnWizardPageChanging(wxWizardEvent& event) 
{
	// Do any required changes to sfm and filter settings.
	// Note: We should not need to call RetokenizeText() from here because the 
	// filterPage is only accessible within the wizard after the user clicks on 
	// <New Project> from the projectPage, and no document will be open, since 
	// the docPage is the last page in the wizard, appearing after this page.
	// Note: Should the call to gpApp->DoUsfmFilterChanges() be done in the DocPage's
	// OnWizardFinish routine rather than here???

#ifdef _Trace_FilterMarkers
	TRACE0("In Filter page's OnWizardNext BEFORE DoUsfmFilterChanges call:\n");
	TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
	TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
	TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",pDoc->m_sfmSetBeforeEdit);
	TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",pDoc->m_filterMarkersBeforeEdit);
#endif

	// In the wizard the USFM page is always visited before the Filter page, and USFM's OnWizardPageChanging
	// updates the global sfm set and filter markers variables, so they need not be done again here
	// before calling DoUsfmFilterChanges.
	gpApp->DoUsfmFilterChanges(&filterPgCommon, NoReparse); // NoReparse within wizard since no doc is open there

#ifdef _Trace_FilterMarkers
	TRACE0("In Filter page's OnWizardNext AFTER DoUsfmFilterChanges call:\n");
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

// /////////////////////////////////// CFilterPagePrefs ///////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS( CFilterPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CFilterPagePrefs, wxPanel)
	EVT_INIT_DIALOG(CFilterPagePrefs::InitDialog)// not strictly necessary for dialogs based on wxDialog
	//EVT_SET_FOCUS(CFilterPagePrefs::OnSetFocus) // doesn't get called in a wizard page
	EVT_LISTBOX(IDC_LIST_SFMS, CFilterPagePrefs::OnLbnSelchangeListSfmsDoc)
	EVT_LISTBOX(IDC_LIST_SFMS_PROJ, CFilterPagePrefs::OnLbnSelchangeListSfmsProj)
	EVT_LISTBOX(IDC_LIST_SFMS_FACTORY, CFilterPagePrefs::OnLbnSelchangeListSfmsFactory)
END_EVENT_TABLE()

CFilterPagePrefs::CFilterPagePrefs()
{
}

CFilterPagePrefs::CFilterPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );

	filterPgCommon.DoSetDataAndPointers();
}

CFilterPagePrefs::~CFilterPagePrefs() // destructor
{
	
}

bool CFilterPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CFilterPagePrefs::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	filterPgCommon.pFilterPageSizer = FilterPageFunc(this, TRUE, TRUE);
}
	
void CFilterPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	filterPgCommon.DoInit();
}

void CFilterPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// wx version notes: Any changes made to the logic in OnOK should also be made to 
	// OnWizardPageChanging above.
	// In DoStartWorkingWizard, CFilterPagePrefs::OnWizardPageChanging() 
	// is called when page changes or Finish button is pressed. 
	//
	// CFilterPagePrefs::OnOK() is always called when the user dismisses the EditPreferencesDlg
	// dialog with the OK button. Note that whem EditPreferencesDlg's OK button is pressed,
	// this CFilterPagePrefs::OnOK() is also always called AFTER CUSFMPage::OnOK() is called.
	//
	// We've modified the normal property sheet behavior which was taht the page's OnOK() 
	// method would be called when the user clicks on OK in the property sheet, but only 
	// if the user accesses the filterPage via the tab (which also calls OnInitDialog)
	// before clicking the property sheet's OK button. Now CFilterPagePrefs::OnOK is always
	// called whenever EditPreferencesDlg OK button is pressed.

	// Validation of the language page data should be done in the caller's
	// OnOK() method before calling of this CFilterPagePrefs::OnOK() from there.


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
	wxASSERT(pUSFMPageInPrefs != NULL);
	// This filterPage::OnOK() handler should only be called from edit preferences
	// Since the usfmPage's OnOK handler is always called before this one, we don't need to call
	// DoUsfmSetChanges() from here. We can inspect bSFMsetChanged there to see if any sfm set change
	// occurred. If so, we call DoUsfmFilterChanges with the NoReparse parameter. For a case where there
	// only was a filtering change we call DoUsfmFilterChanges with the DoReparse parameter.
	if (pUSFMPageInPrefs->usfmPgCommon.bSFMsetChanged)
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
		gpApp->DoUsfmFilterChanges(&filterPgCommon, NoReparse);
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
		gpApp->DoUsfmFilterChanges(&filterPgCommon, DoReparse);
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


void CFilterPagePrefs::OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event))
{
	filterPgCommon.DoLbnSelchangeListSfmsDoc();
}
void CFilterPagePrefs::OnCheckListBoxToggleDoc(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleDoc(event);
}

void CFilterPagePrefs::OnCheckListBoxToggleProj(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleProj(event);
}

void CFilterPagePrefs::OnCheckListBoxToggleFactory(wxCommandEvent& event)
{
	filterPgCommon.DoCheckListBoxToggleFactory(event);
}

void CFilterPagePrefs::OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event))
{
	filterPgCommon.DoLbnSelchangeListSfmsProj();
}

void CFilterPagePrefs::OnLbnSelchangeListSfmsFactory(wxCommandEvent& WXUNUSED(event))
{
	// The factory list is read only, if the user clicked on the checkbox
	// OnCheckListBoxToggleFactory() will issue a beep and insure that the 
	// checkbox state doesn't change
}
